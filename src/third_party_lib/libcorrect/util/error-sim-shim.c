#include "correct/util/error-sim-shim.h"

ssize_t conv_shim27_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    init_viterbi27(conv_v, 0);
    update_viterbi27_blk(conv_v, soft, soft_len / 2 - 2);
    size_t n_decoded_bits = (soft_len / 2) - 8;
    chainback_viterbi27(conv_v, msg, n_decoded_bits, 0);
    return (n_decoded_bits % 8) ? (n_decoded_bits / 8) + 1 : n_decoded_bits / 8;
}

ssize_t conv_shim29_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    init_viterbi29(conv_v, 0);
    update_viterbi29_blk(conv_v, soft, soft_len / 2 - 2);
    size_t n_decoded_bits = (soft_len / 2) - 10;
    chainback_viterbi29(conv_v, msg, n_decoded_bits, 0);
    return (n_decoded_bits % 8) ? (n_decoded_bits / 8) + 1 : n_decoded_bits / 8;
}

ssize_t conv_shim39_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    init_viterbi39(conv_v, 0);
    update_viterbi39_blk(conv_v, soft, soft_len / 3 - 2);
    size_t n_decoded_bits = (soft_len / 3) - 10;
    chainback_viterbi39(conv_v, msg, n_decoded_bits, 0);
    return (n_decoded_bits % 8) ? (n_decoded_bits / 8) + 1 : n_decoded_bits / 8;
}

ssize_t conv_shim615_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    init_viterbi615(conv_v, 0);
    update_viterbi615_blk(conv_v, soft, soft_len / 6 - 2);
    size_t n_decoded_bits = (soft_len / 6) - 16;
    chainback_viterbi615(conv_v, msg, n_decoded_bits, 0);
    return (n_decoded_bits % 8) ? (n_decoded_bits / 8) + 1 : n_decoded_bits / 8;
}
