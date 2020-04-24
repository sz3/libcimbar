#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "rs_tester.h"
#include "rs_tester_fec_shim.h"

void print_test_type(size_t block_length, size_t message_length,
                     size_t num_errors, size_t num_erasures) {
    printf(
        "testing reed solomon block length=%zu, message length=%zu, "
        "errors=%zu, erasures=%zu...",
        block_length, message_length, num_errors, num_erasures);
}

void fail_test() {
    printf("FAILED\n");
    exit(1);
}

void pass_test() { printf("PASSED\n"); }

void run_tests(correct_reed_solomon *rs, void *fec_rs, rs_testbench *testbench,
               size_t block_length, size_t test_msg_length, size_t num_errors,
               size_t num_erasures, size_t num_iterations) {
    // run both ways, correct->fec and fec->correct
    rs_test test;
    test.encode = rs_correct_encode;
    test.encoder = rs;
    test.decode = rs_fec_decode;
    test.decoder = fec_rs;

    print_test_type(block_length, test_msg_length, num_errors, num_erasures);
    for (size_t i = 0; i < num_iterations; i++) {
        rs_test_run run = test_rs_errors(&test, testbench, test_msg_length, num_errors,
                                     num_erasures);
        if (!run.output_matches) {
            fail_test();
        }
    }

    test.encode = rs_fec_encode;
    test.encoder = fec_rs;
    test.decode = rs_correct_decode;
    test.decoder = rs;
    for (size_t i = 0; i < num_iterations; i++) {
        rs_test_run run = test_rs_errors(&test, testbench, test_msg_length, num_errors,
                                     num_erasures);
        if (!run.output_matches) {
            fail_test();
        }
    }
    pass_test();
}

int main() {
    srand(time(NULL));

    size_t block_length = 255;
    size_t min_distance = 32;
    size_t message_length = block_length - min_distance;

    size_t pad_length;
    void *fec_rs;

    correct_reed_solomon *rs = correct_reed_solomon_create(
        correct_rs_primitive_polynomial_ccsds, 1, 1, min_distance);
    rs_testbench *testbench = rs_testbench_create(block_length, min_distance);

    pad_length = message_length / 2;
    fec_rs = init_rs_char(8, correct_rs_primitive_polynomial_ccsds, 1, 1, min_distance,
                          pad_length);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              0, 0, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              min_distance / 2, 0, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              0, min_distance, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              min_distance / 4, min_distance / 2, 20000);
    free_rs_char(fec_rs);

    pad_length = 0;
    fec_rs = init_rs_char(8, correct_rs_primitive_polynomial_ccsds, 1, 1, min_distance,
                          pad_length);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              0, 0, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              min_distance / 2, 0, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              0, min_distance, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              min_distance / 4, min_distance / 2, 20000);
    free_rs_char(fec_rs);

    rs_testbench_destroy(testbench);
    correct_reed_solomon_destroy(rs);

    min_distance = 16;
    message_length = block_length - min_distance;
    rs = correct_reed_solomon_create(
        correct_rs_primitive_polynomial_ccsds, 1, 1, min_distance);
    testbench = rs_testbench_create(block_length, min_distance);

    pad_length = message_length / 2;
    fec_rs = init_rs_char(8, correct_rs_primitive_polynomial_ccsds, 1, 1, min_distance,
                          pad_length);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              0, 0, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              min_distance / 2, 0, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              0, min_distance, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              min_distance / 4, min_distance / 2, 20000);
    free_rs_char(fec_rs);

    pad_length = 0;
    fec_rs = init_rs_char(8, correct_rs_primitive_polynomial_ccsds, 1, 1, min_distance,
                          pad_length);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              0, 0, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              min_distance / 2, 0, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              0, min_distance, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              min_distance / 4, min_distance / 2, 20000);
    free_rs_char(fec_rs);

    rs_testbench_destroy(testbench);
    correct_reed_solomon_destroy(rs);

    printf("test passed\n");
    return 0;
}
