#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "correct.h"
#include "fec_shim.h"
#include "correct/util/error-sim-shim.h"

size_t max_block_len = 4096;

size_t test_conv(correct_convolutional *conv, void *fec,
                 ssize_t (*decode)(void *, uint8_t *, size_t, uint8_t *),
                 conv_testbench **testbench_ptr, size_t msg_len, double eb_n0,
                 double bpsk_bit_energy, double bpsk_voltage) {
    uint8_t *msg = malloc(max_block_len);

    size_t num_errors = 0;

    while (msg_len) {
        size_t block_len = (max_block_len < msg_len) ? max_block_len : msg_len;
        msg_len -= block_len;

        for (unsigned int j = 0; j < block_len; j++) {
            msg[j] = rand() % 256;
        }

        *testbench_ptr =
            resize_conv_testbench(*testbench_ptr, conv_correct_enclen, conv, block_len);
        conv_testbench *testbench = *testbench_ptr;
        testbench->encoder = conv;
        testbench->encode = conv_correct_encode;
        testbench->decoder = fec;
        testbench->decode = decode;
        build_white_noise(testbench->noise, testbench->enclen, eb_n0, bpsk_bit_energy);
        num_errors += test_conv_noise(testbench, msg, block_len, bpsk_voltage);
    }
    free(msg);
    return num_errors;
}

void assert_test_result(correct_convolutional *conv, void *fec,
                        ssize_t (*decode)(void *, uint8_t *, size_t, uint8_t *),
                        conv_testbench **testbench, size_t test_length, size_t rate, size_t order,
                        double eb_n0, double error_rate) {
    double bpsk_voltage = 1.0 / sqrt(2.0);
    double bpsk_sym_energy = 2 * pow(bpsk_voltage, 2.0);
    double bpsk_bit_energy = bpsk_sym_energy * rate;

    size_t error_count =
        test_conv(conv, fec, decode, testbench, test_length, eb_n0, bpsk_bit_energy, bpsk_voltage);
    double observed_error_rate = error_count / ((double)test_length * 8);
    if (observed_error_rate > error_rate) {
        printf(
            "test failed, expected error rate=%.2e, observed error rate=%.2e @%.1fdB for rate %zu "
            "order %zu\n",
            error_rate, observed_error_rate, eb_n0, rate, order);
        exit(1);
    } else {
        printf(
            "test passed, expected error rate=%.2e, observed error rate=%.2e @%.1fdB for rate %zu "
            "order %zu\n",
            error_rate, observed_error_rate, eb_n0, rate, order);
    }
}

int main() {
    srand(time(NULL));

    conv_testbench *testbench = NULL;

    correct_convolutional *conv;
    void *fec;
    uint16_t *poly;

    poly = (uint16_t[]){V27POLYA, V27POLYB};
    conv = correct_convolutional_create(2, 7, poly);
    fec = create_viterbi27(8 * max_block_len);
    assert_test_result(conv, fec, conv_shim27_decode, &testbench, 1000000, 2, 6, INFINITY, 0);
    assert_test_result(conv, fec, conv_shim27_decode, &testbench, 1000000, 2, 6, 4.5, 8e-06);
    assert_test_result(conv, fec, conv_shim27_decode, &testbench, 1000000, 2, 6, 4.0, 5e-05);
    delete_viterbi27(fec);
    correct_convolutional_destroy(conv);

    printf("\n");

    poly = (uint16_t[]){V29POLYA, V29POLYB};
    conv = correct_convolutional_create(2, 9, poly);
    fec = create_viterbi29(8 * max_block_len);
    assert_test_result(conv, fec, conv_shim29_decode, &testbench, 1000000, 2, 9, INFINITY, 0);
    assert_test_result(conv, fec, conv_shim29_decode, &testbench, 1000000, 2, 9, 4.5, 3e-06);
    assert_test_result(conv, fec, conv_shim29_decode, &testbench, 1000000, 2, 9, 4.0, 8e-06);
    delete_viterbi29(fec);
    correct_convolutional_destroy(conv);

    printf("\n");

    poly = (uint16_t[]){V39POLYA, V39POLYB, V39POLYC};
    conv = correct_convolutional_create(3, 9, poly);
    fec = create_viterbi39(8 * max_block_len);
    assert_test_result(conv, fec, conv_shim39_decode, &testbench, 1000000, 3, 9, INFINITY, 0);
    assert_test_result(conv, fec, conv_shim39_decode, &testbench, 1000000, 3, 9, 4.5, 3e-06);
    assert_test_result(conv, fec, conv_shim39_decode, &testbench, 1000000, 3, 9, 4.0, 9e-06);
    delete_viterbi39(fec);
    correct_convolutional_destroy(conv);

    printf("\n");

    poly = (uint16_t[]){V615POLYA, V615POLYB, V615POLYC, V615POLYD, V615POLYE, V615POLYF};
    conv = correct_convolutional_create(6, 15, poly);
    fec = create_viterbi615(8 * max_block_len);
    assert_test_result(conv, fec, conv_shim615_decode, &testbench, 100000, 6, 15, INFINITY, 0);
    assert_test_result(conv, fec, conv_shim615_decode, &testbench, 100000, 6, 15, 3.0, 2e-05);
    assert_test_result(conv, fec, conv_shim615_decode, &testbench, 100000, 6, 15, 2.5, 4e-05);
    delete_viterbi615(fec);
    correct_convolutional_destroy(conv);

    printf("\n");

    free_scratch(testbench);
    return 0;
}
