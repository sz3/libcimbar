#include "correct/convolutional/history_buffer.h"

history_buffer *history_buffer_create(unsigned int min_traceback_length,
                                      unsigned int traceback_group_length,
                                      unsigned int renormalize_interval, unsigned int num_states,
                                      shift_register_t highbit) {
    history_buffer *buf = calloc(1, sizeof(history_buffer));

    *(unsigned int *)&buf->min_traceback_length = min_traceback_length;
    *(unsigned int *)&buf->traceback_group_length = traceback_group_length;
    *(unsigned int *)&buf->cap = min_traceback_length + traceback_group_length;
    *(unsigned int *)&buf->num_states = num_states;
    *(shift_register_t *)&buf->highbit = highbit;

    buf->history = malloc(buf->cap * sizeof(uint8_t *));
    for (unsigned int i = 0; i < buf->cap; i++) {
        buf->history[i] = calloc(num_states, sizeof(uint8_t));
    }
    buf->fetched = malloc(buf->cap * sizeof(uint8_t));

    buf->index = 0;
    buf->len = 0;

    buf->renormalize_counter = 0;
    buf->renormalize_interval = renormalize_interval;

    return buf;
}

void history_buffer_destroy(history_buffer *buf) {
    for (unsigned int i = 0; i < buf->cap; i++) {
        free(buf->history[i]);
    }
    free(buf->history);
    free(buf->fetched);
    free(buf);
}

void history_buffer_reset(history_buffer *buf) {
    buf->len = 0;
    buf->index = 0;
}

uint8_t *history_buffer_get_slice(history_buffer *buf) { return buf->history[buf->index]; }

shift_register_t history_buffer_search(history_buffer *buf, const distance_t *distances,
                                       unsigned int search_every) {
    shift_register_t bestpath;
    distance_t leasterror = USHRT_MAX;
    // search for a state with the least error
    for (shift_register_t state = 0; state < buf->num_states; state += search_every) {
        if (distances[state] < leasterror) {
            leasterror = distances[state];
            bestpath = state;
        }
    }
    return bestpath;
}

void history_buffer_renormalize(history_buffer *buf, distance_t *distances,
                                shift_register_t min_register) {
    distance_t min_distance = distances[min_register];
    for (shift_register_t i = 0; i < buf->num_states; i++) {
        distances[i] -= min_distance;
    }
}

void history_buffer_traceback(history_buffer *buf, shift_register_t bestpath,
                              unsigned int min_traceback_length, bit_writer_t *output) {
    unsigned int fetched_index = 0;
    shift_register_t highbit = buf->highbit;
    unsigned int index = buf->index;
    unsigned int cap = buf->cap;
    for (unsigned int j = 0; j < min_traceback_length; j++) {
        if (index == 0) {
            index = cap - 1;
        } else {
            index--;
        }
        // we're walking backwards from what the work we did before
        // so, we'll shift high order bits in
        // the path will cross multiple different shift register states, and we determine
        //   which state by going backwards one time slice at a time
        uint8_t history = buf->history[index][bestpath];
        shift_register_t pathbit = history ? highbit : 0;
        bestpath |= pathbit;
        bestpath >>= 1;
    }
    unsigned int prefetch_index = index;
    if (prefetch_index == 0) {
        prefetch_index = cap - 1;
    } else {
        prefetch_index--;
    }
    unsigned int len = buf->len;
    for (unsigned int j = min_traceback_length; j < len; j++) {
        index = prefetch_index;
        if (prefetch_index == 0) {
            prefetch_index = cap - 1;
        } else {
            prefetch_index--;
        }
        prefetch(buf->history[prefetch_index]);
        // we're walking backwards from what the work we did before
        // so, we'll shift high order bits in
        // the path will cross multiple different shift register states, and we determine
        //   which state by going backwards one time slice at a time
        uint8_t history = buf->history[index][bestpath];
        shift_register_t pathbit = history ? highbit : 0;
        bestpath |= pathbit;
        bestpath >>= 1;
        buf->fetched[fetched_index] = (pathbit ? 1 : 0);
        fetched_index++;
    }
    bit_writer_write_bitlist_reversed(output, buf->fetched, fetched_index);
    buf->len -= fetched_index;
}

void history_buffer_process_skip(history_buffer *buf, distance_t *distances, bit_writer_t *output,
                                 unsigned int skip) {
    buf->index++;
    if (buf->index == buf->cap) {
        buf->index = 0;
    }

    buf->renormalize_counter++;
    buf->len++;

    // there are four ways these branches can resolve
    // a) we are neither renormalizing nor doing a traceback
    // b) we are renormalizing but not doing a traceback
    // c) we are renormalizing and doing a traceback
    // d) we are not renormalizing but we are doing a traceback
    // in case c, we want to save the effort of finding the bestpath
    //    since that's expensive
    // so we have to check for that case after we renormalize
    if (buf->renormalize_counter == buf->renormalize_interval) {
        buf->renormalize_counter = 0;
        shift_register_t bestpath = history_buffer_search(buf, distances, skip);
        history_buffer_renormalize(buf, distances, bestpath);
        if (buf->len == buf->cap) {
            // reuse the bestpath found for renormalizing
            history_buffer_traceback(buf, bestpath, buf->min_traceback_length, output);
        }
    } else if (buf->len == buf->cap) {
        // not renormalizing, find the bestpath here
        shift_register_t bestpath = history_buffer_search(buf, distances, skip);
        history_buffer_traceback(buf, bestpath, buf->min_traceback_length, output);
    }
}

void history_buffer_process(history_buffer *buf, distance_t *distances, bit_writer_t *output) {
    history_buffer_process_skip(buf, distances, output, 1);
}

void history_buffer_flush(history_buffer *buf, bit_writer_t *output) {
    history_buffer_traceback(buf, 0, 0, output);
}
