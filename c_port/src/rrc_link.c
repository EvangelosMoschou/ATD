#include "rrc_link.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double urand(void) {
    return ((double)rand() + 1.0) / ((double)RAND_MAX + 2.0);
}

static double gauss_rand(void) {
    const double u1 = urand();
    const double u2 = urand();
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

static int bit_errors_uint(unsigned a, unsigned b, int bits) {
    unsigned x = a ^ b;
    int c = 0;
    for (int i = 0; i < bits; ++i) {
        c += (x >> i) & 1u;
    }
    return c;
}

static double c_dist2(Complex a, Complex b) {
    const double dr = a.re - b.re;
    const double di = a.im - b.im;
    return dr * dr + di * di;
}

static Complex c_mul(Complex a, Complex b) {
    Complex out;
    out.re = a.re * b.re - a.im * b.im;
    out.im = a.re * b.im + a.im * b.re;
    return out;
}

static Complex c_expj(double th) {
    Complex out;
    out.re = cos(th);
    out.im = sin(th);
    return out;
}

int normalize_constellation(Complex* c, int m) {
    if (!c || m <= 0) {
        return -1;
    }

    double p = 0.0;
    for (int i = 0; i < m; ++i) {
        p += c[i].re * c[i].re + c[i].im * c[i].im;
    }
    p /= (double)m;

    if (p <= 0.0) {
        return -2;
    }

    const double s = 1.0 / sqrt(p);
    for (int i = 0; i < m; ++i) {
        c[i].re *= s;
        c[i].im *= s;
    }
    return 0;
}

static int design_rrc_taps(double beta, int span, int sps, double* h) {
    if (!h || beta <= 0.0 || beta >= 1.0 || span <= 0 || sps <= 0) {
        return -1;
    }

    const int ntaps = span * sps + 1;
    const int mid = ntaps / 2;

    for (int n = 0; n < ntaps; ++n) {
        const double t = ((double)n - (double)mid) / (double)sps;

        if (fabs(t) < 1e-12) {
            h[n] = 1.0 + beta * (4.0 / M_PI - 1.0);
            continue;
        }

        if (fabs(fabs(t) - 1.0 / (4.0 * beta)) < 1e-12) {
            const double a = (1.0 + 2.0 / M_PI) * sin(M_PI / (4.0 * beta));
            const double b = (1.0 - 2.0 / M_PI) * cos(M_PI / (4.0 * beta));
            h[n] = (beta / sqrt(2.0)) * (a + b);
            continue;
        }

        const double num = sin(M_PI * t * (1.0 - beta)) +
                           4.0 * beta * t * cos(M_PI * t * (1.0 + beta));
        const double den = M_PI * t * (1.0 - (4.0 * beta * t) * (4.0 * beta * t));
        h[n] = num / den;
    }

    double energy = 0.0;
    for (int n = 0; n < ntaps; ++n) {
        energy += h[n] * h[n];
    }
    if (energy <= 0.0) {
        return -2;
    }

    const double inv = 1.0 / sqrt(energy);
    for (int n = 0; n < ntaps; ++n) {
        h[n] *= inv;
    }

    return ntaps;
}

static void conv_real_taps_complex(
    const Complex* x,
    size_t nx,
    const double* h,
    int nh,
    Complex* y) {
    const size_t ny = nx + (size_t)nh - 1u;

    for (size_t n = 0; n < ny; ++n) {
        double acc_re = 0.0;
        double acc_im = 0.0;

        const size_t k_min = (n >= (size_t)(nh - 1)) ? (n - (size_t)(nh - 1)) : 0u;
        const size_t k_max = (n < nx - 1u) ? n : (nx - 1u);

        for (size_t k = k_min; k <= k_max; ++k) {
            const int hi = (int)(n - k);
            acc_re += x[k].re * h[hi];
            acc_im += x[k].im * h[hi];
        }

        y[n].re = acc_re;
        y[n].im = acc_im;
    }
}

static void apply_frequency_shift(Complex* x, size_t n, double fs_hz, double fc_hz, int sign) {
    const double sgn = (sign >= 0) ? 1.0 : -1.0;
    for (size_t i = 0; i < n; ++i) {
        const double th = sgn * 2.0 * M_PI * fc_hz * (double)i / fs_hz;
        x[i] = c_mul(x[i], c_expj(th));
    }
}

static int export_time_domain_csv(const char* tag, const Complex* before_up, const Complex* after_up, size_t n, size_t max_samples) {
    if (!tag || !before_up || !after_up || n == 0 || max_samples == 0) {
        return -1;
    }

    char name[96];
    snprintf(name, sizeof(name), "time_domain_%s.csv", tag);

    FILE* f = fopen(name, "w");
    if (!f) {
        return -2;
    }

    const size_t nout = (n < max_samples) ? n : max_samples;
    fprintf(f, "sample,before_i,before_q,after_i,after_q\n");
    for (size_t i = 0; i < nout; ++i) {
        fprintf(f, "%zu,%.9f,%.9f,%.9f,%.9f\n", i, before_up[i].re, before_up[i].im, after_up[i].re, after_up[i].im);
    }

    fclose(f);
    printf("Wrote %s\n", name);
    return 0;
}

static int export_dual_spectrum_csv(const char* file_name, const Complex* a, const Complex* b, size_t n, int fft_len, double sample_rate_hz, double carrier_hz) {
    if (!file_name || !a || !b || fft_len <= 16 || n == 0) {
        return -1;
    }

    const int nfft = fft_len;
    if ((size_t)nfft > n) {
        return -2;
    }

    FILE* f = fopen(file_name, "w");
    if (!f) {
        return -3;
    }

    fprintf(f, "freq_ghz,a_db,b_db\n");

    for (int k = 0; k < nfft; ++k) {
        const int ks = (k + nfft / 2) % nfft;
        double a_re = 0.0;
        double a_im = 0.0;
        double b_re = 0.0;
        double b_im = 0.0;

        for (int m = 0; m < nfft; ++m) {
            const double ang = -2.0 * M_PI * (double)ks * (double)m / (double)nfft;
            const double ca = cos(ang);
            const double sa = sin(ang);

            a_re += a[m].re * ca - a[m].im * sa;
            a_im += a[m].re * sa + a[m].im * ca;

            b_re += b[m].re * ca - b[m].im * sa;
            b_im += b[m].re * sa + b[m].im * ca;
        }

        const double pa = (a_re * a_re + a_im * a_im) / (double)nfft;
        const double pb = (b_re * b_re + b_im * b_im) / (double)nfft;
        const double a_db = 10.0 * log10(pa + 1e-18);
        const double b_db = 10.0 * log10(pb + 1e-18);
        const double f_hz = (((double)k / (double)nfft) - 0.5) * sample_rate_hz + carrier_hz;
        const double f_ghz = f_hz / 1e9;

        fprintf(f, "%.9f,%.9f,%.9f\n", f_ghz, a_db, b_db);
    }

    fclose(f);
    printf("Wrote %s\n", file_name);
    return 0;
}

static int export_rrc_response_csv(const char* tag, const double* taps, int ntaps, int fft_len, double sample_rate_hz, double carrier_hz) {
    if (!tag || !taps || ntaps <= 0 || fft_len <= 16) {
        return -1;
    }

    char name[96];
    snprintf(name, sizeof(name), "rrc_response_%s.csv", tag);

    FILE* f = fopen(name, "w");
    if (!f) {
        return -2;
    }

    fprintf(f, "freq_ghz,mag_db\n");

    for (int k = 0; k < fft_len; ++k) {
        const int ks = (k + fft_len / 2) % fft_len;
        double re = 0.0;
        double im = 0.0;
        for (int n = 0; n < ntaps; ++n) {
            const double ang = -2.0 * M_PI * (double)ks * (double)n / (double)fft_len;
            re += taps[n] * cos(ang);
            im += taps[n] * sin(ang);
        }

        const double mag = sqrt(re * re + im * im);
        const double db = 20.0 * log10(mag + 1e-18);
        const double f_hz = (((double)k / (double)fft_len) - 0.5) * sample_rate_hz + carrier_hz;
        const double f_ghz = f_hz / 1e9;
        fprintf(f, "%.9f,%.9f\n", f_ghz, db);
    }

    fclose(f);
    printf("Wrote %s\n", name);
    return 0;
}

static unsigned detect_nearest(const Complex sample, const Complex* c, int m) {
    unsigned best = 0;
    double best_d = c_dist2(sample, c[0]);
    for (unsigned k = 1; k < (unsigned)m; ++k) {
        const double d = c_dist2(sample, c[k]);
        if (d < best_d) {
            best_d = d;
            best = k;
        }
    }
    return best;
}

int simulate_modulated_link(
    const char* tag,
    const Complex* constellation,
    int m,
    int bits_per_symbol,
    size_t symbols_per_snr,
    double constellation_capture_db,
    size_t constellation_points,
    const LinkOptions* options) {
    if (!options) {
        return -1;
    }

    const int sps = options->sps;
    const int filter_span_symbols = options->filter_span_symbols;
    const double rolloff = options->rolloff;

    if (!tag || !constellation || m <= 1 || bits_per_symbol <= 0 || symbols_per_snr < 10 ||
        sps <= 1 || filter_span_symbols < 2 || rolloff <= 0.0 || rolloff >= 1.0) {
        return -2;
    }

    if (options->use_rf && (options->sample_rate_hz <= 0.0 || options->carrier_hz <= 0.0)) {
        return -3;
    }

    const int ntaps = filter_span_symbols * sps + 1;
    const int group_delay = (ntaps - 1) / 2;
    const int discard_syms = filter_span_symbols;

    double* taps = (double*)calloc((size_t)ntaps, sizeof(double));
    unsigned* tx_idx = (unsigned*)calloc(symbols_per_snr, sizeof(unsigned));
    Complex* tx_syms = (Complex*)calloc(symbols_per_snr, sizeof(Complex));
    Complex* up = (Complex*)calloc(symbols_per_snr * (size_t)sps, sizeof(Complex));

    if (!taps || !tx_idx || !tx_syms || !up) {
        free(taps);
        free(tx_idx);
        free(tx_syms);
        free(up);
        return -2;
    }

    if (design_rrc_taps(rolloff, filter_span_symbols, sps, taps) < 0) {
        free(taps);
        free(tx_idx);
        free(tx_syms);
        free(up);
        return -3;
    }

    char ber_name[64];
    char const_name[64];
    snprintf(ber_name, sizeof(ber_name), "ber_%s.csv", tag);
    snprintf(const_name, sizeof(const_name), "constellation_%s.csv", tag);

    FILE* fber = fopen(ber_name, "w");
    FILE* fconst = fopen(const_name, "w");
    if (!fber || !fconst) {
        if (fber) fclose(fber);
        if (fconst) fclose(fconst);
        free(taps);
        free(tx_idx);
        free(tx_syms);
        free(up);
        return -4;
    }

    fprintf(fber, "ebn0_db,ber,bit_errors,total_bits\n");
    fprintf(fconst, "rx_i,rx_q,tx_symbol,rx_symbol\n");

    const size_t up_len = symbols_per_snr * (size_t)sps;
    const size_t tx_len = up_len + (size_t)ntaps - 1u;
    const size_t rx_len = tx_len + (size_t)ntaps - 1u;

    Complex* tx_wave = (Complex*)calloc(tx_len, sizeof(Complex));
    Complex* tx_chan = (Complex*)calloc(tx_len, sizeof(Complex));
    Complex* noisy = (Complex*)calloc(tx_len, sizeof(Complex));
    Complex* noisy_bb = (Complex*)calloc(tx_len, sizeof(Complex));
    Complex* rx_wave = (Complex*)calloc(rx_len, sizeof(Complex));

    if (!tx_wave || !tx_chan || !noisy || !noisy_bb || !rx_wave) {
        fclose(fber);
        fclose(fconst);
        free(taps);
        free(tx_idx);
        free(tx_syms);
        free(up);
        free(tx_wave);
        free(tx_chan);
        free(noisy);
        free(noisy_bb);
        free(rx_wave);
        return -6;
    }

    int diagnostics_written = 0;

    for (double ebn0_db = 0.0; ebn0_db <= 20.0 + 1e-12; ebn0_db += 2.0) {
        for (size_t n = 0; n < symbols_per_snr; ++n) {
            tx_idx[n] = (unsigned)(rand() % m);
            tx_syms[n] = constellation[tx_idx[n]];
        }

        for (size_t i = 0; i < up_len; ++i) {
            up[i].re = 0.0;
            up[i].im = 0.0;
        }
        for (size_t n = 0; n < symbols_per_snr; ++n) {
            up[n * (size_t)sps] = tx_syms[n];
        }

        conv_real_taps_complex(up, up_len, taps, ntaps, tx_wave);

        for (size_t i = 0; i < tx_len; ++i) {
            tx_chan[i] = tx_wave[i];
        }

        if (options->use_rf) {
            apply_frequency_shift(tx_chan, tx_len, options->sample_rate_hz, options->carrier_hz, +1);
        }

        const double ebn0_lin = pow(10.0, ebn0_db / 10.0);
        const double eb = 1.0 / (double)bits_per_symbol;
        const double n0 = eb / ebn0_lin;
        const double sigma = sqrt(n0 / 2.0);

        for (size_t i = 0; i < tx_len; ++i) {
            noisy[i].re = tx_chan[i].re + sigma * gauss_rand();
            noisy[i].im = tx_chan[i].im + sigma * gauss_rand();
        }

        for (size_t i = 0; i < tx_len; ++i) {
            noisy_bb[i] = noisy[i];
        }
        if (options->use_rf) {
            apply_frequency_shift(noisy_bb, tx_len, options->sample_rate_hz, options->carrier_hz, -1);
        }

        conv_real_taps_complex(noisy_bb, tx_len, taps, ntaps, rx_wave);

        if (options->export_spectrum && !diagnostics_written && fabs(ebn0_db - options->spectrum_capture_db) < 1e-12) {
            char spec_before_name[96];
            char spec_after_name[96];
            snprintf(spec_before_name, sizeof(spec_before_name), "spectrum_before_upconversion_%s.csv", tag);
            snprintf(spec_after_name, sizeof(spec_after_name), "spectrum_after_upconversion_%s.csv", tag);

            const int rc_time = export_time_domain_csv(tag, tx_wave, tx_chan, tx_len, 2048);
            const double fs_hz = (options->sample_rate_hz > 0.0) ? options->sample_rate_hz : 1.0;
            const double fc_hz = (options->use_rf && options->carrier_hz > 0.0) ? options->carrier_hz : 0.0;
            const int rc_pre = export_dual_spectrum_csv(spec_before_name, tx_wave, noisy_bb, tx_len, options->spectrum_fft_len, fs_hz, fc_hz);
            const int rc_post = export_dual_spectrum_csv(spec_after_name, tx_chan, noisy, tx_len, options->spectrum_fft_len, fs_hz, fc_hz);
            const int rc_rrc = export_rrc_response_csv(tag, taps, ntaps, options->spectrum_fft_len, fs_hz, fc_hz);

            if (rc_time != 0 || rc_pre != 0 || rc_post != 0 || rc_rrc != 0) {
                fclose(fber);
                fclose(fconst);
                free(taps);
                free(tx_idx);
                free(tx_syms);
                free(up);
                free(tx_wave);
                free(tx_chan);
                free(noisy);
                free(noisy_bb);
                free(rx_wave);
                return -7;
            }
            diagnostics_written = 1;
        }

        unsigned long long bit_errors = 0;
        unsigned long long total_bits = 0;
        size_t const_written = 0;

        const size_t start = (size_t)(2 * group_delay);

        for (size_t n = (size_t)discard_syms; n + (size_t)discard_syms < symbols_per_snr; ++n) {
            const size_t idx = start + n * (size_t)sps;
            if (idx >= rx_len) {
                break;
            }

            const Complex r = rx_wave[idx];
            const unsigned rx_sym = detect_nearest(r, constellation, m);
            const unsigned tx_sym = tx_idx[n];

            bit_errors += (unsigned long long)bit_errors_uint(tx_sym, rx_sym, bits_per_symbol);
            total_bits += (unsigned long long)bits_per_symbol;

            if (fabs(ebn0_db - constellation_capture_db) < 1e-12 && const_written < constellation_points) {
                fprintf(fconst, "%.8f,%.8f,%u,%u\n", r.re, r.im, tx_sym, rx_sym);
                ++const_written;
            }
        }

        if (total_bits == 0) {
            fclose(fber);
            fclose(fconst);
            free(taps);
            free(tx_idx);
            free(tx_syms);
            free(up);
            free(tx_wave);
            free(tx_chan);
            free(noisy);
            free(noisy_bb);
            free(rx_wave);
            return -8;
        }

        const double ber = (double)bit_errors / (double)total_bits;
        fprintf(fber, "%.2f,%.12g,%llu,%llu\n", ebn0_db, ber, bit_errors, total_bits);
        printf("%s Eb/N0=%.2f dB -> BER=%.6g (%llu/%llu)\n", tag, ebn0_db, ber, bit_errors, total_bits);
    }

    fclose(fber);
    fclose(fconst);

    free(taps);
    free(tx_idx);
    free(tx_syms);
    free(up);
    free(tx_wave);
    free(tx_chan);
    free(noisy);
    free(noisy_bb);
    free(rx_wave);

    printf("Wrote %s and %s\n", ber_name, const_name);
    return 0;
}
