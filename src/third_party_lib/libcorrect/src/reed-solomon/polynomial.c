#include "correct/reed-solomon/polynomial.h"

polynomial_t polynomial_create(unsigned int order) {
    polynomial_t polynomial;
    polynomial.coeff = malloc(sizeof(field_element_t) * (order + 1));
    polynomial.order = order;
    return polynomial;
}

void polynomial_destroy(polynomial_t polynomial) {
    free(polynomial.coeff);
}

// if you want a full multiplication, then make res.order = l.order + r.order
// but if you just care about a lower order, e.g. mul mod x^i, then you can select
//    fewer coefficients
void polynomial_mul(field_t field, polynomial_t l, polynomial_t r, polynomial_t res) {
    // perform an element-wise multiplication of two polynomials
    memset(res.coeff, 0, sizeof(field_element_t) * (res.order + 1));
    for (unsigned int i = 0; i <= l.order; i++) {
        if (i > res.order) {
            continue;
        }
        unsigned int j_limit = (r.order > res.order - i) ? res.order - i : r.order;
        for (unsigned int j = 0; j <= j_limit; j++) {
            // e.g. alpha^5*x * alpha^37*x^2 --> alpha^42*x^3
            res.coeff[i + j] = field_add(field, res.coeff[i + j], field_mul(field, l.coeff[i], r.coeff[j]));
        }
    }
}

void polynomial_mod(field_t field, polynomial_t dividend, polynomial_t divisor, polynomial_t mod) {
    // find the polynomial remainder of dividend mod divisor
    // do long division and return just the remainder (written to mod)

    if (mod.order < dividend.order) {
        // mod.order must be >= dividend.order (scratch space needed)
        // this is an error -- catch it in debug?
        return;
    }
    // initialize remainder as dividend
    memcpy(mod.coeff, dividend.coeff, sizeof(field_element_t) * (dividend.order + 1));

    // XXX make sure divisor[divisor_order] is nonzero
    field_logarithm_t divisor_leading = field.log[divisor.coeff[divisor.order]];
    // long division steps along one order at a time, starting at the highest order
    for (unsigned int i = dividend.order; i > 0; i--) {
        // look at the leading coefficient of dividend and divisor
        // if leading coefficient of dividend / leading coefficient of divisor is q
        //   then the next row of subtraction will be q * divisor
        // if order of q < 0 then what we have is the remainder and we are done
        if (i < divisor.order) {
            break;
        }
        if (mod.coeff[i] == 0) {
            continue;
        }
        unsigned int q_order = i - divisor.order;
        field_logarithm_t q_coeff = field_div_log(field, field.log[mod.coeff[i]], divisor_leading);

        // now that we've chosen q, multiply the divisor by q and subtract from
        //   our remainder. subtracting in GF(2^8) is XOR, just like addition
        for (unsigned int j = 0; j <= divisor.order; j++) {
            if (divisor.coeff[j] == 0) {
                continue;
            }
            // all of the multiplication is shifted up by q_order places
            mod.coeff[j + q_order] = field_add(field, mod.coeff[j + q_order],
                        field_mul_log_element(field, field.log[divisor.coeff[j]], q_coeff));
        }
    }
}

void polynomial_formal_derivative(field_t field, polynomial_t poly, polynomial_t der) {
    // if f(x) = a(n)*x^n + ... + a(1)*x + a(0)
    // then f'(x) = n*a(n)*x^(n-1) + ... + 2*a(2)*x + a(1)
    // where n*a(n) = sum(k=1, n, a(n)) e.g. the nth sum of a(n) in GF(2^8)

    // assumes der.order = poly.order - 1
    memset(der.coeff, 0, sizeof(field_element_t) * (der.order + 1));
    for (unsigned int i = 0; i <= der.order; i++) {
        // we're filling in the ith power of der, so we look ahead one power in poly
        // f(x) = a(i + 1)*x^(i + 1) -> f'(x) = (i + 1)*a(i + 1)*x^i
        // where (i + 1)*a(i + 1) is the sum of a(i + 1) (i + 1) times, not the product
        der.coeff[i] = field_sum(field, poly.coeff[i + 1], i + 1);
    }
}

field_element_t polynomial_eval(field_t field, polynomial_t poly, field_element_t val) {
    // evaluate the polynomial poly at a particular element val
    if (val == 0) {
        return poly.coeff[0];
    }

    field_element_t res = 0;

    // we're going to start at 0th order and multiply by val each time
    field_logarithm_t val_exponentiated = field.log[1];
    field_logarithm_t val_log = field.log[val];

    for (unsigned int i = 0; i <= poly.order; i++) {
        if (poly.coeff[i] != 0) {
            // multiply-accumulate by the next coeff times the next power of val
            res = field_add(field, res,
                    field_mul_log_element(field, field.log[poly.coeff[i]], val_exponentiated));
        }
        // now advance to the next power
        val_exponentiated = field_mul_log(field, val_exponentiated, val_log);
    }
    return res;
}

