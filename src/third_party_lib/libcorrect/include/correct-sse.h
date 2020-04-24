#ifndef CORRECT_SSE_H
#define CORRECT_SSE_H
#include <correct.h>

struct correct_convolutional_sse;
typedef struct correct_convolutional_sse correct_convolutional_sse;

/* SSE versions of libcorrect's convolutional encoder/decoder.
 * These instances should not be used with the non-sse functions,
 * and non-sse instances should not be used with the sse functions.
 */

correct_convolutional_sse *correct_convolutional_sse_create(
    size_t rate, size_t order, const correct_convolutional_polynomial_t *poly);

void correct_convolutional_sse_destroy(correct_convolutional_sse *conv);

size_t correct_convolutional_sse_encode_len(correct_convolutional_sse *conv, size_t msg_len);

size_t correct_convolutional_sse_encode(correct_convolutional_sse *conv, const uint8_t *msg,
                                        size_t msg_len, uint8_t *encoded);

ssize_t correct_convolutional_sse_decode(correct_convolutional_sse *conv, const uint8_t *encoded,
                                         size_t num_encoded_bits, uint8_t *msg);

ssize_t correct_convolutional_sse_decode_soft(correct_convolutional_sse *conv,
                                              const correct_convolutional_soft_t *encoded,
                                              size_t num_encoded_bits, uint8_t *msg);

#endif
