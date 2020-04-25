#include "correct/reed-solomon.h"

size_t block_size = 255;
int power_max = 8;

// visit all of the elements from the poly
bool trypoly(field_operation_t poly, field_logarithm_t *log) {
    memset(log, 0, block_size + 1);
    field_operation_t element = 1;
    log[0] = (field_logarithm_t)0;
    for (field_operation_t i = 1; i < block_size + 1; i++) {
        element = element * 2;
        element = (element > block_size) ? (element ^ poly) : element;
        if (log[element] != 0) {
            return false;
        }
        log[element] = (field_logarithm_t)i;
    }
    return true;
}

int main() {
    field_logarithm_t *log = malloc((block_size + 1) * sizeof(field_logarithm_t));
    for (field_operation_t i = (block_size + 1); i < (block_size + 1) << 1; i++) {
        if (trypoly(i, log)) {
            printf("0x%x valid: ", i);
            field_operation_t poly = i;
            int power = power_max;
            while(poly) {
                if (poly & (block_size + 1)) {
                    if (power > 1) {
                        printf("x^%d", power);
                    } else if (power) {
                        printf("x");
                    } else {
                        printf("1");
                    }
                    if (poly & block_size) {
                        printf(" + ");
                    }
                }
                power--;
                poly <<= 1;
                poly &= (block_size << 1) + 1;
            }
            printf("\n");
        }
    }
    free(log);
    return 0;
}
