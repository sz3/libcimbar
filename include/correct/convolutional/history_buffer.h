#include "correct/convolutional.h"
#include "correct/convolutional/bit.h"

// ring buffer of path histories
// generates output bits after accumulating sufficient history
typedef struct {
    // history entries must be at least this old to be decoded
    const unsigned int min_traceback_length;
    // we'll decode entries in bursts. this tells us the length of the burst
    const unsigned int traceback_group_length;
    // we will store a total of cap entries. equal to min_traceback_length +
    // traceback_group_length
    const unsigned int cap;

    // how many states in the shift register? this is one of the dimensions of
    // history table
    const unsigned int num_states;
    // what's the high order bit of the shift register?
    const shift_register_t highbit;

    // history is a compact history representation for every shift register
    // state,
    //    one bit per time slice
    uint8_t **history;

    // which slice are we writing next?
    unsigned int index;

    // how many valid entries are there?
    unsigned int len;

    // temporary store of fetched bits
    uint8_t *fetched;

    // how often should we renormalize?
    unsigned int renormalize_interval;
    unsigned int renormalize_counter;
} history_buffer;

history_buffer *history_buffer_create(unsigned int min_traceback_length,
                                      unsigned int traceback_group_length,
                                      unsigned int renormalize_interval,
                                      unsigned int num_states,
                                      shift_register_t highbit);
void history_buffer_destroy(history_buffer *buf);
void history_buffer_reset(history_buffer *buf);
void history_buffer_step(history_buffer *buf);
uint8_t *history_buffer_get_slice(history_buffer *buf);
shift_register_t history_buffer_search(history_buffer *buf,
                                       const distance_t *distances,
                                       unsigned int search_every);
void history_buffer_traceback(history_buffer *buf, shift_register_t bestpath,
                              unsigned int min_traceback_length,
                              bit_writer_t *output);
void history_buffer_process_skip(history_buffer *buf, distance_t *distances,
                                 bit_writer_t *output, unsigned int skip);
void history_buffer_process(history_buffer *buf, distance_t *distances,
                            bit_writer_t *output);
void history_buffer_flush(history_buffer *buf, bit_writer_t *output);
