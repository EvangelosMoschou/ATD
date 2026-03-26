#include "propagation_losses.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    PropagationConfig cfg;
    cfg.distance_m = 35786000.0;
    cfg.rain_rate_mm_per_h = 31.0;
    cfg.temperature_c = 25.0;
    cfg.water_density_g_per_m3 = 7.5;
    cfg.total_pressure_pa = 101325.0;
    cfg.effective_atm_distance_m = 10000.0;
    cfg.effective_rain_distance_m = 5000.0;

    const double f_start_ghz = 1.0;
    const double f_end_ghz = 60.0;
    const double f_step_ghz = 0.1;
    const size_t max_points = 700;

    PropagationPoint* points = (PropagationPoint*)calloc(max_points, sizeof(PropagationPoint));
    if (!points) {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    size_t count = 0;
    const int rc = compute_propagation_sweep(
        &cfg,
        f_start_ghz,
        f_end_ghz,
        f_step_ghz,
        points,
        max_points,
        &count);

    if (rc != 0) {
        fprintf(stderr, "compute_propagation_sweep failed with code %d\n", rc);
        free(points);
        return 2;
    }

    FILE* fp = fopen("propagation_results.csv", "w");
    if (!fp) {
        fprintf(stderr, "Could not open output CSV\n");
        free(points);
        return 3;
    }

    fprintf(fp, "frequency_ghz,fspl_db,gas_db,rain_db,total_db\n");
    for (size_t i = 0; i < count; ++i) {
        fprintf(fp, "%.6f,%.6f,%.6f,%.6f,%.6f\n",
                points[i].frequency_hz / 1e9,
                points[i].fspl_db,
                points[i].gas_db,
                points[i].rain_db,
                points[i].total_db);
    }

    fclose(fp);
    printf("Wrote %zu points to propagation_results.csv\n", count);

    free(points);
    return 0;
}
