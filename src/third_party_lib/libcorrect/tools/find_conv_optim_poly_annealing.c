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
#include <signal.h>

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

void shuffle(int *a, size_t len) {
    for (size_t i = 0; i < len - 2; i++) {
        size_t j = rand() % (len - i) + i;
        int temp = a[i];
        a[i] = a[j];
        a[j] = temp;
    }
}

int rand_geo(float p, int max) {
    int geo = 1;
    while (geo < max) {
        if (rand() / (float)RAND_MAX > p) {
            geo++;
        } else {
            break;
        }
    }
    return geo;
}

void next_neighbor(correct_convolutional_polynomial_t *start,
                   correct_convolutional_polynomial_t *neighbor, size_t rate, size_t order) {
    int coeffs[rate * (order - 2)];
    for (int i = 0; i < rate * (order - 2); i++) {
        coeffs[i] = i;
    }
    shuffle(coeffs, rate * (order - 2));

    memcpy(neighbor, start, rate * sizeof(correct_convolutional_polynomial_t));
    size_t nflips = rand_geo(0.4, rate * (order - 2));
    for (int i = 0; i < nflips; i++) {
        ptrdiff_t index = coeffs[i] / (order - 2);
        // decide which bit to flip
        // we avoid the edge bits to prevent creating a degenerate poly
        neighbor[index] ^= 1 << (coeffs[i] % (order - 2) + 1);
    }
}

bool accept(float cost_a, float cost_b, double temperature) {
    if (cost_b < cost_a) {
        return true;
    }

    float p = (float)(rand()) / (float)(RAND_MAX);

    return exp((cost_a - cost_b) / (cost_a * temperature)) > p;
}

typedef struct {
    size_t rate;
    size_t order;
    correct_convolutional_polynomial_t *poly;
    unsigned int distance;
    conv_testbench *scratch;
    size_t msg_len;
    double eb_n0;
    double bpsk_voltage;
    double bpsk_bit_energy;
} thread_args;

const size_t max_block_len = 16384;

void *find_cost_thread(void *vargs) {
    thread_args *args = (thread_args *)vargs;
    conv_t *conv;
    uint8_t *msg = malloc(max_block_len);

    conv = conv_create(args->rate, args->order, args->poly);
    args->distance = 0;
    conv_testbench *scratch = args->scratch;

    size_t bytes_remaining = args->msg_len;
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

        scratch = resize_conv_testbench(scratch, conv_enclen, conv, block_len);
        scratch->encode = conv_encode;
        scratch->encoder = conv;
        scratch->decode = conv_decode;
        scratch->decoder = conv;

        build_white_noise(scratch->noise, scratch->enclen, args->eb_n0, args->bpsk_bit_energy);

        args->distance += test_conv_noise(scratch, msg, block_len, args->bpsk_voltage);
    }
    conv_destroy(conv);
    free(msg);
    pthread_exit(NULL);
}

float find_cost(size_t rate, size_t order, correct_convolutional_polynomial_t *poly, size_t msg_len,
                conv_testbench **scratches, size_t num_scratches, float *weights, double *eb_n0,
                double bpsk_voltage, double bpsk_bit_energy) {
    thread_args *args = malloc(num_scratches * sizeof(thread_args));
    pthread_t *threads = malloc(num_scratches * sizeof(pthread_t));

    for (size_t i = 0; i < num_scratches; i++) {
        args[i].rate = rate;
        args[i].order = order;
        args[i].poly = poly;
        args[i].scratch = scratches[i];
        args[i].msg_len = msg_len;
        args[i].eb_n0 = eb_n0[i];
        args[i].bpsk_voltage = bpsk_voltage;
        args[i].bpsk_bit_energy = bpsk_bit_energy;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&threads[i], &attr, find_cost_thread, &args[i]);
    }

    for (size_t i = 0; i < num_scratches; i++) {
        pthread_join(threads[i], NULL);
    }

    float cost = 0;
    printf("poly:");
    for (size_t i = 0; i < rate; i++) {
        printf(" %06o", poly[i]);
    }
    printf(" error:");
    for (size_t i = 0; i < num_scratches; i++) {
        cost += weights[i] * args[i].distance;
        printf(" %.2e@%.1fdB", (args[i].distance / (float)(msg_len * 8)), eb_n0[i]);
    }
    printf("\n");

    free(args);
    free(threads);

    return cost;
}

static bool terminated = false;

void sig_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM || sig == SIGHUP) {
        if (!terminated) {
            terminated = true;
            printf("terminating after current poly\n");
        }
    }
}

