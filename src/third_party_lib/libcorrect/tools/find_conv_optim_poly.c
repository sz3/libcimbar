#include <stdbool.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <limits.h>
#include <pthread.h>

#if HAVE_SSE
#include "correct/util/error-sim-sse.h"
typedef correct_convolutional_sse conv_t;
static conv_t*(*conv_create)(size_t, size_t, const uint16_t *) = correct_convolutional_sse_create;
static void(*conv_destroy)(conv_t *) = correct_convolutional_sse_destroy;
static size_t(*conv_enclen)(void *, size_t) = conv_correct_sse_enclen;
static void(*conv_encode)(void *, uint8_t *, size_t, uint8_t *) = conv_correct_sse_encode;
static void(*conv_decode)(void *, uint8_t *, size_t, uint8_t *) = conv_correct_sse_decode;
#else
#include "correct/util/error-sim.h"
typedef correct_convolutional conv_t;
static conv_t*(*conv_create)(size_t, size_t, const uint16_t *) = correct_convolutional_create;
static void(*conv_destroy)(conv_t *) = correct_convolutional_destroy;
static size_t(*conv_enclen)(void *, size_t) = conv_correct_enclen;
static void(*conv_encode)(void *, uint8_t *, size_t, uint8_t *) = conv_correct_encode;
static void(*conv_decode)(void *, uint8_t *, size_t, uint8_t *) = conv_correct_decode;
#endif

typedef struct {
    conv_t *conv;
    correct_convolutional_polynomial_t *poly;
} conv_tester_t;

typedef struct {
    int *distances;
    float cost;
    correct_convolutional_polynomial_t *poly;
} conv_result_t;

int compare_conv_results(const void *avoid, const void *bvoid) {
    const conv_result_t *a = (const conv_result_t *)avoid;
    const conv_result_t *b = (const conv_result_t *)bvoid;

    if (a->cost > b->cost) {
        return 1;
    }
    return -1;
}

typedef struct {
    size_t rate;
    size_t order;
    conv_result_t *items;
    size_t items_len;
    conv_testbench *scratch;
    uint8_t *msg;
    size_t msg_len;
    size_t test_offset;
    double bpsk_voltage;
} exhaustive_thread_args;

void *search_exhaustive_thread(void *vargs) {
    exhaustive_thread_args *args = (exhaustive_thread_args *)vargs;
    conv_t *conv;
    for (size_t i = 0; i < args->items_len; i++) {
        conv = conv_create(args->rate, args->order, args->items[i].poly);
        args->scratch->encode = conv_encode;
        args->scratch->encoder = conv;
        args->scratch->decode = conv_decode;
        args->scratch->decoder = conv;
        args->items[i].distances[args->test_offset] += test_conv_noise(args->scratch, args->msg, args->msg_len, args->bpsk_voltage);
        conv_destroy(conv);
    }
    pthread_exit(NULL);
}

void search_exhaustive(size_t rate, size_t order,
                       size_t n_bytes, uint8_t *msg,
                       conv_testbench **scratches, size_t num_scratches,
                       float *weights,
                       conv_result_t *items,
                       size_t items_len, double bpsk_voltage) {

    exhaustive_thread_args *args = malloc(num_scratches * sizeof(exhaustive_thread_args));
    pthread_t *threads = malloc(num_scratches * sizeof(pthread_t));

    for (size_t i = 0; i < num_scratches; i++) {
        args[i].rate = rate;
        args[i].order = order;
        args[i].items = items;
        args[i].items_len = items_len;
        args[i].scratch = scratches[i];
        args[i].msg = msg;
        args[i].msg_len = n_bytes;
        args[i].test_offset = i;
        args[i].bpsk_voltage = bpsk_voltage;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&threads[i], &attr, search_exhaustive_thread, &args[i]);
    }

    for (size_t i = 0; i < num_scratches; i++) {
        pthread_join(threads[i], NULL);
    }

    free(args);
    free(threads);

}

