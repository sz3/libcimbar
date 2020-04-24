#include "correct/convolutional/convolutional.h"

size_t correct_convolutional_encode_len(correct_convolutional *conv, size_t msg_len) {
    size_t msgbits = 8 * msg_len;
    size_t encodedbits = conv->rate * (msgbits + conv->order + 1);
    return encodedbits;
}

// shift in most significant bit every time, one byte at a time
// shift register takes most recent bit on right, shifts left
// poly is written in same order, just & mask message w/ poly

// assume that encoded length is long enough?
size_t correct_convolutional_encode(correct_convolutional *conv,
                                    const uint8_t *msg,
                                    size_t msg_len,
                                    uint8_t *encoded) {
    // convolutional code convolves filter coefficients, given by
    //     the polynomial, with some history from our message.
    //     the history is stored as single subsequent bits in shiftregister
    shift_register_t shiftregister = 0;

    // shiftmask is the shiftregister bit mask that removes bits
    //      that extend beyond order
    // e.g. if order is 7, then remove the 8th bit and beyond
    unsigned int shiftmask = (1 << conv->order) - 1;

    size_t encoded_len_bits = correct_convolutional_encode_len(conv, msg_len);
    size_t encoded_len = (encoded_len_bits % 8) ? (encoded_len_bits / 8 + 1) : (encoded_len_bits / 8);
    bit_writer_reconfigure(conv->bit_writer, encoded, encoded_len);

    bit_reader_reconfigure(conv->bit_reader, msg, msg_len);

    for (size_t i = 0; i < 8 * msg_len; i++) {
        // shiftregister has oldest bits on left, newest on right
        shiftregister <<= 1;
        shiftregister |= bit_reader_read(conv->bit_reader, 1);
        shiftregister &= shiftmask;
        // shift most significant bit from byte and move down one bit at a time

        // we do direct lookup of our convolutional output here
        // all of the bits from this convolution are stored in this row
        unsigned int out = conv->table[shiftregister];
        bit_writer_write(conv->bit_writer, out, conv->rate);
    }

    // now flush the shiftregister
    // this is simply running the loop as above but without any new inputs
    // or rather, the new input string is all 0s
    for (size_t i = 0; i < conv->order + 1; i++) {
        shiftregister <<= 1;
        shiftregister &= shiftmask;
        unsigned int out = conv->table[shiftregister];
        bit_writer_write(conv->bit_writer, out, conv->rate);
    }

    // 0-fill any remaining bits on our final byte
    bit_writer_flush_byte(conv->bit_writer);

    return encoded_len_bits;
}
