#include "correct/convolutional/sse/convolutional.h"

correct_convolutional_sse *correct_convolutional_sse_create(size_t rate,
                                                            size_t order,
                                                            const polynomial_t *poly) {
    correct_convolutional_sse *conv = malloc(sizeof(correct_convolutional_sse));
    correct_convolutional *init_conv = _correct_convolutional_init(&conv->base_conv, rate, order, poly);
    if (!init_conv) {
        free(conv);
        conv = NULL;
    }
    return conv;
}

void correct_convolutional_sse_destroy(correct_convolutional_sse *conv) {
    if (conv->base_conv.has_init_decode) {
        oct_lookup_destroy(conv->oct_lookup);
    }
    _correct_convolutional_teardown(&conv->base_conv);
    free(conv);
}
