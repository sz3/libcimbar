#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <assert.h>

#include <correct.h>
#include <fec.h>

// this program allows us to find all of the polynomials that come with libfec
// this way, we can provide compatibility with libfec-encoded streams and vice versa
// we can do this without directly copy-pasting from libfec's source, thanks
//   to this finder

typedef struct {
    void *vit;
    int update_len;
    int (*init)(void *, int);
    int (*update)(void *, unsigned char *, int);
    int (*chainback)(void *, unsigned char *, unsigned int, unsigned int);
} libfec_decoder_t;

void byte2bit(uint8_t *bytes, uint8_t *bits, size_t n_bits) {
    unsigned char cmask = 0x80;
    for (size_t i = 0; i < n_bits; i++) {
        bits[i] = (bytes[i/8] & cmask) ? 255 : 0;
        cmask >>= 1;
        if (!cmask) {
            cmask = 0x80;
        }
    }
}

correct_convolutional_polynomial_t *resize_poly_list(correct_convolutional_polynomial_t *polys, size_t cap) {
    polys = realloc(polys, cap * sizeof(correct_convolutional_polynomial_t));
    return polys;
}

void find_poly_coeff(size_t rate, size_t order, uint8_t *msg, size_t msg_len, libfec_decoder_t libfec, correct_convolutional_polynomial_t **polys_dest, size_t *polys_len, size_t search_coeff) {
    // find a single coefficient of an unknown convolutional polynomial
    // we are given a payload to encode, and we'll test all possible coefficients
    //    to see which ones yield correct decodings by libfec, which has some
    //    unknown polynomial "baked in"

    // temp poly (this will be the one we search with)
    correct_convolutional_polynomial_t *poly = malloc(rate * sizeof(correct_convolutional_polynomial_t));

    // what's the largest coefficient value we'll test?
    correct_convolutional_polynomial_t maxcoeff = (1 << order) - 1;

    // note that we start about half way in
    // this sum asks that we have the
    //   a) highest order bit set
    //   b) lowest order bit set
    // we're only interested in coefficient values for which this is
    //   true because if it weren't, the coefficient would actually be
    //   of a smaller order than its supposed given order
    correct_convolutional_polynomial_t startcoeff = (1 << (order - 1)) + 1;

    // the values of this don't really matter except for the coeff we're searching for
    // but just to be safe, we set them all
    for (size_t i = 0; i < rate; i++) {
        poly[i] = startcoeff;
    }

    // create a dummy encoder so that we can find how long the resulting encoded value is
    correct_convolutional *conv_dummy = correct_convolutional_create(rate, order, poly);
    size_t enclen_bits = correct_convolutional_encode_len(conv_dummy, msg_len);
    size_t enclen = (enclen_bits % 8) ? (enclen_bits / 8 + 1) : enclen_bits / 8;
    correct_convolutional_destroy(conv_dummy);

    // compact encoded format (this comes from libcorrect)
    uint8_t *encoded = malloc(enclen * sizeof(uint8_t));
    // soft encoded format (this goes to libfec, one byte per bit)
    uint8_t *encoded_bits = malloc(enclen * 8 * sizeof(uint8_t));
    // resulting decoded message which we'll compare to our given payload
    uint8_t *msg_cmp = malloc(msg_len * sizeof(uint8_t));

    // we keep a list of coefficients which yielded correct decodings
    // there could be 0, 1, or more than 1, and we'll return all of them
    // we'll dynamically resize this as we go
    size_t polys_cap = 1;
    *polys_len = 0;
    correct_convolutional_polynomial_t *polys = NULL;
    polys = resize_poly_list(polys, polys_cap);

    // iteration constants -- we go by 2 because we want the lowest order bit to
    // stay set
    for (correct_convolutional_polynomial_t i = startcoeff; i <= maxcoeff; i += 2) {
        poly[search_coeff] = i;
        correct_convolutional *conv = correct_convolutional_create(rate, order, poly);

        correct_convolutional_encode(conv, (uint8_t*)msg, msg_len, encoded);
        byte2bit(encoded, encoded_bits, enclen);

        // now erase all the bits we're not searching for
        for (size_t i = 0; i < msg_len * 8; i++) {
            for (size_t j = 0; j < rate; j++) {
                if (j != search_coeff) {
                    // 128 is a soft erasure
                    encoded_bits[i * rate + j] = 128;
                }
            }
        }

        libfec.init(libfec.vit, 0);
        libfec.update(libfec.vit, encoded_bits, libfec.update_len);
        libfec.chainback(libfec.vit, msg_cmp, 8 * msg_len, 0);

        correct_convolutional_destroy(conv);

        if (memcmp(msg_cmp, msg, msg_len) == 0) {
            // match found

            // resize list to make room
            if (*polys_len == polys_cap) {
                polys = resize_poly_list(polys, polys_cap * 2);
                polys_cap *= 2;
            }
            polys[*polys_len] = i;
            *polys_len = *polys_len + 1;
        }
    }

    polys = resize_poly_list(polys, *polys_len);
    *polys_dest = polys;
    free(poly);
    free(msg_cmp);
    free(encoded);
    free(encoded_bits);
}

