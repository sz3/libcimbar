#include "correct/reed-solomon/encode.h"

// calculate all syndromes of the received polynomial at the roots of the generator
// because we're evaluating at the roots of the generator, and because the transmitted
//   polynomial was made to be a product of the generator, we know that the transmitted
//   polynomial is 0 at these roots
// any nonzero syndromes we find here are the values of the error polynomial evaluated
//   at these roots, so these values give us a window into the error polynomial. if
//   these syndromes are all zero, then we can conclude the error polynomial is also
//   zero. if they're nonzero, then we know our message received an error in transit.
// returns true if syndromes are all zero
static bool reed_solomon_find_syndromes(field_t field, polynomial_t msgpoly, field_logarithm_t **generator_root_exp,
                                        field_element_t *syndromes, size_t min_distance) {
    bool all_zero = true;
    memset(syndromes, 0, min_distance * sizeof(field_element_t));
    for (unsigned int i = 0; i < min_distance; i++) {
        // profiling reveals that this function takes about 50% of the cpu time of
        // decoding. so, in order to speed it up a little, we precompute and save
        // the successive powers of the roots of the generator, which are
        // located in generator_root_exp
        field_element_t eval = polynomial_eval_lut(field, msgpoly, generator_root_exp[i]);
        if (eval) {
            all_zero = false;
        }
        syndromes[i] = eval;
    }
    return all_zero;
}

// Berlekamp-Massey algorithm to find LFSR that describes syndromes
// returns number of errors and writes the error locator polynomial to rs->error_locator
static unsigned int reed_solomon_find_error_locator(correct_reed_solomon *rs, size_t num_erasures) {
    unsigned int numerrors = 0;

    memset(rs->error_locator.coeff, 0, (rs->min_distance + 1) * sizeof(field_element_t));

    // initialize to f(x) = 1
    rs->error_locator.coeff[0] = 1;
    rs->error_locator.order = 0;

    memcpy(rs->last_error_locator.coeff, rs->error_locator.coeff, (rs->min_distance + 1) * sizeof(field_element_t));
    rs->last_error_locator.order = rs->error_locator.order;

    field_element_t discrepancy;
    field_element_t last_discrepancy = 1;
    unsigned int delay_length = 1;

    for (unsigned int i = rs->error_locator.order; i < rs->min_distance - num_erasures; i++) {
        discrepancy = rs->syndromes[i];
        for (unsigned int j = 1; j <= numerrors; j++) {
            discrepancy = field_add(rs->field, discrepancy,
                                    field_mul(rs->field, rs->error_locator.coeff[j], rs->syndromes[i - j]));
        }

        if (!discrepancy) {
            // our existing LFSR describes the new syndrome as well
            // leave it as-is but update the number of delay elements
            //   so that if a discrepancy occurs later we can eliminate it
            delay_length++;
            continue;
        }

        if (2 * numerrors <= i) {
            // there's a discrepancy, but we still have room for more taps
            // lengthen LFSR by one tap and set weight to eliminate discrepancy

            // shift the last locator by the delay length, multiply by discrepancy,
            //   and divide by the last discrepancy
            // we move down because we're shifting up, and this prevents overwriting
            for (int j = rs->last_error_locator.order; j >= 0; j--) {
                // the bounds here will be ok since we have a headroom of numerrors
                rs->last_error_locator.coeff[j + delay_length] = field_div(
                    rs->field, field_mul(rs->field, rs->last_error_locator.coeff[j], discrepancy), last_discrepancy);
            }
            for (int j = delay_length - 1; j >= 0; j--) {
                rs->last_error_locator.coeff[j] = 0;
            }

            // locator = locator - last_locator
            // we will also update last_locator to be locator before this loop takes place
            field_element_t temp;
            for (int j = 0; j <= (rs->last_error_locator.order + delay_length); j++) {
                temp = rs->error_locator.coeff[j];
                rs->error_locator.coeff[j] =
                    field_add(rs->field, rs->error_locator.coeff[j], rs->last_error_locator.coeff[j]);
                rs->last_error_locator.coeff[j] = temp;
            }
            unsigned int temp_order = rs->error_locator.order;
            rs->error_locator.order = rs->last_error_locator.order + delay_length;
            rs->last_error_locator.order = temp_order;

            // now last_locator is locator before we started,
            //   and locator is (locator - (discrepancy/last_discrepancy) * x^(delay_length) * last_locator)

            numerrors = i + 1 - numerrors;
            last_discrepancy = discrepancy;
            delay_length = 1;
            continue;
        }

        // no more taps
        // unlike the previous case, we are preserving last locator,
        //    but we'll update locator as before
        // we're basically flattening the two loops from the previous case because
        //    we no longer need to update last_locator
        for (int j = rs->last_error_locator.order; j >= 0; j--) {
            rs->error_locator.coeff[j + delay_length] =
                field_add(rs->field, rs->error_locator.coeff[j + delay_length],
                          field_div(rs->field, field_mul(rs->field, rs->last_error_locator.coeff[j], discrepancy),
                                    last_discrepancy));
        }
        rs->error_locator.order = (rs->last_error_locator.order + delay_length > rs->error_locator.order)
                                      ? rs->last_error_locator.order + delay_length
                                      : rs->error_locator.order;
        delay_length++;
    }
    return rs->error_locator.order;
}

