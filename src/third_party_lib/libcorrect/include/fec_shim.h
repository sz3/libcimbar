#ifndef CORRECT_FEC_H
#define CORRECT_FEC_H
// libcorrect's libfec shim header
// this is a partial implementation of libfec
// header signatures derived from found usages of libfec -- some things may be different
#include <correct.h>

// Reed-Solomon
void *init_rs_char(int symbol_size, int primitive_polynomial, int first_consecutive_root,
                   int root_gap, int number_roots, unsigned int pad);
void free_rs_char(void *rs);
void encode_rs_char(void *rs, const unsigned char *msg, unsigned char *parity);
void decode_rs_char(void *rs, unsigned char *block, int *erasure_locations, int num_erasures);

// Convolutional Codes

// Polynomials
// These have been determined via find_conv_libfec_poly.c
// We could just make up new ones, but we use libfec's here so that
//   codes encoded by this library can be decoded by the original libfec
//   and vice-versa
#define V27POLYA 0155
#define V27POLYB 0117

#define V29POLYA 0657
#define V29POLYB 0435

#define V39POLYA 0755
#define V39POLYB 0633
#define V39POLYC 0447

#define V615POLYA 042631
#define V615POLYB 047245
#define V615POLYC 056507
#define V615POLYD 073363
#define V615POLYE 077267
#define V615POLYF 064537

// Convolutional Methods
void *create_viterbi27(int num_decoded_bits);
int init_viterbi27(void *vit, int _mystery);
int update_viterbi27_blk(void *vit, unsigned char *encoded_soft, int n_encoded_groups);
int chainback_viterbi27(void *vit, unsigned char *decoded, unsigned int n_decoded_bits, unsigned int _mystery);
void delete_viterbi27(void *vit);

void *create_viterbi29(int num_decoded_bits);
int init_viterbi29(void *vit, int _mystery);
int update_viterbi29_blk(void *vit, unsigned char *encoded_soft, int n_encoded_groups);
int chainback_viterbi29(void *vit, unsigned char *decoded, unsigned int n_decoded_bits, unsigned int _mystery);
void delete_viterbi29(void *vit);

void *create_viterbi39(int num_decoded_bits);
int init_viterbi39(void *vit, int _mystery);
int update_viterbi39_blk(void *vit, unsigned char *encoded_soft, int n_encoded_groups);
int chainback_viterbi39(void *vit, unsigned char *decoded, unsigned int n_decoded_bits, unsigned int _mystery);
void delete_viterbi39(void *vit);

void *create_viterbi615(int num_decoded_bits);
int init_viterbi615(void *vit, int _mystery);
int update_viterbi615_blk(void *vit, unsigned char *encoded_soft, int n_encoded_groups);
int chainback_viterbi615(void *vit, unsigned char *decoded, unsigned int n_decoded_bits, unsigned int _mystery);
void delete_viterbi615(void *vit);

// Misc other
static inline int parity(unsigned int x) {
    /* http://graphics.stanford.edu/~seander/bithacks.html#ParityParallel */
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x &= 0xf;
    return (0x6996 >> x) & 1;
}

#endif
