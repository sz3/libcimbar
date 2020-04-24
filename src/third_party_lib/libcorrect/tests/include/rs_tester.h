#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "correct.h"

void rs_correct_encode(void *encoder, uint8_t *msg, size_t msg_length,
                       uint8_t *msg_out);
void rs_correct_decode(void *decoder, uint8_t *encoded, size_t encoded_length,
                       uint8_t *erasure_locations, size_t erasure_length,
                       uint8_t *msg, size_t pad_length, size_t num_roots);

typedef struct {
    size_t block_length;
    size_t message_length;
    size_t min_distance;
    unsigned char *msg;
    uint8_t *encoded;
    int *indices;
    uint8_t *corrupted_encoded;
    uint8_t *erasure_locations;
    unsigned char *recvmsg;
} rs_testbench;

typedef struct {
    void (*encode)(void *, uint8_t *, size_t, uint8_t *);
    void *encoder;
    void (*decode)(void *, uint8_t *, size_t, uint8_t *, size_t, uint8_t *, size_t, size_t);
    void *decoder;
} rs_test;

rs_testbench *rs_testbench_create(size_t block_length, size_t min_distance);
void rs_testbench_destroy(rs_testbench *testbench);

typedef struct {
    bool output_matches;
} rs_test_run;

rs_test_run test_rs_errors(rs_test *test, rs_testbench *testbench, size_t msg_length,
                    size_t num_errors, size_t num_erasures);
