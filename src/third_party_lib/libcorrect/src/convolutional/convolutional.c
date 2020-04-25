#include "correct/convolutional/convolutional.h"

// https://www.youtube.com/watch?v=b3_lVSrPB6w

correct_convolutional *_correct_convolutional_init(correct_convolutional *conv,
                                                   size_t rate, size_t order,
                                                   const polynomial_t *poly) {
    if (order > 8 * sizeof(shift_register_t)) {
        // XXX turn this into an error code
        // printf("order must be smaller than 8 * sizeof(shift_register_t)\n");
        return NULL;
    }
    if (rate < 2) {
        // XXX turn this into an error code
        // printf("rate must be 2 or greater\n");
        return NULL;
    }

    conv->order = order;
    conv->rate = rate;
    conv->numstates = 1 << order;

    unsigned int *table = malloc(sizeof(unsigned int) * (1 << order));
    fill_table(conv->rate, conv->order, poly, table);
    *(unsigned int **)&conv->table = table;

    conv->bit_writer = bit_writer_create(NULL, 0);
    conv->bit_reader = bit_reader_create(NULL, 0);

    conv->has_init_decode = false;
    return conv;
}

correct_convolutional *correct_convolutional_create(size_t rate, size_t order,
                                                    const polynomial_t *poly) {
    correct_convolutional *conv = malloc(sizeof(correct_convolutional));
    correct_convolutional *init_conv = _correct_convolutional_init(conv, rate, order, poly);
    if (!init_conv) {
        free(conv);
    }
    return init_conv;
}

void _correct_convolutional_teardown(correct_convolutional *conv) {
    free(*(unsigned int **)&conv->table);
    bit_writer_destroy(conv->bit_writer);
    bit_reader_destroy(conv->bit_reader);
    if (conv->has_init_decode) {
        pair_lookup_destroy(conv->pair_lookup);
        history_buffer_destroy(conv->history_buffer);
        error_buffer_destroy(conv->errors);
        free(conv->distances);
    }
}

void correct_convolutional_destroy(correct_convolutional *conv) {
    _correct_convolutional_teardown(conv);
    free(conv);
}
