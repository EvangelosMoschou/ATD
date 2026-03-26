#include <math.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "rrc_link.h"
#include "prng.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(int argc, char** argv) {
    const int m = 8;
    const int bits_per_symbol = 3;
    const size_t symbols_per_snr = 200000;
    const double constellation_capture_db = 10.0;
    const size_t constellation_points = 4000;
    const int sps = 8;
    const int span = 16;
    const double rolloff = 0.2;
    LinkOptions opts;

    Complex constel[8];
    for (int k = 0; k < m; ++k) {
        const double th = 2.0 * M_PI * (double)k / (double)m;
        constel[k].re = cos(th);
        constel[k].im = sin(th);
    }

    if (normalize_constellation(constel, m) != 0) {
        fprintf(stderr, "Failed to normalize 8-PSK constellation\n");
        return 1;
    }

    unsigned int seed = (unsigned int)time(NULL);
    int avg_windows = 1;
    if (argc >= 2) {
        char* end = NULL;
        errno = 0;
        const unsigned long v = strtoul(argv[1], &end, 10);
        if (errno != 0 || end == argv[1] || *end != '\0' || v > UINT_MAX) {
            fprintf(stderr, "Usage: %s [seed]\n", argv[0]);
            return 2;
        }
        seed = (unsigned int)v;
    }
    prng_seed((uint32_t)seed);
    printf("8psk RNG seed=%u\n", seed);

    const char* avg_env = getenv("SPECTRUM_AVG_WINDOWS");
    if (avg_env && *avg_env) {
        char* end = NULL;
        errno = 0;
        const long v = strtol(avg_env, &end, 10);
        if (errno == 0 && end != avg_env && *end == '\0' && v >= 1) {
            avg_windows = (int)v;
        }
    }
    printf("8psk spectrum_avg_windows=%d\n", avg_windows);

    opts.sps = sps;
    opts.filter_span_symbols = span;
    opts.rolloff = rolloff;
    opts.use_rf = 1;
    opts.sample_rate_hz = 4.0e9;
    opts.carrier_hz = 30.0e9;
    opts.export_spectrum = 1;
    opts.spectrum_capture_db = 10.0;
    opts.spectrum_fft_len = 1024;
    opts.spectrum_avg_windows = avg_windows;
    opts.use_gray_coding = 1;
    opts.ring_bits = 0;

    return simulate_modulated_link(
        "8psk",
        constel,
        m,
        bits_per_symbol,
        symbols_per_snr,
        constellation_capture_db,
        constellation_points,
        &opts);
}
