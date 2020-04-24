#include "correct/convolutional/convolutional.h"

void conv_decode_print_iter(correct_convolutional *conv, unsigned int iter,
                            unsigned int winner_index) {
    if (iter < 2220) {
        return;
    }
    printf("iteration: %d\n", iter);
    distance_t *errors = conv->errors->write_errors;
    printf("errors:\n");
    for (shift_register_t i = 0; i < conv->numstates / 2; i++) {
        printf("%2d: %d\n", i, errors[i]);
    }
    printf("\n");
    printf("history:\n");
    for (shift_register_t i = 0; i < conv->numstates / 2; i++) {
        printf("%2d: ", i);
        for (unsigned int j = 0; j <= winner_index; j++) {
            printf("%d", conv->history_buffer->history[j][i] ? 1 : 0);
        }
        printf("\n");
    }
    printf("\n");
}

void convolutional_decode_warmup(correct_convolutional *conv, unsigned int sets,
                                 const uint8_t *soft) {
    // first phase: load shiftregister up from 0 (order goes from 1 to conv->order)
    // we are building up error metrics for the first order bits
    for (unsigned int i = 0; i < conv->order - 1 && i < sets; i++) {
        // peel off rate bits from encoded to recover the same `out` as in the encoding process
        // the difference being that this `out` will have the channel noise/errors applied
        unsigned int out;
        if (!soft) {
            out = bit_reader_read(conv->bit_reader, conv->rate);
        }
        const distance_t *read_errors = conv->errors->read_errors;
        distance_t *write_errors = conv->errors->write_errors;
        // walk all of the state we have so far
        for (size_t j = 0; j < (1 << (i + 1)); j += 1) {
            unsigned int last = j >> 1;
            distance_t dist;
            if (soft) {
                if (conv->soft_measurement == CORRECT_SOFT_LINEAR) {
                    dist = metric_soft_distance_linear(conv->table[j], soft + i * conv->rate,
                                                       conv->rate);
                } else {
                    dist = metric_soft_distance_quadratic(conv->table[j], soft + i * conv->rate,
                                                          conv->rate);
                }
            } else {
                dist = metric_distance(conv->table[j], out);
            }
            write_errors[j] = dist + read_errors[last];
        }
        error_buffer_swap(conv->errors);
    }
}