// find the roots of the error locator polynomial
// Chien search
bool reed_solomon_factorize_error_locator(field_t field, unsigned int num_skip, polynomial_t locator_log, field_element_t *roots,
                                          field_logarithm_t **element_exp) {
    // normally it'd be tricky to find all the roots
    // but, the finite field is awfully finite...
    // just brute force search across every field element
    unsigned int root = num_skip;
    memset(roots + num_skip, 0, (locator_log.order) * sizeof(field_element_t));
    for (field_operation_t i = 0; i < 256; i++) {
        // we make two optimizations here to help this search go faster
        // a) we have precomputed the first successive powers of every single element
        //   in the field. we need at most n powers, where n is the largest possible
        //   degree of the error locator
        // b) we have precomputed the error locator polynomial in log form, which
        //   helps reduce some lookups that would be done here
        if (!polynomial_eval_log_lut(field, locator_log, element_exp[i])) {
            roots[root] = (field_element_t)i;
            root++;
        }
    }
    // this is where we find out if we are have too many errors to recover from
    // berlekamp-massey may have built an error locator that has 0 discrepancy
    // on the syndromes but doesn't have enough roots
    return root == locator_log.order + num_skip;
}

// use error locator and syndromes to find the error evaluator polynomial
void reed_solomon_find_error_evaluator(field_t field, polynomial_t locator, polynomial_t syndromes,
                                       polynomial_t error_evaluator) {
    // the error evaluator, omega(x), is S(x)*Lamba(x) mod x^(2t)
    // where S(x) is a polynomial constructed from the syndromes
    //   S(1) + S(2)*x + ... + S(2t)*x(2t - 1)
    // and Lambda(x) is the error locator
    // the modulo is implicit here -- we have limited the max length of error_evaluator,
    //   which polynomial_mul will interpret to mean that it should not compute
    //   powers larger than that, which is the same as performing mod x^(2t)
    polynomial_mul(field, locator, syndromes, error_evaluator);
}

