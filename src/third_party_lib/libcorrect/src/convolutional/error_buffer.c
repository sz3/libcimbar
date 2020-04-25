#include "correct/convolutional/error_buffer.h"

error_buffer_t *error_buffer_create(unsigned int num_states) {
    error_buffer_t *buf = calloc(1, sizeof(error_buffer_t));

    // how large are the error buffers?
    buf->num_states = num_states;

    // save two error metrics, one for last round and one for this
    // (double buffer)
    // the error metric is the aggregated number of bit errors found
    //   at a given path which terminates at a particular shift register state
    buf->errors[0] = calloc(sizeof(distance_t), num_states);
    buf->errors[1] = calloc(sizeof(distance_t), num_states);

    // which buffer are we using, 0 or 1?
    buf->index = 0;

    buf->read_errors = buf->errors[0];
    buf->write_errors = buf->errors[1];

    return buf;
}

void error_buffer_destroy(error_buffer_t *buf) {
    free(buf->errors[0]);
    free(buf->errors[1]);
    free(buf);
}

void error_buffer_reset(error_buffer_t *buf) {
    memset(buf->errors[0], 0, buf->num_states * sizeof(distance_t));
    memset(buf->errors[1], 0, buf->num_states * sizeof(distance_t));
    buf->index = 0;
    buf->read_errors = buf->errors[0];
    buf->write_errors = buf->errors[1];
}

void error_buffer_swap(error_buffer_t *buf) {
    buf->read_errors = buf->errors[buf->index];
    buf->index = (buf->index + 1) % 2;
    buf->write_errors = buf->errors[buf->index];
}
