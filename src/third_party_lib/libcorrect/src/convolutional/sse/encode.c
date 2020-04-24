#include "correct/convolutional/sse/convolutional.h"

size_t correct_convolutional_sse_encode_len(correct_convolutional_sse *conv, size_t msg_len) {
    return correct_convolutional_encode_len(&conv->base_conv, msg_len);
}

size_t correct_convolutional_sse_encode(correct_convolutional_sse *conv, const uint8_t *msg, size_t msg_len, uint8_t *encoded) {
    return correct_convolutional_encode(&conv->base_conv, msg, msg_len, encoded);
}
