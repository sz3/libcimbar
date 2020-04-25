#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <fec.h>
void rs_fec_encode(void *encoder, uint8_t *msg, size_t msg_length,
                   uint8_t *msg_out);
void rs_fec_decode(void *decoder, uint8_t *encoded, size_t encoded_length,
                   uint8_t *erasure_locations, size_t erasure_length,
                   uint8_t *msg, size_t pad_length, size_t num_roots);