// we choose 2 bytes because we need a payload that's longer than
// the shift register under test. since that includes an order 15
// s.r., we need at least 15 bits.
size_t msg_len = 2;

void find_poly(size_t rate, size_t order, libfec_decoder_t libfec, correct_convolutional_polynomial_t *poly) {
    // find the complete set of coefficients that are "baked in" to
    //   one particular method of libfec
    // most of this method is described by find_poly_coeff

    // for each coeff we want to find, we'll generate random 2-byte payloads and give
    //   them to find_poly_coeff. If find_poly_coeff returns an empty list, we
    //   try again. If it returns a nonempty list, then we find the intersection of
    //   all the coefficient values find_poly_coeff has given us so far (we start
    //   with the complete set). we are finished when only one coeff value remains

    // we perform this process for each coeff e.g. 6 times for a rate 1/6 polynomial

    uint8_t msg[msg_len];

    // this is the list returned to us by find_poly_coeff
    correct_convolutional_polynomial_t *polys;
    // the list's length is written here
    size_t polys_len;

    printf("rate 1/%zu order %zu poly:", rate, order);

    for (size_t search_coeff = 0; search_coeff < rate; search_coeff++) {
        correct_convolutional_polynomial_t *fit = NULL;
        size_t fit_len = 0;
        size_t fit_cap = 0;
        bool done = false;

        while (!done) {
            for (size_t i = 0; i < msg_len; i++) {
                msg[i] = rand() % 256;
            }
            find_poly_coeff(rate, order, msg, msg_len, libfec, &polys, &polys_len, search_coeff);

            if (polys_len == 0) {
                // skip if none fit (this is a special case)
                continue;
            }

            if (fit_len == 0) {
                // the very first intersection
                // we'll just copy the list handed to us
                fit_cap = polys_len;
                fit_len = polys_len;
                fit = resize_poly_list(fit, fit_cap);
                for (size_t i = 0; i < polys_len; i++) {
                    fit[i] = polys[i];
                }
            } else {
                // find intersection
                ptrdiff_t polys_iter = 0;
                ptrdiff_t fit_iter = 0;
                ptrdiff_t new_fit_iter = 0;
                // the lists generated by find_poly_coeff are sorted
                // so we just retain the sorted property and walk both
                while (polys_iter < polys_len && fit_iter < fit_len) {
                    if (polys[polys_iter] < fit[fit_iter]) {
                        polys_iter++;
                    } else if (polys[polys_iter] > fit[fit_iter]) {
                        fit_iter++;
                    } else {
                        fit[new_fit_iter] = fit[fit_iter];
                        polys_iter++;
                        fit_iter++;
                        new_fit_iter++;
                    }
                }
                // if new_fit_iter is 0 here then we don't intersect at all
                // in this case we have to restart the search for this coeff
                if (new_fit_iter != 0) {
                    fit_len = new_fit_iter;
                } else {
                    free(fit);
                    fit = NULL;
                    fit_cap = 0;
                    fit_len = 0;
                }
            }

            free(polys);

            if (fit_len == 1) {
                poly[search_coeff] = fit[0];
                if (order <= 9) {
                    printf(" %04o", fit[0]);
                } else {
                    printf(" %06o", fit[0]);
                }
                done = true;
            }
        }

        free(fit);
    }
    printf("\n");
}

int main() {
    libfec_decoder_t libfec;

    srand(time(NULL));

    setbuf(stdout, NULL);

    correct_convolutional_polynomial_t poly[6];

    libfec.vit = create_viterbi27(8 * msg_len);
    libfec.update_len = 8 * msg_len + 6;
    libfec.init = init_viterbi27;
    libfec.update = update_viterbi27_blk;
    libfec.chainback = chainback_viterbi27;
    find_poly(2, 7, libfec, poly);
    delete_viterbi27(libfec.vit);

    libfec.vit = create_viterbi29(8 * msg_len);
    libfec.update_len = 8 * msg_len + 8;
    libfec.init = init_viterbi29;
    libfec.update = update_viterbi29_blk;
    libfec.chainback = chainback_viterbi29;
    find_poly(2, 9, libfec, poly);
    delete_viterbi29(libfec.vit);

    libfec.vit = create_viterbi39(8 * msg_len);
    libfec.update_len = 8 * msg_len + 8;
    libfec.init = init_viterbi39;
    libfec.update = update_viterbi39_blk;
    libfec.chainback = chainback_viterbi39;
    find_poly(3, 9, libfec, poly);
    delete_viterbi39(libfec.vit);

    libfec.vit = create_viterbi615(8 * msg_len);
    libfec.update_len = 8 * msg_len + 14;
    libfec.init = init_viterbi615;
    libfec.update = update_viterbi615_blk;
    libfec.chainback = chainback_viterbi615;
    find_poly(6, 15, libfec, poly);
    delete_viterbi615(libfec.vit);

    return 0;
}
