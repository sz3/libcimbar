#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <stdio.h>

#include "correct.h"
#include "correct/portable.h"

size_t distance(uint8_t *a, uint8_t *b, size_t len);
void gaussian(double *res, size_t n_res, double sigma);

void encode_bpsk(uint8_t *msg, double *voltages, size_t n_syms, double bpsk_voltage);
void byte2bit(uint8_t *bytes, uint8_t *bits, size_t n_bits);
void decode_bpsk(uint8_t *soft, uint8_t *msg, size_t n_syms);
void decode_bpsk_soft(double *voltages, uint8_t *soft, size_t n_syms, double bpsk_voltage);
double log2amp(double l);
double amp2log(double a);
double sigma_for_eb_n0(double eb_n0, double bpsk_bit_energy);
void build_white_noise(double *noise, size_t n_syms, double eb_n0, double bpsk_bit_energy);
void add_white_noise(double *signal, double *noise, size_t n_syms);

typedef struct {
    uint8_t *msg_out;
    size_t msg_len;
    uint8_t *encoded;
    double *v;
    double *corrupted;
    uint8_t *soft;
    double *noise;
    size_t enclen;
    size_t enclen_bytes;
    void (*encode)(void *, uint8_t *msg, size_t msg_len, uint8_t *encoded);
    void *encoder;
    ssize_t (*decode)(void *, uint8_t *soft, size_t soft_len, uint8_t *msg);
    void *decoder;
} conv_testbench;

conv_testbench *resize_conv_testbench(conv_testbench *scratch, size_t (*enclen)(void *, size_t), void *enc, size_t msg_len);
void free_scratch(conv_testbench *scratch);
int test_conv_noise(conv_testbench *scratch, uint8_t *msg, size_t n_bytes,
                    double bpsk_voltage);

size_t conv_correct_enclen(void *conv_v, size_t msg_len);
void conv_correct_encode(void *conv_v, uint8_t *msg, size_t msg_len, uint8_t *encoded);
ssize_t conv_correct_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg);
