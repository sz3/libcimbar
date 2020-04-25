#include "correct/util/error-sim-sse.h"

size_t conv_correct_sse_enclen(void *conv_v, size_t msg_len) {
    return correct_convolutional_sse_encode_len((correct_convolutional_sse *)conv_v, msg_len);
}

void conv_correct_sse_encode(void *conv_v, uint8_t *msg, size_t msg_len, uint8_t *encoded) {
    correct_convolutional_sse_encode((correct_convolutional_sse *)conv_v, msg, msg_len, encoded);
}

ssize_t conv_correct_sse_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    return correct_convolutional_sse_decode_soft((correct_convolutional_sse *)conv_v, soft, soft_len, msg);
}
