#ifndef RRC_LINK_H
#define RRC_LINK_H

#include <stddef.h>

typedef struct Complex {
    double re;
    double im;
} Complex;

typedef struct LinkOptions {
    int sps;
    int filter_span_symbols;
    double rolloff;
    int use_rf;
    double sample_rate_hz;
    double carrier_hz;
    int export_spectrum;
    double spectrum_capture_db;
    int spectrum_fft_len;
} LinkOptions;

int normalize_constellation(Complex* c, int m);

int simulate_modulated_link(
    const char* tag,
    const Complex* constellation,
    int m,
    int bits_per_symbol,
    size_t symbols_per_snr,
    double constellation_capture_db,
    size_t constellation_points,
    const LinkOptions* options);

#endif