void convolutional_decode_inner(correct_convolutional *conv, unsigned int sets,
                                const uint8_t *soft) {
    shift_register_t highbit = 1 << (conv->order - 1);
    for (unsigned int i = conv->order - 1; i < (sets - conv->order + 1); i++) {
        distance_t *distances = conv->distances;
        // lasterrors are the aggregate bit errors for the states of shiftregister for the previous
        // time slice
        if (soft) {
            if (conv->soft_measurement == CORRECT_SOFT_LINEAR) {
                for (unsigned int j = 0; j < 1 << (conv->rate); j++) {
                    distances[j] =
                        metric_soft_distance_linear(j, soft + i * conv->rate, conv->rate);
                }
            } else {
                for (unsigned int j = 0; j < 1 << (conv->rate); j++) {
                    distances[j] =
                        metric_soft_distance_quadratic(j, soft + i * conv->rate, conv->rate);
                }
            }
        } else {
            unsigned int out = bit_reader_read(conv->bit_reader, conv->rate);
            for (unsigned int i = 0; i < 1 << (conv->rate); i++) {
                distances[i] = metric_distance(i, out);
            }
        }
        pair_lookup_t pair_lookup = conv->pair_lookup;
        pair_lookup_fill_distance(pair_lookup, distances);

        // a mask to get the high order bit from the shift register
        unsigned int num_iter = highbit << 1;
        const distance_t *read_errors = conv->errors->read_errors;
        // aggregate bit errors for this time slice
        distance_t *write_errors = conv->errors->write_errors;

        uint8_t *history = history_buffer_get_slice(conv->history_buffer);
        // walk through all states, ignoring oldest bit
        // we will track a best register state (path) and the number of bit errors at that path at
        // this time slice
        // this loop considers two paths per iteration (high order bit set, clear)
        // so, it only runs numstates/2 iterations
        // we'll update the history for every state and find the path with the least aggregated bit
        // errors

        // now run the main loop
        // we calculate 2 sets of 2 register states here (4 states per iter)
        // this creates 2 sets which share a predecessor, and 2 sets which share a successor
        //
        // the first set definition is the two states that are the same except for the least order
        // bit
        // these two share a predecessor because their high n - 1 bits are the same (differ only by
        // newest bit)
        //
        // the second set definition is the two states that are the same except for the high order
        // bit
        // these two share a successor because the oldest high order bit will be shifted out, and
        // the other bits will be present in the successor
        //
        shift_register_t highbase = highbit >> 1;
        for (shift_register_t low = 0, high = highbit, base = 0; high < num_iter;
             low += 8, high += 8, base += 4) {
            // shifted-right ancestors
            // low and low_plus_one share low_past_error
            //   note that they are the same when shifted right by 1
            // same goes for high and high_plus_one
            for (shift_register_t offset = 0, base_offset = 0; base_offset < 4;
                 offset += 2, base_offset += 1) {
                distance_pair_key_t low_key = pair_lookup.keys[base + base_offset];
                distance_pair_key_t high_key = pair_lookup.keys[highbase + base + base_offset];
                distance_pair_t low_concat_dist = pair_lookup.distances[low_key];
                distance_pair_t high_concat_dist = pair_lookup.distances[high_key];

                distance_t low_past_error = read_errors[base + base_offset];
                distance_t high_past_error = read_errors[highbase + base + base_offset];

                distance_t low_error = (low_concat_dist & 0xffff) + low_past_error;
                distance_t high_error = (high_concat_dist & 0xffff) + high_past_error;

                shift_register_t successor = low + offset;
                distance_t error;
                uint8_t history_mask;
                if (low_error <= high_error) {
                    error = low_error;
                    history_mask = 0;
                } else {
                    error = high_error;
                    history_mask = 1;
                }
                write_errors[successor] = error;
                history[successor] = history_mask;

                shift_register_t low_plus_one = low + offset + 1;

                distance_t low_plus_one_error = (low_concat_dist >> 16) + low_past_error;
                distance_t high_plus_one_error = (high_concat_dist >> 16) + high_past_error;

                shift_register_t plus_one_successor = low_plus_one;
                distance_t plus_one_error;
                uint8_t plus_one_history_mask;
                if (low_plus_one_error <= high_plus_one_error) {
                    plus_one_error = low_plus_one_error;
                    plus_one_history_mask = 0;
                } else {
                    plus_one_error = high_plus_one_error;
                    plus_one_history_mask = 1;
                }
                write_errors[plus_one_successor] = plus_one_error;
                history[plus_one_successor] = plus_one_history_mask;
            }
        }

        history_buffer_process(conv->history_buffer, write_errors, conv->bit_writer);
        error_buffer_swap(conv->errors);
    }
}

void convolutional_decode_tail(correct_convolutional *conv, unsigned int sets,
                               const uint8_t *soft) {
    // flush state registers
    // now we only shift in 0s, skipping 1-successors
    shift_register_t highbit = 1 << (conv->order - 1);
    for (unsigned int i = sets - conv->order + 1; i < sets; i++) {
        // lasterrors are the aggregate bit errors for the states of shiftregister for the previous
        // time slice
        const distance_t *read_errors = conv->errors->read_errors;
        // aggregate bit errors for this time slice
        distance_t *write_errors = conv->errors->write_errors;

        uint8_t *history = history_buffer_get_slice(conv->history_buffer);

        // calculate the distance from all output states to our sliced bits
        distance_t *distances = conv->distances;
        if (soft) {
            if (conv->soft_measurement == CORRECT_SOFT_LINEAR) {
                for (unsigned int j = 0; j < 1 << (conv->rate); j++) {
                    distances[j] =
                        metric_soft_distance_linear(j, soft + i * conv->rate, conv->rate);
                }
            } else {
                for (unsigned int j = 0; j < 1 << (conv->rate); j++) {
                    distances[j] =
                        metric_soft_distance_quadratic(j, soft + i * conv->rate, conv->rate);
                }
            }
        } else {
            unsigned int out = bit_reader_read(conv->bit_reader, conv->rate);
            for (unsigned int i = 0; i < 1 << (conv->rate); i++) {
                distances[i] = metric_distance(i, out);
            }
        }
        const unsigned int *table = conv->table;

        // a mask to get the high order bit from the shift register
        unsigned int num_iter = highbit << 1;
        unsigned int skip = 1 << (conv->order - (sets - i));
        unsigned int base_skip = skip >> 1;

        shift_register_t highbase = highbit >> 1;
        for (shift_register_t low = 0, high = highbit, base = 0; high < num_iter;
             low += skip, high += skip, base += base_skip) {
            unsigned int low_output = table[low];
            unsigned int high_output = table[high];
            distance_t low_dist = distances[low_output];
            distance_t high_dist = distances[high_output];

            distance_t low_past_error = read_errors[base];
            distance_t high_past_error = read_errors[highbase + base];

            distance_t low_error = low_dist + low_past_error;
            distance_t high_error = high_dist + high_past_error;

            shift_register_t successor = low;
            distance_t error;
            uint8_t history_mask;
            if (low_error < high_error) {
                error = low_error;
                history_mask = 0;
            } else {
                error = high_error;
                history_mask = 1;
            }
            write_errors[successor] = error;
            history[successor] = history_mask;
        }

        history_buffer_process_skip(conv->history_buffer, write_errors, conv->bit_writer, skip);
        error_buffer_swap(conv->errors);
    }
}

