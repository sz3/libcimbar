#include "correct/util/error-sim-fec.h"

void conv_fec27_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    init_viterbi27(conv_v, 0);
    update_viterbi27_blk(conv_v, soft, soft_len / 2 - 2);
    size_t n_decoded_bits = (soft_len / 2) - 8;
    chainback_viterbi27(conv_v, msg, n_decoded_bits, 0);
}

void conv_fec29_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    init_viterbi29(conv_v, 0);
    update_viterbi29_blk(conv_v, soft, soft_len / 2 - 2);
    size_t n_decoded_bits = (soft_len / 2) - 10;
    chainback_viterbi29(conv_v, msg, n_decoded_bits, 0);
}

void conv_fec39_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    init_viterbi39(conv_v, 0);
    update_viterbi39_blk(conv_v, soft, soft_len / 3 - 2);
    size_t n_decoded_bits = (soft_len / 3) - 10;
    chainback_viterbi39(conv_v, msg, n_decoded_bits, 0);
}

void conv_fec615_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    init_viterbi615(conv_v, 0);
    update_viterbi615_blk(conv_v, soft, soft_len / 6 - 2);
    size_t n_decoded_bits = (soft_len / 6) - 16;
    chainback_viterbi615(conv_v, msg, n_decoded_bits, 0);
}
