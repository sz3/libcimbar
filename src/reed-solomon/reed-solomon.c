#include "correct/reed-solomon/reed-solomon.h"

// coeff must be of size nroots + 1
// e.g. 2 roots (x + alpha)(x + alpha^2) yields a poly with 3 terms x^2 + g0*x + g1
static polynomial_t reed_solomon_build_generator(field_t field, unsigned int nroots, field_element_t first_consecutive_root, unsigned int root_gap, polynomial_t generator, field_element_t *roots) {
    // generator has order 2*t
    // of form (x + alpha^1)(x + alpha^2)...(x - alpha^2*t)
    for (unsigned int i = 0; i < nroots; i++) {
        roots[i] = field.exp[(root_gap * (i + first_consecutive_root)) % 255];
    }
    return polynomial_create_from_roots(field, nroots, roots);
}

correct_reed_solomon *correct_reed_solomon_create(field_operation_t primitive_polynomial, field_logarithm_t first_consecutive_root, field_logarithm_t generator_root_gap, size_t num_roots) {
    correct_reed_solomon *rs = calloc(1, sizeof(correct_reed_solomon));
    rs->field = field_create(primitive_polynomial);

    rs->block_length = 255;
    rs->min_distance = num_roots;
    rs->message_length = rs->block_length - rs->min_distance;

    rs->first_consecutive_root = first_consecutive_root;
    rs->generator_root_gap = generator_root_gap;

    rs->generator_roots = malloc(rs->min_distance * sizeof(field_element_t));

    rs->generator = reed_solomon_build_generator(rs->field, rs->min_distance, rs->first_consecutive_root, rs->generator_root_gap, rs->generator, rs->generator_roots);

    rs->encoded_polynomial = polynomial_create(rs->block_length - 1);
    rs->encoded_remainder = polynomial_create(rs->block_length - 1);

    rs->has_init_decode = false;

    return rs;
}

void correct_reed_solomon_destroy(correct_reed_solomon *rs) {
    field_destroy(rs->field);
    polynomial_destroy(rs->generator);
    free(rs->generator_roots);
    polynomial_destroy(rs->encoded_polynomial);
    polynomial_destroy(rs->encoded_remainder);
    if (rs->has_init_decode) {
        free(rs->syndromes);
        free(rs->modified_syndromes);
        polynomial_destroy(rs->received_polynomial);
        polynomial_destroy(rs->error_locator);
        polynomial_destroy(rs->error_locator_log);
        polynomial_destroy(rs->erasure_locator);
        free(rs->error_roots);
        free(rs->error_vals);
        free(rs->error_locations);
        polynomial_destroy(rs->last_error_locator);
        polynomial_destroy(rs->error_evaluator);
        polynomial_destroy(rs->error_locator_derivative);
        for (unsigned int i = 0; i < rs->min_distance; i++) {
            free(rs->generator_root_exp[i]);
        }
        free(rs->generator_root_exp);
        for (field_operation_t i = 0; i < 256; i++) {
            free(rs->element_exp[i]);
        }
        free(rs->element_exp);
        polynomial_destroy(rs->init_from_roots_scratch[0]);
        polynomial_destroy(rs->init_from_roots_scratch[1]);
    }
    free(rs);
}

void correct_reed_solomon_debug_print(correct_reed_solomon *rs) {
    for (unsigned int i = 0; i < 256; i++) {
        printf("%3d  %3d    %3d  %3d\n", i, rs->field.exp[i], i, rs->field.log[i]);
    }
    printf("\n");

    printf("roots: ");
    for (unsigned int i = 0; i < rs->min_distance; i++) {
        printf("%d", rs->generator_roots[i]);
        if (i < rs->min_distance - 1) {
            printf(", ");
        }
    }
    printf("\n\n");

    printf("generator: ");
    for (unsigned int i = 0; i < rs->generator.order + 1; i++) {
        printf("%d*x^%d", rs->generator.coeff[i], i);
        if (i < rs->generator.order) {
            printf(" + ");
        }
    }
    printf("\n\n");

    printf("generator (alpha format): ");
    for (unsigned int i = rs->generator.order + 1; i > 0; i--) {
        printf("alpha^%d*x^%d", rs->field.log[rs->generator.coeff[i - 1]], i - 1);
        if (i > 1) {
            printf(" + ");
        }
    }
    printf("\n\n");

    printf("remainder: ");
    bool has_printed = false;
    for (unsigned int i = 0; i < rs->encoded_remainder.order + 1; i++) {
        if (!rs->encoded_remainder.coeff[i]) {
            continue;
        }
        if (has_printed) {
            printf(" + ");
        }
        has_printed = true;
        printf("%d*x^%d", rs->encoded_remainder.coeff[i], i);
    }
    printf("\n\n");

    printf("syndromes: ");
    for (unsigned int i = 0; i < rs->min_distance; i++) {
        printf("%d", rs->syndromes[i]);
        if (i < rs->min_distance - 1) {
            printf(", ");
        }
    }
    printf("\n\n");

    printf("numerrors: %d\n\n", rs->error_locator.order);

    printf("error locator: ");
    has_printed = false;
    for (unsigned int i = 0; i < rs->error_locator.order + 1; i++) {
        if (!rs->error_locator.coeff[i]) {
            continue;
        }
        if (has_printed) {
            printf(" + ");
        }
        has_printed = true;
        printf("%d*x^%d", rs->error_locator.coeff[i], i);
    }
    printf("\n\n");

    printf("error roots: ");
    for (unsigned int i = 0; i < rs->error_locator.order; i++) {
        printf("%d@%d", polynomial_eval(rs->field, rs->error_locator, rs->error_roots[i]), rs->error_roots[i]);
        if (i < rs->error_locator.order - 1) {
            printf(", ");
        }
    }
    printf("\n\n");

    printf("error evaluator: ");
    has_printed = false;
    for (unsigned int i = 0; i < rs->error_evaluator.order; i++) {
        if (!rs->error_evaluator.coeff[i]) {
            continue;
        }
        if (has_printed) {
            printf(" + ");
        }
        has_printed = true;
        printf("%d*x^%d", rs->error_evaluator.coeff[i], i);
    }
    printf("\n\n");

    printf("error locator derivative: ");
    has_printed = false;
    for (unsigned int i = 0; i < rs->error_locator_derivative.order; i++) {
        if (!rs->error_locator_derivative.coeff[i]) {
            continue;
        }
        if (has_printed) {
            printf(" + ");
        }
        has_printed = true;
        printf("%d*x^%d", rs->error_locator_derivative.coeff[i], i);
    }
    printf("\n\n");

    printf("error locator: ");
    for (unsigned int i = 0; i < rs->error_locator.order; i++) {
        printf("%d@%d", rs->error_vals[i], rs->error_locations[i]);
        if (i < rs->error_locator.order - 1) {
            printf(", ");
        }
    }
    printf("\n\n");
}