void search_exhaustive_init(conv_result_t *items, size_t items_len,
                            size_t num_scratches) {
    for (size_t i = 0; i < items_len; i++) {
        for (size_t j = 0; j < num_scratches; j++) {
            items[i].distances[j] = 0;
        }
    }
}

void search_exhaustive_fin(conv_result_t *items, size_t items_len,
                           float *weights, size_t weights_len) {
    for (size_t i = 0; i < items_len; i++) {
        items[i].cost = 0;
        for (size_t j = 0; j < weights_len; j++) {
            items[i].cost += weights[j] * items[i].distances[j];
        }
    }

    qsort(items, items_len, sizeof(conv_result_t), compare_conv_results);
}

const size_t max_block_len = 16384;
const size_t max_msg_len = 50000000;

void test(size_t rate, size_t order,
          conv_tester_t start, conv_testbench **scratches,
          size_t num_scratches, float *weights,
          size_t n_bytes, double *eb_n0,
          double bpsk_bit_energy, size_t n_iter,
          double bpsk_voltage) {

    uint8_t *msg = malloc(max_block_len * sizeof(uint8_t));

    correct_convolutional_polynomial_t maxcoeff = (1 << order) - 1;
    correct_convolutional_polynomial_t startcoeff = (1 << (order - 1)) + 1;
    size_t num_polys = (maxcoeff - startcoeff) / 2 + 1;
    size_t convs_len = 1;
    for (size_t i = 0; i < rate; i++) {
        convs_len *= num_polys;
    }

    conv_result_t *exhaustive = malloc(convs_len * sizeof(conv_result_t));
    correct_convolutional_polynomial_t *iter_poly = malloc(rate * sizeof(correct_convolutional_polynomial_t));

    for (size_t i = 0; i < rate; i++) {
        iter_poly[i] = startcoeff;
    }

    // init exhaustive with all polys
    for (size_t i = 0; i < convs_len; i++) {
        exhaustive[i].poly = malloc(rate * sizeof(correct_convolutional_polynomial_t));
        exhaustive[i].distances = calloc(num_scratches, sizeof(int));
        exhaustive[i].cost = 0;
        memcpy(exhaustive[i].poly, iter_poly, rate * sizeof(correct_convolutional_polynomial_t));
        // this next loop adds 2 with "carry"
        for (size_t j = 0; j < rate; j++) {
            if (iter_poly[j] < maxcoeff) {
                iter_poly[j] += 2;
                // no more carries to propagate
                break;
            } else {
                iter_poly[j] = startcoeff;
            }
        }
    }
    free(iter_poly);

    while (convs_len > 20) {
        size_t bytes_remaining = n_bytes;

        // call init(), which sets all the error metrics to 0 for our new run
        search_exhaustive_init(exhaustive, convs_len, num_scratches);

        while (bytes_remaining) {
            // in order to keep memory usage constant, we separate the msg into
            // blocks and send each one through
            // each time we do this, we have to calculate a new noise for each
            // testbench

            size_t block_len = (max_block_len < bytes_remaining) ? max_block_len : bytes_remaining;
            bytes_remaining -= block_len;

            for (unsigned int j = 0; j < block_len; j++) {
                msg[j] = rand() % 256;
            }

            for (size_t i = 0; i < num_scratches; i++) {
                scratches[i] = resize_conv_testbench(scratches[i], conv_enclen, start.conv, block_len);
                build_white_noise(scratches[i]->noise, scratches[i]->enclen, eb_n0[i], bpsk_bit_energy);
            }

            search_exhaustive(rate, order,
                              block_len, msg, scratches, num_scratches, weights,
                              exhaustive, convs_len, bpsk_voltage);
        }

        // call fin(), which calculates a cost metric for all of the distances
        // added by our msg block iterations and then sorts by this metric
        search_exhaustive_fin(exhaustive, convs_len, weights, num_scratches);

        // decide parameters for next loop iter
        // if we've reduced to 20 or fewer items, we're going to just select
        // those and declare the test done
        size_t new_convs_len = (convs_len / 2) < 20 ? 20 : convs_len / 2;

        // normally we'll double the message length each time we halve
        // the number of entries so that each iter takes roughly the
        // same time but has twice the resolution of the previous run.
        //
        // however, if we've reached max_msg_len, then we assume that
        // the error stats collected are likely converged to whatever
        // final value they'll take, and adding more length will not
        // help us get better metrics. if we're at that point, then
        // we just select the top 20 items and declare them winners
        if (n_bytes >= max_msg_len) {
            // converged case
            new_convs_len = 20;
        } else {
            // increase our error metric resolution next run
            n_bytes *= 2;
            n_bytes = (n_bytes < max_msg_len) ? n_bytes : max_msg_len;
        }
        for (size_t i = new_convs_len; i < convs_len; i++) {
            // these entries lost, free their memory here
            free(exhaustive[i].poly);
            free(exhaustive[i].distances);
        }
        convs_len = new_convs_len;
        printf("exhaustive run: %zu items remain\n", convs_len);
    }

    for (size_t i = 0; i < convs_len; i++) {
        for (size_t j = 0; j < rate; j++) {
            printf(" %06o", exhaustive[i].poly[j]);
        }
        printf(":");
        for (size_t j = 0; j < num_scratches; j++) {
            printf(" %.2e@%.1fdB", exhaustive[i].distances[j]/((float)n_bytes * 8), eb_n0[j]);
        }
        printf("\n");
    }

    for (size_t i = 0; i < convs_len; i++) {
        free(exhaustive[i].poly);
        free(exhaustive[i].distances);
    }
    free(exhaustive);
    free(msg);
}

