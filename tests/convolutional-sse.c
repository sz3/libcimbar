#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "correct/util/error-sim-sse.h"

size_t max_block_len = 4096;

size_t test_conv(correct_convolutional_sse *conv, conv_testbench **testbench_ptr,
               size_t msg_len, double eb_n0, double bpsk_bit_energy,
               double bpsk_voltage) {
    uint8_t *msg = malloc(max_block_len);

    size_t num_errors = 0;

    while (msg_len) {
        size_t block_len = (max_block_len < msg_len) ? max_block_len : msg_len;
        msg_len -= block_len;

        for (unsigned int j = 0; j < block_len; j++) {
            msg[j] = rand() % 256;
        }

        *testbench_ptr = resize_conv_testbench(*testbench_ptr, conv_correct_sse_enclen, conv, block_len);
        conv_testbench *testbench = *testbench_ptr;
        testbench->encoder = conv;
        testbench->encode = conv_correct_sse_encode;
        testbench->decoder = conv;
        testbench->decode = conv_correct_sse_decode;
        build_white_noise(testbench->noise, testbench->enclen, eb_n0, bpsk_bit_energy);
        num_errors += test_conv_noise(testbench, msg, block_len, bpsk_voltage);
    }
    free(msg);
    return num_errors;
}

void assert_test_result(correct_convolutional_sse *conv, conv_testbench **testbench,
                        size_t test_length, size_t rate, size_t order, double eb_n0, double error_rate) {
    double bpsk_voltage = 1.0/sqrt(2.0);
    double bpsk_sym_energy = 2*pow(bpsk_voltage, 2.0);
    double bpsk_bit_energy = bpsk_sym_energy * rate;

    size_t error_count = test_conv(conv, testbench, test_length, eb_n0, bpsk_bit_energy, bpsk_voltage);
    double observed_error_rate = error_count/((double)test_length * 8);
    if (observed_error_rate > error_rate) {
        printf("test failed, expected error rate=%.2e, observed error rate=%.2e @%.1fdB for rate %zu order %zu\n",
                error_rate, observed_error_rate, eb_n0, rate, order);
        exit(1);
    } else {
        printf("test passed, expected error rate=%.2e, observed error rate=%.2e @%.1fdB for rate %zu order %zu\n",
                error_rate, observed_error_rate, eb_n0, rate, order);
    }
}

int main() {
    srand(time(NULL));

    conv_testbench *testbench = NULL;

    correct_convolutional_sse *conv;

    // n.b. the error rates below are at 5.0dB/4.5dB for order 6 polys
    //  and 4.5dB/4.0dB for order 7-9 polys. this can be easy to miss.

    conv = correct_convolutional_sse_create(2, 6, correct_conv_r12_6_polynomial);
    assert_test_result(conv, &testbench, 1000000, 2, 6, INFINITY, 0);
    assert_test_result(conv, &testbench, 1000000, 2, 6, 5.0, 8e-06);
    assert_test_result(conv, &testbench, 1000000, 2, 6, 4.5, 3e-05);
    correct_convolutional_sse_destroy(conv);

    printf("\n");

    conv = correct_convolutional_sse_create(2, 7, correct_conv_r12_7_polynomial);
    assert_test_result(conv, &testbench, 1000000, 2, 7, INFINITY, 0);
    assert_test_result(conv, &testbench, 1000000, 2, 7, 4.5, 1e-05);
    assert_test_result(conv, &testbench, 1000000, 2, 7, 4.0, 5e-05);
    correct_convolutional_sse_destroy(conv);

    printf("\n");

    conv = correct_convolutional_sse_create(2, 8, correct_conv_r12_8_polynomial);
    assert_test_result(conv, &testbench, 1000000, 2, 8, INFINITY, 0);
    assert_test_result(conv, &testbench, 1000000, 2, 8, 4.5, 5e-06);
    assert_test_result(conv, &testbench, 1000000, 2, 8, 4.0, 3e-05);
    correct_convolutional_sse_destroy(conv);

    printf("\n");

    conv = correct_convolutional_sse_create(2, 9, correct_conv_r12_9_polynomial);
    assert_test_result(conv, &testbench, 1000000, 2, 9, INFINITY, 0);
    assert_test_result(conv, &testbench, 1000000, 2, 9, 4.5, 3e-06);
    assert_test_result(conv, &testbench, 1000000, 2, 9, 4.0, 8e-06);
    correct_convolutional_sse_destroy(conv);

    printf("\n");

    conv = correct_convolutional_sse_create(3, 6, correct_conv_r13_6_polynomial);
    assert_test_result(conv, &testbench, 1000000, 3, 6, INFINITY, 0);
    assert_test_result(conv, &testbench, 1000000, 3, 6, 5.0, 5e-06);
    assert_test_result(conv, &testbench, 1000000, 3, 6, 4.5, 2e-05);
    correct_convolutional_sse_destroy(conv);

    printf("\n");

    conv = correct_convolutional_sse_create(3, 7, correct_conv_r13_7_polynomial);
    assert_test_result(conv, &testbench, 1000000, 3, 7, INFINITY, 0);
    assert_test_result(conv, &testbench, 1000000, 3, 7, 4.5, 5e-06);
    assert_test_result(conv, &testbench, 1000000, 3, 7, 4.0, 3e-05);
    correct_convolutional_sse_destroy(conv);

    printf("\n");

    conv = correct_convolutional_sse_create(3, 8, correct_conv_r13_8_polynomial);
    assert_test_result(conv, &testbench, 1000000, 3, 8, INFINITY, 0);
    assert_test_result(conv, &testbench, 1000000, 3, 8, 4.5, 4e-06);
    assert_test_result(conv, &testbench, 1000000, 3, 8, 4.0, 1e-05);
    correct_convolutional_sse_destroy(conv);

    printf("\n");

    conv = correct_convolutional_sse_create(3, 9, correct_conv_r13_9_polynomial);
    assert_test_result(conv, &testbench, 1000000, 3, 9, INFINITY, 0);
    assert_test_result(conv, &testbench, 1000000, 3, 9, 4.5, 3e-06);
    assert_test_result(conv, &testbench, 1000000, 3, 9, 4.0, 5e-06);
    correct_convolutional_sse_destroy(conv);

    printf("\n");

    free_scratch(testbench);
    return 0;
}
