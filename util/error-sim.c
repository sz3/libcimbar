#include "correct/util/error-sim.h"

size_t distance(uint8_t *a, uint8_t *b, size_t len) {
    size_t dist = 0;
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) {

        }
        dist += popcount((unsigned int)a[i] ^ (unsigned int)b[i]);
    }
    return dist;
}

void gaussian(double *res, size_t n_res, double sigma) {
    for (size_t i = 0; i < n_res; i += 2) {
        // compute using polar method of box muller
        double s, u, v;
        while (true) {
            u = (double)(rand())/(double)RAND_MAX;
            v = (double)(rand())/(double)RAND_MAX;

            s = pow(u, 2.0) + pow(v, 2.0);

            if (s > DBL_EPSILON && s < 1) {
                break;
            }
        }

        double base = sqrt((-2.0 * log(s))/s);

        double z0 = u * base;
        res[i] = z0 * sigma;

        if (i + 1 < n_res) {
            double z1 = v * base;
            res[i + 1] = z1 * sigma;
        }
    }
}

void encode_bpsk(uint8_t *msg, double *voltages, size_t n_syms, double bpsk_voltage) {
    uint8_t mask = 0x80;
    for (size_t i = 0; i < n_syms; i++) {
        voltages[i] = msg[i/8] & mask ? bpsk_voltage : -bpsk_voltage;
        mask >>= 1;
        if (!mask) {
            mask = 0x80;
        }
    }
}

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

void decode_bpsk(uint8_t *soft, uint8_t *msg, size_t n_syms) {
    uint8_t mask = 0x80;
    for (size_t i = 0; i < n_syms; i++) {
        uint8_t bit = soft[i] > 127 ? 1 : 0;
        if (bit) {
            msg[i/8] |= mask;
        }
        mask >>= 1;
        if (!mask) {
            mask = 0x80;
        }
    }
}

void decode_bpsk_soft(double *voltages, uint8_t *soft, size_t n_syms, double bpsk_voltage) {
    for (size_t i = 0; i < n_syms; i++) {
        double rel = voltages[i]/bpsk_voltage;
        if (rel > 1) {
            soft[i] = 255;
        } else if (rel < -1) {
            soft[i] = 0;
        } else {
            soft[i] = (uint8_t)(127.5 + 127.5 * rel);
        }
    }
}

double log2amp(double l) {
    return pow(10.0, l/10.0);
}

double amp2log(double a) {
    return 10.0 * log10(a);
}

double sigma_for_eb_n0(double eb_n0, double bpsk_bit_energy) {
    // eb/n0 is the ratio of bit energy to noise energy
    // eb/n0 is expressed in dB so first we convert to amplitude
    double eb_n0_amp = log2amp(eb_n0);
    // now the conversion. sigma^2 = n0/2 = ((eb/n0)^-1 * eb)/2 = eb/(2 * (eb/n0))
    return sqrt(bpsk_bit_energy/(double)(2.0 * eb_n0_amp));
}

void build_white_noise(double *noise, size_t n_syms, double eb_n0, double bpsk_bit_energy) {
    double sigma = sigma_for_eb_n0(eb_n0, bpsk_bit_energy);
    gaussian(noise, n_syms, sigma);
}

void add_white_noise(double *signal, double *noise, size_t n_syms) {
    const double sqrt_2 = sqrt(2);
    for (size_t i = 0; i < n_syms; i++) {
        // we want to add the noise in to the signal
        // but we can't add them directly, because they're expressed as magnitudes
        //   and the signal is real valued while the noise is complex valued

        // we'll assume that the noise is exactly half real, half imaginary
        // which means it forms a 90-45-45 triangle in the complex plane
        // that means that the magnitude we have here is sqrt(2) * the real valued portion
        // so, we'll divide by sqrt(2)
        // (we are effectively throwing away the complex portion)
        signal[i] += noise[i]/sqrt_2;
    }
}

conv_testbench *resize_conv_testbench(conv_testbench *scratch, size_t (*enclen_f)(void *, size_t), void *enc, size_t msg_len) {
    if (!scratch) {
        scratch = calloc(1, sizeof(conv_testbench));
    }

    scratch->msg_out = realloc(scratch->msg_out, msg_len);

    size_t enclen = enclen_f(enc, msg_len);
    size_t enclen_bytes = (enclen % 8) ? (enclen/8 + 1) : enclen/8;
    scratch->enclen = enclen;
    scratch->enclen_bytes = enclen_bytes;

    scratch->encoded = realloc(scratch->encoded, enclen_bytes);
    scratch->v = realloc(scratch->v, enclen * sizeof(double));
    scratch->corrupted = realloc(scratch->corrupted, enclen * sizeof(double));
    scratch->noise = realloc(scratch->noise, enclen * sizeof(double));
    scratch->soft = realloc(scratch->soft, enclen);
    return scratch;
}

void free_scratch(conv_testbench *scratch) {
    free(scratch->msg_out);
    free(scratch->encoded);
    free(scratch->v);
    free(scratch->corrupted);
    free(scratch->soft);
    free(scratch->noise);
    free(scratch);
}

int test_conv_noise(conv_testbench *scratch, uint8_t *msg, size_t n_bytes,
                    double bpsk_voltage) {
    scratch->encode(scratch->encoder, msg, n_bytes, scratch->encoded);
    encode_bpsk(scratch->encoded, scratch->v, scratch->enclen, bpsk_voltage);

    memcpy(scratch->corrupted, scratch->v, scratch->enclen * sizeof(double));
    add_white_noise(scratch->corrupted, scratch->noise, scratch->enclen);
    decode_bpsk_soft(scratch->corrupted, scratch->soft, scratch->enclen, bpsk_voltage);

    memset(scratch->msg_out, 0, n_bytes);

    ssize_t decode_len = scratch->decode(scratch->decoder, scratch->soft, scratch->enclen, scratch->msg_out);

    if (decode_len != n_bytes) {
        printf("expected to decode %zu bytes, decoded %zu bytes instead\n", n_bytes, decode_len);
        exit(1);
    }

    return distance((uint8_t*)msg, scratch->msg_out, n_bytes);
}

size_t conv_correct_enclen(void *conv_v, size_t msg_len) {
    return correct_convolutional_encode_len((correct_convolutional *)conv_v, msg_len);
}

void conv_correct_encode(void *conv_v, uint8_t *msg, size_t msg_len, uint8_t *encoded) {
    correct_convolutional_encode((correct_convolutional *)conv_v, msg, msg_len, encoded);
}

ssize_t conv_correct_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    return correct_convolutional_decode_soft((correct_convolutional *)conv_v, soft, soft_len, msg);
}