// use error locator, error roots and syndromes to find the error values
// that is, the elements in the finite field which can be added to the received
//   polynomial at the locations of the error roots in order to produce the
//   transmitted polynomial
// forney algorithm
void reed_solomon_find_error_values(correct_reed_solomon *rs) {
    // error value e(j) = -(X(j)^(1-c) * omega(X(j)^-1))/(lambda'(X(j)^-1))
    // where X(j)^-1 is a root of the error locator, omega(X) is the error evaluator,
    //   lambda'(X) is the first formal derivative of the error locator,
    //   and c is the first consecutive root of the generator used in encoding

    // first find omega(X), the error evaluator
    // we generate S(x), the polynomial constructed from the roots of the syndromes
    // this is *not* the polynomial constructed by expanding the products of roots
    // S(x) = S(1) + S(2)*x + ... + S(2t)*x(2t - 1)
    polynomial_t syndrome_poly;
    syndrome_poly.order = rs->min_distance - 1;
    syndrome_poly.coeff = rs->syndromes;
    memset(rs->error_evaluator.coeff, 0, (rs->error_evaluator.order + 1) * sizeof(field_element_t));
    reed_solomon_find_error_evaluator(rs->field, rs->error_locator, syndrome_poly, rs->error_evaluator);

    // now find lambda'(X)
    rs->error_locator_derivative.order = rs->error_locator.order - 1;
    polynomial_formal_derivative(rs->field, rs->error_locator, rs->error_locator_derivative);

    // calculate each e(j)
    for (unsigned int i = 0; i < rs->error_locator.order; i++) {
        if (rs->error_roots[i] == 0) {
            continue;
        }
        rs->error_vals[i] = field_mul(
            rs->field, field_pow(rs->field, rs->error_roots[i], rs->first_consecutive_root - 1),
            field_div(
                rs->field, polynomial_eval_lut(rs->field, rs->error_evaluator, rs->element_exp[rs->error_roots[i]]),
                polynomial_eval_lut(rs->field, rs->error_locator_derivative, rs->element_exp[rs->error_roots[i]])));
    }
}

void reed_solomon_find_error_locations(field_t field, field_logarithm_t generator_root_gap,
                                       field_element_t *error_roots, field_logarithm_t *error_locations,
                                       unsigned int num_errors, unsigned int num_skip) {
    for (unsigned int i = 0; i < num_errors; i++) {
        // the error roots are the reciprocals of the error locations, so div 1 by them

        // we do mod 255 here because the log table aliases at index 1
        // the log of 1 is both 0 and 255 (alpha^255 = alpha^0 = 1)
        // for most uses it makes sense to have log(1) = 255, but in this case
        // we're interested in a byte index, and the 255th index is not even valid
        // just wrap it back to 0

        if (error_roots[i] == 0) {
            continue;
        }

        field_operation_t loc = field_div(field, 1, error_roots[i]);
        for (field_operation_t j = 0; j < 256; j++) {
            if (field_pow(field, j, generator_root_gap) == loc) {
                error_locations[i] = field.log[j];
                break;
            }
        }
    }
}

// erasure method -- take given locations and convert to roots
// this is the inverse of reed_solomon_find_error_locations
static void reed_solomon_find_error_roots_from_locations(field_t field, field_logarithm_t generator_root_gap,
                                                         const field_logarithm_t *error_locations,
                                                         field_element_t *error_roots, unsigned int num_errors) {
    for (unsigned int i = 0; i < num_errors; i++) {
        field_element_t loc = field_pow(field, field.exp[error_locations[i]], generator_root_gap);
        // field_element_t loc = field.exp[error_locations[i]];
        error_roots[i] = field_div(field, 1, loc);
        // error_roots[i] = loc;
    }
}

// erasure method -- given the roots of the error locator, create the polynomial
static polynomial_t reed_solomon_find_error_locator_from_roots(field_t field, unsigned int num_errors,
                                                               field_element_t *error_roots,
                                                               polynomial_t error_locator,
                                                               polynomial_t *scratch) {
    // multiply out roots to build the error locator polynomial
    return polynomial_init_from_roots(field, num_errors, error_roots, error_locator, scratch);
}

