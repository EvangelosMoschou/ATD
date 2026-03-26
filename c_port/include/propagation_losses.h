#ifndef PROPAGATION_LOSSES_H
#define PROPAGATION_LOSSES_H

#include <stddef.h>

typedef struct PropagationConfig {
    double distance_m;
    double rain_rate_mm_per_h;
    double temperature_c;
    double water_density_g_per_m3;
    double total_pressure_pa;
    double effective_atm_distance_m;
    double effective_rain_distance_m;
} PropagationConfig;

typedef struct PropagationPoint {
    double frequency_hz;
    double fspl_db;
    double gas_db;
    double rain_db;
    double total_db;
} PropagationPoint;

int compute_propagation_sweep(
    const PropagationConfig* cfg,
    double f_start_ghz,
    double f_end_ghz,
    double f_step_ghz,
    PropagationPoint* out_points,
    size_t max_points,
    size_t* out_count);

#endif
