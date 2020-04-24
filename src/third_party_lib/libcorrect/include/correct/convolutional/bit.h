#ifndef CORRECT_CONVOLUTIONAL_BIT
#define CORRECT_CONVOLUTIONAL_BIT
#include "correct/convolutional.h"

typedef struct {
    uint8_t current_byte;
    unsigned int current_byte_len;
    uint8_t *bytes;
    size_t byte_index;
    size_t len;
} bit_writer_t;

bit_writer_t *bit_writer_create(uint8_t *bytes, size_t len);

void bit_writer_reconfigure(bit_writer_t *w, uint8_t *bytes, size_t len);

void bit_writer_destroy(bit_writer_t *w);

void bit_writer_write(bit_writer_t *w, uint8_t val, unsigned int n);

void bit_writer_write_1(bit_writer_t *w, uint8_t val);

void bit_writer_write_bitlist_reversed(bit_writer_t *w, uint8_t *l, size_t len);

void bit_writer_flush_byte(bit_writer_t *w);

size_t bit_writer_length(bit_writer_t *w);

typedef struct {
    uint8_t current_byte;
    size_t byte_index;
    size_t len;
    size_t current_byte_len;
    const uint8_t *bytes;
} bit_reader_t;

bit_reader_t *bit_reader_create(const uint8_t *bytes, size_t len);

void bit_reader_reconfigure(bit_reader_t *r, const uint8_t *bytes, size_t len);

void bit_reader_destroy(bit_reader_t *r);

uint8_t bit_reader_read(bit_reader_t *r, unsigned int n);
#endif