// erasure method
static void reed_solomon_find_modified_syndromes(correct_reed_solomon *rs, field_element_t *syndromes, polynomial_t error_locator, field_element_t *modified_syndromes) {
    polynomial_t syndrome_poly;
    syndrome_poly.order = rs->min_distance - 1;
    syndrome_poly.coeff = syndromes;

    polynomial_t modified_syndrome_poly;
    modified_syndrome_poly.order = rs->min_distance - 1;
    modified_syndrome_poly.coeff = modified_syndromes;

    polynomial_mul(rs->field, error_locator, syndrome_poly, modified_syndrome_poly);
}

void correct_reed_solomon_decoder_create(correct_reed_solomon *rs) {
    rs->has_init_decode = true;
    rs->syndromes = calloc(rs->min_distance, sizeof(field_element_t));
    rs->modified_syndromes = calloc(2 * rs->min_distance, sizeof(field_element_t));
    rs->received_polynomial = polynomial_create(rs->block_length - 1);
    rs->error_locator = polynomial_create(rs->min_distance);
    rs->error_locator_log = polynomial_create(rs->min_distance);
    rs->erasure_locator = polynomial_create(rs->min_distance);
    rs->error_roots = calloc(2 * rs->min_distance, sizeof(field_element_t));
    rs->error_vals = malloc(rs->min_distance * sizeof(field_element_t));
    rs->error_locations = malloc(rs->min_distance * sizeof(field_logarithm_t));

    rs->last_error_locator = polynomial_create(rs->min_distance);
    rs->error_evaluator = polynomial_create(rs->min_distance - 1);
    rs->error_locator_derivative = polynomial_create(rs->min_distance - 1);

    // calculate and store the first block_length powers of every generator root
    // we would have to do this work in order to calculate the syndromes
    // if we save it, we can prevent the need to recalculate it on subsequent calls
    // total memory usage is min_distance * block_length bytes e.g. 32 * 255 ~= 8k
    rs->generator_root_exp = malloc(rs->min_distance * sizeof(field_logarithm_t *));
    for (unsigned int i = 0; i < rs->min_distance; i++) {
        rs->generator_root_exp[i] = malloc(rs->block_length * sizeof(field_logarithm_t));
        polynomial_build_exp_lut(rs->field, rs->generator_roots[i], rs->block_length - 1, rs->generator_root_exp[i]);
    }

    // calculate and store the first min_distance powers of every element in the field
    // we would have to do this for chien search anyway, and its size is only 256 * min_distance bytes
    // for min_distance = 32 this is 8k of memory, a pittance for the speedup we receive in exchange
    // we also get to reuse this work during error value calculation
    rs->element_exp = malloc(256 * sizeof(field_logarithm_t *));
    for (field_operation_t i = 0; i < 256; i++) {
        rs->element_exp[i] = malloc(rs->min_distance * sizeof(field_logarithm_t));
        polynomial_build_exp_lut(rs->field, i, rs->min_distance - 1, rs->element_exp[i]);
    }

    rs->init_from_roots_scratch[0] = polynomial_create(rs->min_distance);
    rs->init_from_roots_scratch[1] = polynomial_create(rs->min_distance);
}