void _convolutional_decode_init(correct_convolutional *conv, unsigned int min_traceback,
                                unsigned int traceback_length, unsigned int renormalize_interval) {
    conv->has_init_decode = true;

    conv->distances = calloc(1 << (conv->rate), sizeof(distance_t));
    conv->pair_lookup = pair_lookup_create(conv->rate, conv->order, conv->table);

    conv->soft_measurement = CORRECT_SOFT_LINEAR;

    // we limit history to go back as far as 5 * the order of our polynomial
    conv->history_buffer = history_buffer_create(min_traceback, traceback_length, renormalize_interval,
                                                 conv->numstates / 2, 1 << (conv->order - 1));

    conv->errors = error_buffer_create(conv->numstates);
}

static ssize_t _convolutional_decode(correct_convolutional *conv, size_t num_encoded_bits,
                                     size_t num_encoded_bytes, uint8_t *msg,
                                     const soft_t *soft_encoded) {
    if (!conv->has_init_decode) {
        uint64_t max_error_per_input = conv->rate * soft_max;
        unsigned int renormalize_interval = distance_max / max_error_per_input;
        _convolutional_decode_init(conv, 5 * conv->order, 15 * conv->order, renormalize_interval);
    }

    size_t sets = num_encoded_bits / conv->rate;
    // XXX fix this vvvvvv
    size_t decoded_len_bytes = num_encoded_bytes;
    bit_writer_reconfigure(conv->bit_writer, msg, decoded_len_bytes);

    error_buffer_reset(conv->errors);
    history_buffer_reset(conv->history_buffer);

    // no outputs are generated during warmup
    convolutional_decode_warmup(conv, sets, soft_encoded);
    convolutional_decode_inner(conv, sets, soft_encoded);
    convolutional_decode_tail(conv, sets, soft_encoded);

    history_buffer_flush(conv->history_buffer, conv->bit_writer);

    return bit_writer_length(conv->bit_writer);
}

// perform viterbi decoding
// hard decoder
ssize_t correct_convolutional_decode(correct_convolutional *conv, const uint8_t *encoded,
                                     size_t num_encoded_bits, uint8_t *msg) {
    if (num_encoded_bits % conv->rate) {
        // XXX turn this into an error code
        // printf("encoded length of message must be a multiple of rate\n");
        return -1;
    }

    size_t num_encoded_bytes =
        (num_encoded_bits % 8) ? (num_encoded_bits / 8 + 1) : (num_encoded_bits / 8);
    bit_reader_reconfigure(conv->bit_reader, encoded, num_encoded_bytes);

    return _convolutional_decode(conv, num_encoded_bits, num_encoded_bytes, msg, NULL);
}

ssize_t correct_convolutional_decode_soft(correct_convolutional *conv, const soft_t *encoded,
                                          size_t num_encoded_bits, uint8_t *msg) {
    if (num_encoded_bits % conv->rate) {
        // XXX turn this into an error code
        // printf("encoded length of message must be a multiple of rate\n");
        return -1;
    }

    size_t num_encoded_bytes =
        (num_encoded_bits % 8) ? (num_encoded_bits / 8 + 1) : (num_encoded_bits / 8);

    return _convolutional_decode(conv, num_encoded_bits, num_encoded_bytes, msg, encoded);
}
