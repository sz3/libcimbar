#include "correct/convolutional/sse/lookup.h"

quad_lookup_t quad_lookup_create(unsigned int rate,
                                 unsigned int order,
                                 const unsigned int *table) {
    quad_lookup_t quads;

    quads.keys = malloc(sizeof(unsigned int) * (1 << (order - 2)));
    quads.outputs = calloc((1 << (rate * 4)), sizeof(unsigned int));
    unsigned int *inv_outputs = calloc((1 << (rate * 4)), sizeof(unsigned int));
    unsigned int output_counter = 1;
    // for every (even-numbered) shift register state, find the concatenated output of the state
    //   and the subsequent state that follows it (low bit set). then, check to see if this
    //   concatenated output has a unique key assigned to it already. if not, give it a key.
    //   if it does, retrieve the key. assign this key to the shift register state.
    for (unsigned int i = 0; i < (1 << (order - 2)); i++) {
        // first get the concatenated quad of outputs
        unsigned int out = table[i * 4 + 3];
        out <<= rate;
        out |= table[i * 4 + 2];
        out <<= rate;
        out |= table[i * 4 + 1];
        out <<= rate;
        out |= table[i * 4];

        // does this concatenated output exist in the outputs table yet?
        if (!inv_outputs[out]) {
            // doesn't exist, allocate a new key
            inv_outputs[out] = output_counter;
            quads.outputs[output_counter] = out;
            output_counter++;
        }
        // set the opaque key for the ith shift register state to the concatenated output entry
        quads.keys[i] = inv_outputs[out];
    }
    quads.outputs_len = output_counter;
    quads.output_mask = (1 << (rate)) - 1;
    quads.output_width = rate;
    quads.distances = calloc(quads.outputs_len, sizeof(distance_quad_t));
    free(inv_outputs);
    return quads;
}

void quad_lookup_destroy(quad_lookup_t quads) {
    free(quads.keys);
    free(quads.outputs);
    free(quads.distances);
}

void quad_lookup_fill_distance(quad_lookup_t quads, distance_t *distances) {
    for (unsigned int i = 1; i < quads.outputs_len; i += 1) {
        output_quad_t concat_out = quads.outputs[i];
        unsigned int i_0 = concat_out & quads.output_mask;
        concat_out >>= quads.output_width;
        unsigned int i_1 = concat_out & quads.output_mask;
        concat_out >>= quads.output_width;
        unsigned int i_2 = concat_out & quads.output_mask;
        concat_out >>= quads.output_width;
        unsigned int i_3 = concat_out;

        quads.distances[i] = ((uint64_t)distances[i_3] << 48) | ((uint64_t)distances[i_2] << 32) | (distances[i_1] << 16) | distances[i_0];
    }
}

distance_oct_key_t oct_lookup_find_key(output_oct_t *outputs, output_oct_t out, size_t num_keys) {
    for (size_t i = 1; i < num_keys; i++) {
        if (outputs[i] == out) {
            return i;
        }
    }
    return 0;
}