field_element_t polynomial_eval_lut(field_t field, polynomial_t poly, const field_logarithm_t *val_exp) {
    // evaluate the polynomial poly at a particular element val
    // in this case, all of the logarithms of the successive powers of val have been precalculated
    // this removes the extra work we'd have to do to calculate val_exponentiated each time
    //   if this function is to be called on the same val multiple times
    if (val_exp[0] == 0) {
        return poly.coeff[0];
    }

    field_element_t res = 0;

    for (unsigned int i = 0; i <= poly.order; i++) {
        if (poly.coeff[i] != 0) {
            // multiply-accumulate by the next coeff times the next power of val
            res = field_add(field, res,
                    field_mul_log_element(field, field.log[poly.coeff[i]], val_exp[i]));
        }
    }
    return res;
}

field_element_t polynomial_eval_log_lut(field_t field, polynomial_t poly_log, const field_logarithm_t *val_exp) {
    // evaluate the log_polynomial poly at a particular element val
    // like polynomial_eval_lut, the logarithms of the successive powers of val have been
    //   precomputed
    if (val_exp[0] == 0) {
        if (poly_log.coeff[0] == 0) {
            // special case for the non-existant log case
            return 0;
        }
        return field.exp[poly_log.coeff[0]];
    }

    field_element_t res = 0;

    for (unsigned int i = 0; i <= poly_log.order; i++) {
        // using 0 as a sentinel value in log -- log(0) is really -inf
        if (poly_log.coeff[i] != 0) {
            // multiply-accumulate by the next coeff times the next power of val
            res = field_add(field, res,
                    field_mul_log_element(field, poly_log.coeff[i], val_exp[i]));
        }
    }
    return res;
}

void polynomial_build_exp_lut(field_t field, field_element_t val, unsigned int order, field_logarithm_t *val_exp) {
    // create the lookup table of successive powers of val used by polynomial_eval_lut
    field_logarithm_t val_exponentiated = field.log[1];
    field_logarithm_t val_log = field.log[val];
    for (unsigned int i = 0; i <= order; i++) {
        if (val == 0) {
            val_exp[i] = 0;
        } else {
            val_exp[i] = val_exponentiated;
            val_exponentiated = field_mul_log(field, val_exponentiated, val_log);
        }
    }
}

polynomial_t polynomial_init_from_roots(field_t field, unsigned int nroots, field_element_t *roots, polynomial_t poly, polynomial_t *scratch) {
    unsigned int order = nroots;
    polynomial_t l;
    field_element_t l_coeff[2];
    l.order = 1;
    l.coeff = l_coeff;

    // we'll keep two temporary stores of rightside polynomial
    // each time through the loop, we take the previous result and use it as new rightside
    // swap back and forth (prevents the need for a copy)
    polynomial_t r[2];
    r[0] = scratch[0];
    r[1] = scratch[1];
    unsigned int rcoeffres = 0;

    // initialize the result with x + roots[0]
    r[rcoeffres].coeff[1] = 1;
    r[rcoeffres].coeff[0] = roots[0];
    r[rcoeffres].order = 1;

    // initialize lcoeff[1] with x
    // we'll fill in the 0th order term in each loop iter
    l.coeff[1] = 1;

    // loop through, using previous run's result as the new right hand side
    // this allows us to multiply one group at a time
    for (unsigned int i = 1; i < nroots; i++) {
        l.coeff[0] = roots[i];
        unsigned int nextrcoeff = rcoeffres;
        rcoeffres = (rcoeffres + 1) % 2;
        r[rcoeffres].order = i + 1;
        polynomial_mul(field, l, r[nextrcoeff], r[rcoeffres]);
    }

    memcpy(poly.coeff, r[rcoeffres].coeff, (order + 1) * sizeof(field_element_t));
    poly.order = order;

    return poly;
}

polynomial_t polynomial_create_from_roots(field_t field, unsigned int nroots, field_element_t *roots) {
    polynomial_t poly = polynomial_create(nroots);
    unsigned int order = nroots;
    polynomial_t l;
    l.order = 1;
    l.coeff = calloc(2, sizeof(field_element_t));

    polynomial_t r[2];
    // we'll keep two temporary stores of rightside polynomial
    // each time through the loop, we take the previous result and use it as new rightside
    // swap back and forth (prevents the need for a copy)
    r[0].coeff = calloc(order + 1, sizeof(field_element_t));
    r[1].coeff = calloc(order + 1, sizeof(field_element_t));
    unsigned int rcoeffres = 0;

    // initialize the result with x + roots[0]
    r[rcoeffres].coeff[0] = roots[0];
    r[rcoeffres].coeff[1] = 1;
    r[rcoeffres].order = 1;

    // initialize lcoeff[1] with x
    // we'll fill in the 0th order term in each loop iter
    l.coeff[1] = 1;

    // loop through, using previous run's result as the new right hand side
    // this allows us to multiply one group at a time
    for (unsigned int i = 1; i < nroots; i++) {
        l.coeff[0] = roots[i];
        unsigned int nextrcoeff = rcoeffres;
        rcoeffres = (rcoeffres + 1) % 2;
        r[rcoeffres].order = i + 1;
        polynomial_mul(field, l, r[nextrcoeff], r[rcoeffres]);
    }

    memcpy(poly.coeff, r[rcoeffres].coeff, (order + 1) * sizeof(field_element_t));
    poly.order = order;

    free(l.coeff);
    free(r[0].coeff);
    free(r[1].coeff);

    return poly;
}
