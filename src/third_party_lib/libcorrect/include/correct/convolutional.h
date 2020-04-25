#ifndef CORRECT_CONVOLUTIONAL
#define CORRECT_CONVOLUTIONAL
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "correct.h"
#include "correct/portable.h"

typedef unsigned int shift_register_t;
typedef uint16_t polynomial_t;
typedef uint64_t path_t;
typedef uint8_t soft_t;
static const soft_t soft_max = UINT8_MAX;

typedef uint16_t distance_t;
static const distance_t distance_max = UINT16_MAX;

typedef enum {
    CORRECT_SOFT_LINEAR,
    CORRECT_SOFT_QUADRATIC,
} soft_measurement_t;
#endif
