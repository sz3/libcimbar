#include "correct/reed-solomon.h"
#include "correct/reed-solomon/field.h"

polynomial_t polynomial_create(unsigned int order);
void polynomial_destroy(polynomial_t polynomial);
void polynomial_mul(field_t field, polynomial_t l, polynomial_t r, polynomial_t res);
void polynomial_mod(field_t field, polynomial_t dividend, polynomial_t divisor, polynomial_t mod);
void polynomial_formal_derivative(field_t field, polynomial_t poly, polynomial_t der);
field_element_t polynomial_eval(field_t field, polynomial_t poly, field_element_t val);
field_element_t polynomial_eval_lut(field_t field, polynomial_t poly, const field_logarithm_t *val_exp);
field_element_t polynomial_eval_log_lut(field_t field, polynomial_t poly_log, const field_logarithm_t *val_exp);
void polynomial_build_exp_lut(field_t field, field_element_t val, unsigned int order, field_logarithm_t *val_exp);
polynomial_t polynomial_init_from_roots(field_t field, unsigned int nroots, field_element_t *roots, polynomial_t poly, polynomial_t *scratch);
polynomial_t polynomial_create_from_roots(field_t field, unsigned int nroots, field_element_t *roots);