ssize_t correct_reed_solomon_decode(correct_reed_solomon *rs, const uint8_t *encoded, size_t encoded_length,
                                    uint8_t *msg) {
    if (encoded_length > rs->block_length) {
        return -1;
    }

    // the message is the non-remainder part
    size_t msg_length = encoded_length - rs->min_distance;
    // if they handed us a nonfull block, we'll write in 0s
    size_t pad_length = rs->block_length - encoded_length;

    if (!rs->has_init_decode) {
        // initialize rs for decoding
        correct_reed_solomon_decoder_create(rs);
    }

    // we need to copy to our local buffer
    // the buffer we're given has the coordinates in the wrong direction
    // e.g. byte 0 corresponds to the 254th order coefficient
    // so we're going to flip and then write padding
    // the final copied buffer will look like
    // | rem (rs->min_distance) | msg (msg_length) | pad (pad_length) |

    for (unsigned int i = 0; i < encoded_length; i++) {
        rs->received_polynomial.coeff[i] = encoded[encoded_length - (i + 1)];
    }

    // fill the pad_length with 0s
    for (unsigned int i = 0; i < pad_length; i++) {
        rs->received_polynomial.coeff[i + encoded_length] = 0;
    }


    bool all_zero = reed_solomon_find_syndromes(rs->field, rs->received_polynomial, rs->generator_root_exp,
                                                rs->syndromes, rs->min_distance);

    if (all_zero) {
        // syndromes were all zero, so there was no error in the message
        // copy to msg and we are done
        for (unsigned int i = 0; i < msg_length; i++) {
            msg[i] = rs->received_polynomial.coeff[encoded_length - (i + 1)];
        }
        return msg_length;
    }

    unsigned int order = reed_solomon_find_error_locator(rs, 0);
    // XXX fix this vvvv
    rs->error_locator.order = order;

    for (unsigned int i = 0; i <= rs->error_locator.order; i++) {
        // this is a little strange since the coeffs are logs, not elements
        // also, we'll be storing log(0) = 0 for any 0 coeffs in the error locator
        // that would seem bad but we'll just be using this in chien search, and we'll skip all 0 coeffs
        // (you might point out that log(1) also = 0, which would seem to alias. however, that's ok,
        //   because log(1) = 255 as well, and in fact that's how it's represented in our log table)
        rs->error_locator_log.coeff[i] = rs->field.log[rs->error_locator.coeff[i]];
    }
    rs->error_locator_log.order = rs->error_locator.order;

    if (!reed_solomon_factorize_error_locator(rs->field, 0, rs->error_locator_log, rs->error_roots, rs->element_exp)) {
        // roots couldn't be found, so there were too many errors to deal with
        // RS has failed for this message
        return -1;
    }

    reed_solomon_find_error_locations(rs->field, rs->generator_root_gap, rs->error_roots, rs->error_locations,
                                      rs->error_locator.order, 0);

    reed_solomon_find_error_values(rs);

    for (unsigned int i = 0; i < rs->error_locator.order; i++) {
        rs->received_polynomial.coeff[rs->error_locations[i]] =
            field_sub(rs->field, rs->received_polynomial.coeff[rs->error_locations[i]], rs->error_vals[i]);
    }

    for (unsigned int i = 0; i < msg_length; i++) {
        msg[i] = rs->received_polynomial.coeff[encoded_length - (i + 1)];
    }

    return msg_length;
}

