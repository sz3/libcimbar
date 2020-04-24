#include "correct/util/error-sim.h"

#include "correct-sse.h"

size_t conv_correct_sse_enclen(void *conv_v, size_t msg_len);
void conv_correct_sse_encode(void *conv_v, uint8_t *msg, size_t msg_len, uint8_t *encoded);
ssize_t conv_correct_sse_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg);
