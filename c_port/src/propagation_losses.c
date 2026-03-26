#include "propagation_losses.h"

#include <math.h>
#include <stddef.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define LIGHT_SPEED_M_PER_S 299792458.0

static double compute_dry_pressure_pa(double total_pressure_pa, double temperature_c, double water_density_g_per_m3) {
    const double e_vapor = (water_density_g_per_m3 * (temperature_c + 273.15)) / 2.167;
    return total_pressure_pa - e_vapor;
}

static double fspl_db(double distance_m, double frequency_hz) {
    const double ratio = (4.0 * M_PI * distance_m * frequency_hz) / LIGHT_SPEED_M_PER_S;
    return 20.0 * log10(ratio);
}

static double interp_log_linear(double x, const double* xs, const double* ys, size_t n) {
    if (x <= xs[0]) {
        return ys[0];
    }
    if (x >= xs[n - 1]) {
        return ys[n - 1];
    }

    for (size_t i = 0; i + 1 < n; ++i) {
        if (x >= xs[i] && x <= xs[i + 1]) {
            const double t = (x - xs[i]) / (xs[i + 1] - xs[i]);
            const double y0 = log10(ys[i]);
            const double y1 = log10(ys[i + 1]);
            return pow(10.0, y0 + t * (y1 - y0));
        }
    }

    return ys[n - 1];
}

static double interp_linear(double x, const double* xs, const double* ys, size_t n) {
    if (x <= xs[0]) {
        return ys[0];
    }
    if (x >= xs[n - 1]) {
        return ys[n - 1];
    }

    for (size_t i = 0; i + 1 < n; ++i) {
        if (x >= xs[i] && x <= xs[i + 1]) {
            const double t = (x - xs[i]) / (xs[i + 1] - xs[i]);
            return ys[i] + t * (ys[i + 1] - ys[i]);
        }
    }

    return ys[n - 1];
}

static double gas_attenuation_db(
    double path_m,
    double frequency_hz,
    double temperature_c,
    double dry_pressure_pa,
    double water_density_g_per_m3) {
    /*
     * Practical approximation (not a full line-by-line ITU-R P.676 implementation).
     * Keeps runtime low and behavior realistic for assignment-level sweeps.
     */
    const double f_ghz = frequency_hz / 1e9;
    const double t_k = temperature_c + 273.15;
    const double theta = 300.0 / t_k;
    const double p_kpa = dry_pressure_pa / 1000.0;

    const double o2_res = 0.0008 * p_kpa * theta * theta * (f_ghz * f_ghz) /
                          (((f_ghz - 60.0) * (f_ghz - 60.0)) + 4.0);
    const double h2o_res = 0.0006 * water_density_g_per_m3 * theta * theta * (f_ghz * f_ghz) /
                           (((f_ghz - 22.235) * (f_ghz - 22.235)) + 2.25);
    const double continuum = 0.00015 * f_ghz;

    const double gamma_db_per_km = o2_res + h2o_res + continuum;
    return gamma_db_per_km * (path_m / 1000.0);
}

static double rain_attenuation_db(double path_m, double frequency_hz, double rain_rate_mm_per_h) {
    /*
     * ITU-R P.838-inspired power-law approximation using frequency-interpolated
     * coefficients for quick feasibility studies.
     */
    static const double f_tab[] = {1.0, 4.0, 8.0, 15.0, 30.0, 60.0};
    static const double k_tab[] = {1e-6, 1e-4, 0.004, 0.02, 0.18, 0.70};
    static const double a_tab[] = {1.0, 1.0, 1.0, 1.05, 1.12, 1.20};

    const size_t n = sizeof(f_tab) / sizeof(f_tab[0]);
    const double f_ghz = frequency_hz / 1e9;
    const double k = interp_log_linear(f_ghz, f_tab, k_tab, n);
    const double alpha = interp_linear(f_ghz, f_tab, a_tab, n);

    const double gamma_db_per_km = k * pow(rain_rate_mm_per_h, alpha);
    return gamma_db_per_km * (path_m / 1000.0);
}

int compute_propagation_sweep(
    const PropagationConfig* cfg,
    double f_start_ghz,
    double f_end_ghz,
    double f_step_ghz,
    PropagationPoint* out_points,
    size_t max_points,
    size_t* out_count) {
    if (!cfg || !out_points || !out_count || f_step_ghz <= 0.0 || f_end_ghz < f_start_ghz) {
        return -1;
    }

    const double dry_pressure_pa = compute_dry_pressure_pa(
        cfg->total_pressure_pa,
        cfg->temperature_c,
        cfg->water_density_g_per_m3);

    size_t count = 0;
    for (double f_ghz = f_start_ghz; f_ghz <= f_end_ghz + 1e-12; f_ghz += f_step_ghz) {
        if (count >= max_points) {
            return -2;
        }

        const double f_hz = f_ghz * 1e9;
        const double fspl = fspl_db(cfg->distance_m, f_hz);
        const double gas = gas_attenuation_db(
            cfg->effective_atm_distance_m,
            f_hz,
            cfg->temperature_c,
            dry_pressure_pa,
            cfg->water_density_g_per_m3);
        const double rain = rain_attenuation_db(
            cfg->effective_rain_distance_m,
            f_hz,
            cfg->rain_rate_mm_per_h);

        out_points[count].frequency_hz = f_hz;
        out_points[count].fspl_db = fspl;
        out_points[count].gas_db = gas;
        out_points[count].rain_db = rain;
        out_points[count].total_db = fspl + gas + rain;
        ++count;
    }

    *out_count = count;
    return 0;
}