ssize_t correct_reed_solomon_decode_with_erasures(correct_reed_solomon *rs, const uint8_t *encoded,
                                                  size_t encoded_length, const uint8_t *erasure_locations,
                                                  size_t erasure_length, uint8_t *msg) {
    if (!erasure_length) {
        return correct_reed_solomon_decode(rs, encoded, encoded_length, msg);
    }

    if (encoded_length > rs->block_length) {
        return -1;
    }

    if (erasure_length > rs->min_distance) {
        return -1;
    }

    // the message is the non-remainder part
    size_t msg_length = encoded_length - rs->min_distance;
    // if they handed us a nonfull block, we'll write in 0s
    size_t pad_length = rs->block_length - encoded_length;

    if (!rs->has_init_decode) {
        // initialize rs for decoding
        correct_reed_solomon_decoder_create(rs);
    }

    // we need to copy to our local buffer
    // the buffer we're given has the coordinates in the wrong direction
    // e.g. byte 0 corresponds to the 254th order coefficient
    // so we're going to flip and then write padding
    // the final copied buffer will look like
    // | rem (rs->min_distance) | msg (msg_length) | pad (pad_length) |

    for (unsigned int i = 0; i < encoded_length; i++) {
        rs->received_polynomial.coeff[i] = encoded[encoded_length - (i + 1)];
    }

    // fill the pad_length with 0s
    for (unsigned int i = 0; i < pad_length; i++) {
        rs->received_polynomial.coeff[i + encoded_length] = 0;
    }

    for (unsigned int i = 0; i < erasure_length; i++) {
        // remap the coordinates of the erasures
        rs->error_locations[i] = rs->block_length - (erasure_locations[i] + pad_length + 1);
    }

    reed_solomon_find_error_roots_from_locations(rs->field, rs->generator_root_gap, rs->error_locations,
                                                 rs->error_roots, erasure_length);

    rs->erasure_locator =
        reed_solomon_find_error_locator_from_roots(rs->field, erasure_length, rs->error_roots, rs->erasure_locator, rs->init_from_roots_scratch);

    bool all_zero = reed_solomon_find_syndromes(rs->field, rs->received_polynomial, rs->generator_root_exp,
                                                rs->syndromes, rs->min_distance);

    if (all_zero) {
        // syndromes were all zero, so there was no error in the message
        // copy to msg and we are done
        for (unsigned int i = 0; i < msg_length; i++) {
            msg[i] = rs->received_polynomial.coeff[encoded_length - (i + 1)];
        }
        return msg_length;
    }

    reed_solomon_find_modified_syndromes(rs, rs->syndromes, rs->erasure_locator, rs->modified_syndromes);

    field_element_t *syndrome_copy = malloc(rs->min_distance * sizeof(field_element_t));
    memcpy(syndrome_copy, rs->syndromes, rs->min_distance * sizeof(field_element_t));

    for (unsigned int i = erasure_length; i < rs->min_distance; i++) {
        rs->syndromes[i - erasure_length] = rs->modified_syndromes[i];
    }

    unsigned int order = reed_solomon_find_error_locator(rs, erasure_length);
    // XXX fix this vvvv
    rs->error_locator.order = order;

    for (unsigned int i = 0; i <= rs->error_locator.order; i++) {
        // this is a little strange since the coeffs are logs, not elements
        // also, we'll be storing log(0) = 0 for any 0 coeffs in the error locator
        // that would seem bad but we'll just be using this in chien search, and we'll skip all 0 coeffs
        // (you might point out that log(1) also = 0, which would seem to alias. however, that's ok,
        //   because log(1) = 255 as well, and in fact that's how it's represented in our log table)
        rs->error_locator_log.coeff[i] = rs->field.log[rs->error_locator.coeff[i]];
    }
    rs->error_locator_log.order = rs->error_locator.order;

    /*
    for (unsigned int i = 0; i < erasure_length; i++) {
        rs->error_roots[i] = field_div(rs->field, 1, rs->error_roots[i]);
    }
    */

    if (!reed_solomon_factorize_error_locator(rs->field, erasure_length, rs->error_locator_log, rs->error_roots, rs->element_exp)) {
        // roots couldn't be found, so there were too many errors to deal with
        // RS has failed for this message
        free(syndrome_copy);
        return -1;
    }

    polynomial_t temp_poly = polynomial_create(rs->error_locator.order + erasure_length);
    polynomial_mul(rs->field, rs->erasure_locator, rs->error_locator, temp_poly);
    polynomial_t placeholder_poly = rs->error_locator;
    rs->error_locator = temp_poly;

    reed_solomon_find_error_locations(rs->field, rs->generator_root_gap, rs->error_roots, rs->error_locations,
                                      rs->error_locator.order, erasure_length);

    memcpy(rs->syndromes, syndrome_copy, rs->min_distance * sizeof(field_element_t));

    reed_solomon_find_error_values(rs);

    for (unsigned int i = 0; i < rs->error_locator.order; i++) {
        rs->received_polynomial.coeff[rs->error_locations[i]] =
            field_sub(rs->field, rs->received_polynomial.coeff[rs->error_locations[i]], rs->error_vals[i]);
    }

    rs->error_locator = placeholder_poly;

    for (unsigned int i = 0; i < msg_length; i++) {
        msg[i] = rs->received_polynomial.coeff[encoded_length - (i + 1)];
    }

    polynomial_destroy(temp_poly);
    free(syndrome_copy);

    return msg_length;
}
