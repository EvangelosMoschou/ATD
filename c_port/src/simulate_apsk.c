#include <math.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "rrc_link.h"
#include "prng.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_M 256

static int build_64apsk_constellation(Complex* out) {
    const int rings = 8;
    const int points_per_ring = 8;
    const double radii[8] = {1.000, 1.791, 2.405, 2.980, 3.569, 4.235, 5.078, 6.536};
    const double offsets[8] = {
        M_PI / 16.0, 3.0 * M_PI / 16.0, 5.0 * M_PI / 16.0, 7.0 * M_PI / 16.0,
        9.0 * M_PI / 16.0, 11.0 * M_PI / 16.0, 13.0 * M_PI / 16.0, 15.0 * M_PI / 16.0
    };

    int idx = 0;
    for (int r = 0; r < rings; ++r) {
        for (int k = 0; k < points_per_ring; ++k) {
            const double th = offsets[r] + 2.0 * M_PI * (double)k / (double)points_per_ring;
            out[idx].re = radii[r] * cos(th);
            out[idx].im = radii[r] * sin(th);
            ++idx;
        }
    }

    return normalize_constellation(out, 64);
}

static int build_256apsk_constellation(Complex* out) {
    const int rings = 8;
    const int points_per_ring = 32;
    const double radii[8] = {1.000, 1.791, 2.405, 2.980, 3.569, 4.235, 5.078, 6.536};
    const double offsets[8] = {
        M_PI / 32.0, 3.0 * M_PI / 32.0, 5.0 * M_PI / 32.0, 7.0 * M_PI / 32.0,
        9.0 * M_PI / 32.0, 11.0 * M_PI / 32.0, 13.0 * M_PI / 32.0, 15.0 * M_PI / 32.0
    };

    int idx = 0;
    for (int r = 0; r < rings; ++r) {
        for (int k = 0; k < points_per_ring; ++k) {
            const double th = offsets[r] + 2.0 * M_PI * (double)k / (double)points_per_ring;
            out[idx].re = radii[r] * cos(th);
            out[idx].im = radii[r] * sin(th);
            ++idx;
        }
    }

    return normalize_constellation(out, 256);
}

int main(int argc, char** argv) {
    Complex c64[MAX_M];
    Complex c256[MAX_M];

    const int sps = 8;
    const int span = 16;
    const double rolloff = 0.2;
    LinkOptions opts;

    if (build_64apsk_constellation(c64) != 0) {
        fprintf(stderr, "Failed to build 64-APSK constellation\n");
        return 3;
    }
    if (build_256apsk_constellation(c256) != 0) {
        fprintf(stderr, "Failed to build 256-APSK constellation\n");
        return 4;
    }

    unsigned int seed = (unsigned int)time(NULL);
    int avg_windows = 1;

    opts.sps = sps;
    opts.filter_span_symbols = span;
    opts.rolloff = rolloff;
    opts.use_rf = 1;
    opts.sample_rate_hz = 4.0e9;
    opts.export_spectrum = 1;
    opts.spectrum_capture_db = 10.0;
    opts.spectrum_fft_len = 1024;
    opts.use_gray_coding = 1;
    opts.ring_bits = 3;

    int do64 = 1;
    int do256 = 1;

    if (argc >= 2) {
        if (strcmp(argv[1], "64") == 0) {
            do256 = 0;
        } else if (strcmp(argv[1], "256") == 0) {
            do64 = 0;
        } else if (strcmp(argv[1], "all") == 0) {
            do64 = 1;
            do256 = 1;
        } else {
            fprintf(stderr, "Usage: %s [64|256|all]\n", argv[0]);
            return 2;
        }
    }

    if (argc >= 3) {
        char* end = NULL;
        errno = 0;
        const unsigned long v = strtoul(argv[2], &end, 10);
        if (errno != 0 || end == argv[2] || *end != '\0' || v > UINT_MAX) {
            fprintf(stderr, "Usage: %s [64|256|all] [seed]\n", argv[0]);
            return 2;
        }
        seed = (unsigned int)v;
    }
    prng_seed((uint32_t)seed);
    printf("apsk RNG seed=%u\n", seed);

    const char* avg_env = getenv("SPECTRUM_AVG_WINDOWS");
    if (avg_env && *avg_env) {
        char* end = NULL;
        errno = 0;
        const long v = strtol(avg_env, &end, 10);
        if (errno == 0 && end != avg_env && *end == '\0' && v >= 1) {
            avg_windows = (int)v;
        }
    }
    printf("apsk spectrum_avg_windows=%d\n", avg_windows);

    if (do64) {
        opts.carrier_hz = 24.125e9;
        const int rc = simulate_modulated_link("64apsk", c64, 64, 6, 200000, 10.0, 5000, &opts);
        if (rc != 0) {
            return rc;
        }
    }

    if (do256) {
        opts.carrier_hz = 11.2e9;
        opts.spectrum_capture_db = 12.0;
        const int rc = simulate_modulated_link("256apsk", c256, 256, 8, 200000, 12.0, 6000, &opts);
        opts.spectrum_avg_windows = avg_windows;
        if (rc != 0) {
            return rc;
        }
    }

    return 0;
}
