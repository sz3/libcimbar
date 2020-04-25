#include "correct/convolutional/lookup.h"
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

typedef unsigned int distance_quad_key_t;
typedef unsigned int output_quad_t;
typedef uint64_t distance_quad_t;

typedef struct {
    distance_quad_key_t *keys;
    output_quad_t *outputs;
    output_quad_t output_mask;
    unsigned int output_width;
    size_t outputs_len;
    distance_quad_t *distances;
} quad_lookup_t;

typedef uint16_t distance_oct_key_t;
typedef uint64_t output_oct_t;
typedef uint64_t distance_oct_t;

typedef struct {
    distance_oct_key_t *keys;
    output_oct_t *outputs;
    output_oct_t output_mask;
    unsigned int output_width;
    size_t outputs_len;
    distance_oct_t *distances;
} oct_lookup_t;

quad_lookup_t quad_lookup_create(unsigned int rate,
                                 unsigned int order,
                                 const unsigned int *table);
void quad_lookup_destroy(quad_lookup_t quads);
void quad_lookup_fill_distance(quad_lookup_t quads, distance_t *distances);
distance_oct_key_t oct_lookup_find_key(output_oct_t *outputs, output_oct_t out, size_t num_keys);
oct_lookup_t oct_lookup_create(unsigned int rate,
                                 unsigned int order,
                                 const unsigned int *table);
void oct_lookup_destroy(oct_lookup_t octs);
static inline void oct_lookup_fill_distance(oct_lookup_t octs, distance_t *distances) {
    distance_pair_t *pairs = (distance_pair_t*)octs.distances;
    for (unsigned int i = 1; i < octs.outputs_len; i += 1) {
        output_oct_t concat_out = octs.outputs[i];
        unsigned int i_0 = concat_out & 0xff;
        unsigned int i_1 = (concat_out >> 8) & 0xff;
        unsigned int i_2 = (concat_out >> 16) & 0xff;
        unsigned int i_3 = (concat_out >> 24) & 0xff;

        pairs[i*4 + 1] = distances[i_3] << 16 | distances[i_2];
        pairs[i*4 + 0] = distances[i_1] << 16 | distances[i_0];

        concat_out >>= 32;
        unsigned int i_4 = concat_out & 0xff;
        unsigned int i_5 = (concat_out >> 8) & 0xff;
        unsigned int i_6 = (concat_out >> 16) & 0xff;
        unsigned int i_7 = (concat_out >> 24) & 0xff;

        pairs[i*4 + 3] = distances[i_7] << 16 | distances[i_6];
        pairs[i*4 + 2] = distances[i_5] << 16 | distances[i_4];
    }
}