int main(int argc, char **argv) {
    srand(time(NULL));

    size_t rate, order, n_bytes, n_iter;

    sscanf(argv[1], "%zu", &rate);
    sscanf(argv[2], "%zu", &order);
    sscanf(argv[3], "%zu", &n_bytes);
    sscanf(argv[4], "%zu", &n_iter);

    double bpsk_voltage = 1.0/sqrt(2.0);
    double bpsk_sym_energy = 2*pow(bpsk_voltage, 2.0);
    double bpsk_bit_energy = bpsk_sym_energy/1.0;

    bpsk_bit_energy = bpsk_sym_energy * rate;  // rate bits transmitted for every input bit

    correct_convolutional_polynomial_t maxcoeff = (1 << order) - 1;
    correct_convolutional_polynomial_t startcoeff = (1 << (order - 1)) + 1;

    conv_tester_t start;

    start.poly = malloc(rate * sizeof(correct_convolutional_polynomial_t));

    for (size_t i = 0; i < rate; i++) {
        start.poly[i] = ((maxcoeff - startcoeff) / 2) + startcoeff + 1;
    }

    start.conv = conv_create(rate, order, start.poly);

    size_t num_scratches = 4;
    float *weights;
    conv_testbench **scratches = malloc(num_scratches * sizeof(conv_testbench *));
    double *eb_n0;

    for (size_t i = 0; i < num_scratches; i++) {
        scratches[i] = resize_conv_testbench(NULL, conv_enclen, start.conv, max_block_len);
    }

    switch (order) {
        case 6:
            eb_n0 = (double[]){6.0, 5.5, 5.0, 4.5};
            weights = (float[]){8000, 400, 20, 1};
            break;
        case 7:
            eb_n0 = (double[]){5.5, 5.0, 4.5, 4.0};
            weights = (float[]){8000, 400, 20, 1};
            break;
        case 8:
        case 9:
            eb_n0 = (double[]){5.0, 4.5, 4.0, 3.5};
            weights = (float[]){8000, 400, 20, 1};
            break;
        default:
            eb_n0 = (double[]){4.5, 4.0, 3.5, 3.0};
            weights = (float[]){8000, 400, 20, 1};
    }

    test(rate, order, start, scratches, num_scratches, weights, n_bytes, eb_n0, bpsk_bit_energy, n_iter, bpsk_voltage);

    free(start.poly);
    conv_destroy(start.conv);
    for (size_t i = 0; i < num_scratches; i++) {
        free_scratch(scratches[i]);
    }
    free(scratches);

    return 0;
}