void search_simulated_annealing(size_t rate, size_t order, size_t n_steps, conv_tester_t *start,
                                size_t n_bytes, conv_testbench **scratches, size_t num_scratches,
                                float *weights, double start_temperature, double cooling_factor,
                                double *eb_n0, double bpsk_voltage, double bpsk_bit_energy) {
    // perform simulated annealing to find the optimal polynomial

    float cost = find_cost(rate, order, start->poly, n_bytes, scratches, num_scratches, weights,
                           eb_n0, bpsk_voltage, bpsk_bit_energy);

    correct_convolutional_polynomial_t *neighbor_poly =
        malloc(rate * sizeof(correct_convolutional_polynomial_t));

    correct_convolutional_polynomial_t *state =
        malloc(rate * sizeof(correct_convolutional_polynomial_t));
    correct_convolutional_polynomial_t *best =
        malloc(rate * sizeof(correct_convolutional_polynomial_t));

    float best_cost = cost;

    memcpy(state, start->poly, rate * sizeof(correct_convolutional_polynomial_t));
    memcpy(best, start->poly, rate * sizeof(correct_convolutional_polynomial_t));

    double temperature = start_temperature;

    for (size_t i = 0; i < n_steps; i++) {
        next_neighbor(state, neighbor_poly, rate, order);
        float neighbor_cost =
            find_cost(rate, order, neighbor_poly, n_bytes, scratches, num_scratches, weights, eb_n0,
                      bpsk_voltage, bpsk_bit_energy);
        if (accept(cost, neighbor_cost, temperature)) {
            // we're moving to our neighbor's house
            memcpy(state, neighbor_poly, rate * sizeof(correct_convolutional_polynomial_t));
            cost = neighbor_cost;
        } else {
            // actually where we live now is nice
        }

        if (cost < best_cost) {
            best_cost = cost;
            memcpy(best, state, rate * sizeof(correct_convolutional_polynomial_t));
        }

        temperature *= cooling_factor;

        if (terminated) {
            break;
        }
    }

    printf("last state:");
    for (size_t i = 0; i < rate; i++) {
        printf(" %06o", state[i]);
    }
    printf("\n");

    printf("best state:");
    for (size_t i = 0; i < rate; i++) {
        printf(" %06o", best[i]);
    }

    memcpy(start->poly, best, rate * sizeof(correct_convolutional_polynomial_t));

    free(state);
    free(best);
    free(neighbor_poly);
}

void test_sa(size_t rate, size_t order, conv_tester_t start, conv_testbench **scratches,
             size_t num_scratches, float *weights, size_t n_bytes, double *eb_n0,
             double bpsk_bit_energy, size_t n_iter, double bpsk_voltage) {
    for (size_t i = 0; i < n_iter; i++) {
        double temperature = (i == 0) ? 0.5 : 250;
        double cooling_factor = (i == 0) ? 0.985 : 0.95;
        size_t n_steps = (i == 0) ? 500 : 100;

        search_simulated_annealing(rate, order, n_steps, &start, n_bytes, scratches, num_scratches,
                                   weights, temperature, cooling_factor, eb_n0, bpsk_voltage,
                                   bpsk_bit_energy);
    }
}

int main(int argc, char **argv) {
    srand(time(NULL));

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGHUP, sig_handler);

    size_t rate, order, n_bytes, n_iter;

    sscanf(argv[1], "%zu", &rate);
    sscanf(argv[2], "%zu", &order);
    sscanf(argv[3], "%zu", &n_bytes);
    sscanf(argv[4], "%zu", &n_iter);

    double bpsk_voltage = 1.0 / sqrt(2.0);
    double bpsk_sym_energy = 2 * pow(bpsk_voltage, 2.0);
    double bpsk_bit_energy = bpsk_sym_energy / 1.0;

    bpsk_bit_energy = bpsk_sym_energy * rate;  // rate bits transmitted for every input bit

    // correct_convolutional_polynomial_t maxcoeff = (1 << order) - 1;
    correct_convolutional_polynomial_t startcoeff = (1 << (order - 1)) + 1;

    conv_tester_t start;

    start.poly = malloc(rate * sizeof(correct_convolutional_polynomial_t));

    for (size_t i = 0; i < rate; i++) {
        start.poly[i] = ((rand() % (1 << (order - 2))) << 1) + startcoeff;
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
        case 8:
            eb_n0 = (double[]){5.5, 5.0, 4.5, 4.0};
            weights = (float[]){8000, 400, 20, 1};
            break;
        case 9:
        case 10:
            eb_n0 = (double[]){5.0, 4.5, 4.0, 3.5};
            weights = (float[]){8000, 400, 20, 1};
            break;
        case 11:
        case 12:
        case 13:
            eb_n0 = (double[]){4.5, 4.0, 3.5, 3.0};
            weights = (float[]){8000, 400, 20, 1};
            break;
        default:
            eb_n0 = (double[]){3.5, 3.0, 2.5, 2.0};
            weights = (float[]){8000, 400, 20, 1};
    }

    test_sa(rate, order, start, scratches, num_scratches, weights, n_bytes, eb_n0, bpsk_bit_energy,
            n_iter, bpsk_voltage);

    free(start.poly);
    conv_destroy(start.conv);
    for (size_t i = 0; i < num_scratches; i++) {
        free_scratch(scratches[i]);
    }
    free(scratches);

    return 0;
}