oct_lookup_t oct_lookup_create(unsigned int rate,
                                 unsigned int order,
                                 const unsigned int *table) {
    oct_lookup_t octs;

    octs.keys = malloc((1 << (order - 3)) * sizeof(distance_oct_key_t));
    octs.outputs = malloc(((output_oct_t)2 << rate) * sizeof(uint64_t));
    output_oct_t *short_outs = calloc(((output_oct_t)2 << rate), sizeof(output_oct_t));
    size_t outputs_len = 2 << rate;
    unsigned int output_counter = 1;
    // for every (even-numbered) shift register state, find the concatenated output of the state
    //   and the subsequent state that follows it (low bit set). then, check to see if this
    //   concatenated output has a unique key assigned to it already. if not, give it a key.
    //   if it does, retrieve the key. assign this key to the shift register state.
    for (shift_register_t i = 0; i < (1 << (order - 3)); i++) {
        // first get the concatenated oct of outputs
        output_oct_t out = table[i * 8 + 7];
        out <<= rate;
        out |= table[i * 8 + 6];
        out <<= rate;
        out |= table[i * 8 + 5];
        out <<= rate;
        out |= table[i * 8 + 4];
        out <<= rate;
        out |= table[i * 8 + 3];
        out <<= rate;
        out |= table[i * 8 + 2];
        out <<= rate;
        out |= table[i * 8 + 1];
        out <<= rate;
        out |= table[i * 8];

        distance_oct_key_t key = oct_lookup_find_key(short_outs, out, output_counter);
        // does this concatenated output exist in the outputs table yet?
        if (!key) {
            // doesn't exist, allocate a new key
            // now build it in expanded form
            output_oct_t expanded_out = table[i * 8 + 7];
            expanded_out <<= 8;
            expanded_out |= table[i * 8 + 6];
            expanded_out <<= 8;
            expanded_out |= table[i * 8 + 5];
            expanded_out <<= 8;
            expanded_out |= table[i * 8 + 4];
            expanded_out <<= 8;
            expanded_out |= table[i * 8 + 3];
            expanded_out <<= 8;
            expanded_out |= table[i * 8 + 2];
            expanded_out <<= 8;
            expanded_out |= table[i * 8 + 1];
            expanded_out <<= 8;
            expanded_out |= table[i * 8];

            if (output_counter == outputs_len) {
                octs.outputs = realloc(octs.outputs, outputs_len * 2 * sizeof(output_oct_t));
                short_outs = realloc(short_outs, outputs_len * 2 * sizeof(output_oct_t));
                outputs_len *= 2;
            }
            short_outs[output_counter] = out;
            octs.outputs[output_counter] = expanded_out;
            key = output_counter;
            output_counter++;
        }
        // set the opaque key for the ith shift register state to the concatenated output entry
        // we multiply the key by 2 since the distances are strided by 2
        octs.keys[i] = key * 2;
    }
    free(short_outs);
    octs.outputs_len = output_counter;
    octs.output_mask = (1 << (rate)) - 1;
    octs.output_width = rate;
    octs.distances = malloc(octs.outputs_len * 2 * sizeof(uint64_t));
    return octs;
}

void oct_lookup_destroy(oct_lookup_t octs) {
    free(octs.keys);
    free(octs.outputs);
    free(octs.distances);
}

// WIP: sse approach to filling the distance table
/*
void oct_lookup_fill_distance_sse(oct_lookup_t octs, distance_t *distances) {
    distance_pair_t *distance_pair = (distance_pair_t*)octs.distances;
    __v4si index_shuffle_mask = (__v4si){0xffffff00, 0xffffff01, 0xffffff02, 0xffffff03};
    __m256i dist_shuffle_mask = (__m256i){0x01000504, 0x09080d0c, 0xffffffff, 0xffffffff,
                                          0x01000504, 0x09080d0c, 0xffffffff, 0xffffffff};
    const int dist_permute_mask = 0x0c;
    for (unsigned int i = 1; i < octs.outputs_len; i += 2) {
        // big heaping todo vvv
        // a) we want 16 bit distances GATHERed, not 32 bit
        // b) we need to load 8 of those distances, not 4
        __v4si short_concat_index = _mm_loadl_epi64(octs.outputs + 2*i);
        __v4si short_concat_index0 = _mm_loadl_epi64(octs.outputs + 2*i + 1);
        __m256i concat_index = _mm256_cvtepu8_epi32(short_concat_index);
        __m256i concat_index0 = _mm256_cvtepu8_epi32(short_concat_index0);
        __m256i dist = _mm256_i32gather_epi32(distances, concat_index, sizeof(distance_t));
        __m256i dist0 = _mm256_i32gather_epi32(distances, concat_index0, sizeof(distance_t));
        dist = _mm256_shuffle_epi8(dist, dist_shuffle_mask);
        dist0 = _mm256_shuffle_epi8(dist0, dist_shuffle_mask);
        dist = __builtin_shufflevector(dist, dist, 0, 5, 0, 0);
        dist0 = __builtin_shufflevector(dist0, dist0, 0, 5, 0, 0);
        __v4si packed_dist = _mm256_castsi256_si128(dist);
        _mm_store_si128(distance_pair + 8 * i, packed_dist);
        __v4si packed_dist0 = _mm256_castsi256_si128(dist0);
        _mm_store_si128(distance_pair + 8 * i + 4, packed_dist0);
    }
}
*/
