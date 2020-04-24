#include "rs_tester_fec.h"
void rs_fec_encode(void *encoder, uint8_t *msg, size_t msg_length,
                   uint8_t *msg_out) {
    // XXX make sure that pad length used to build encoder corresponds to this
    // msg_length
    memcpy(msg_out, msg, msg_length);
    encode_rs_char(encoder, msg_out, msg_out + msg_length);
}

void rs_fec_decode(void *decoder, uint8_t *encoded, size_t encoded_length,
                   uint8_t *erasure_locations, size_t erasure_length,
                   uint8_t *msg, size_t pad_length, size_t num_roots) {
    // XXX make sure that pad length used to build decoder corresponds to this
    // encoded_length
    if (erasure_length) {
        static size_t locations_len = 0;
        static int *locations = NULL;
        if (locations_len < erasure_length) {
            locations = realloc(locations, erasure_length * sizeof(int));
            locations_len = erasure_length;
        }
        for (size_t i = 0; i < erasure_length; i++) {
            locations[i] = (unsigned int)(erasure_locations[i]) + pad_length;
        }
        decode_rs_char(decoder, encoded, locations, erasure_length);
    } else {
        decode_rs_char(decoder, encoded, NULL, 0);
    }
    memcpy(msg, encoded, encoded_length - num_roots);
}
