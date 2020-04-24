#ifndef CORRECT_CONVOLUTIONAL_H
#define CORRECT_CONVOLUTIONAL_H
#include "correct/convolutional.h"
#include "correct/convolutional/bit.h"
#include "correct/convolutional/metric.h"
#include "correct/convolutional/lookup.h"
#include "correct/convolutional/history_buffer.h"
#include "correct/convolutional/error_buffer.h"

struct correct_convolutional {
    const unsigned int *table;  // size 2**order
    size_t rate;                // e.g. 2, 3...
    size_t order;               // e.g. 7, 9...
    unsigned int numstates;     // 2**order
    bit_writer_t *bit_writer;
    bit_reader_t *bit_reader;

    bool has_init_decode;
    distance_t *distances;
    pair_lookup_t pair_lookup;
    soft_measurement_t soft_measurement;
    history_buffer *history_buffer;
    error_buffer_t *errors;
};

correct_convolutional *_correct_convolutional_init(correct_convolutional *conv,
                                                   size_t rate, size_t order,
                                                   const polynomial_t *poly);
void _correct_convolutional_teardown(correct_convolutional *conv);

// portable versions
void _convolutional_decode_init(correct_convolutional *conv, unsigned int min_traceback, unsigned int traceback_length, unsigned int renormalize_interval);
void convolutional_decode_warmup(correct_convolutional *conv, unsigned int sets,
                                 const uint8_t *soft);
void convolutional_decode_inner(correct_convolutional *conv, unsigned int sets,
                                const uint8_t *soft);
void convolutional_decode_tail(correct_convolutional *conv, unsigned int sets,
                               const uint8_t *soft);
#endif

