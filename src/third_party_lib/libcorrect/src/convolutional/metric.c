#include "correct/convolutional/metric.h"

// measure the square of the euclidean distance between x and y
// since euclidean dist is sqrt(a^2 + b^2 + ... + n^2), the square is just
//    a^2 + b^2 + ... + n^2
distance_t metric_soft_distance_quadratic(unsigned int hard_x, const uint8_t *soft_y, size_t len) {
    distance_t dist = 0;
    for (unsigned int i = 0; i < len; i++) {
        // first, convert hard_x to a soft measurement (0 -> 0, 1 - > 255)
        unsigned int soft_x = (hard_x & 1) ? 255 : 0;
        hard_x >>= 1;
        int d = soft_y[i] - soft_x;
        dist += d*d;
    }
    return dist >> 3;
}

