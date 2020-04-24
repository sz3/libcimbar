#include "correct/reed-solomon/encode.h"

ssize_t correct_reed_solomon_encode(correct_reed_solomon *rs, const uint8_t *msg, size_t msg_length, uint8_t *encoded) {
    if (msg_length > rs->message_length) {
        return -1;
    }

    size_t pad_length = rs->message_length - msg_length;
    for (unsigned int i = 0; i < msg_length; i++) {
        // message goes from high order to low order but libcorrect polynomials go low to high
        // so we reverse on the way in and on the way out
        // we'd have to do a copy anyway so this reversal should be free
        rs->encoded_polynomial.coeff[rs->encoded_polynomial.order - (i + pad_length)] = msg[i];
    }

    // 0-fill the rest of the coefficients -- this length will always be > 0
    // because the order of this poly is block_length and the msg_length <= message_length
    // e.g. 255 and 223
    memset(rs->encoded_polynomial.coeff + (rs->encoded_polynomial.order + 1 - pad_length), 0, pad_length);
    memset(rs->encoded_polynomial.coeff, 0, (rs->encoded_polynomial.order + 1 - rs->message_length));

    polynomial_mod(rs->field, rs->encoded_polynomial, rs->generator, rs->encoded_remainder);

    // now return byte order to highest order to lowest order
    for (unsigned int i = 0; i < msg_length; i++) {
        encoded[i] = rs->encoded_polynomial.coeff[rs->encoded_polynomial.order - (i + pad_length)];
    }

    for (unsigned int i = 0; i < rs->min_distance; i++) {
        encoded[msg_length + i] = rs->encoded_remainder.coeff[rs->min_distance - (i + 1)];
    }

    return rs->block_length;
}
