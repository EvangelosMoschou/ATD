#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_POINTS 5000

typedef struct Point {
    double f_ghz;
    double fspl;
    double gas;
    double rain;
    double total;
} Point;

static double clamp(double v, double lo, double hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

int main(void) {
    FILE* in = fopen("propagation_results.csv", "r");
    if (!in) {
        fprintf(stderr, "Could not open propagation_results.csv\n");
        return 1;
    }

    Point* pts = (Point*)calloc(MAX_POINTS, sizeof(Point));
    if (!pts) {
        fprintf(stderr, "Allocation failed\n");
        fclose(in);
        return 2;
    }

    char line[512];
    if (!fgets(line, sizeof(line), in)) {
        fprintf(stderr, "CSV appears empty\n");
        free(pts);
        fclose(in);
        return 3;
    }

    size_t n = 0;
    double min_x = DBL_MAX, max_x = -DBL_MAX;
    double min_y = DBL_MAX, max_y = -DBL_MAX;

    while (fgets(line, sizeof(line), in)) {
        if (n >= MAX_POINTS) {
            fprintf(stderr, "Too many points (max=%d)\n", MAX_POINTS);
            free(pts);
            fclose(in);
            return 4;
        }

        Point p;
        if (sscanf(line, "%lf,%lf,%lf,%lf,%lf", &p.f_ghz, &p.fspl, &p.gas, &p.rain, &p.total) != 5) {
            continue;
        }

        pts[n++] = p;

        if (p.f_ghz < min_x) min_x = p.f_ghz;
        if (p.f_ghz > max_x) max_x = p.f_ghz;

        const double y2 = p.fspl + p.gas;
        if (p.fspl < min_y) min_y = p.fspl;
        if (p.total < min_y) min_y = p.total;
        if (y2 < min_y) min_y = y2;

        if (p.fspl > max_y) max_y = p.fspl;
        if (p.total > max_y) max_y = p.total;
        if (y2 > max_y) max_y = y2;
    }
    fclose(in);

    if (n == 0) {
        fprintf(stderr, "No data points parsed\n");
        free(pts);
        return 5;
    }

    const int width = 1200;
    const int height = 700;
    const int margin_left = 90;
    const int margin_right = 30;
    const int margin_top = 30;
    const int margin_bottom = 70;

    const double plot_w = (double)(width - margin_left - margin_right);
    const double plot_h = (double)(height - margin_top - margin_bottom);

    const double y_pad = fmax(1.0, 0.05 * (max_y - min_y));
    min_y -= y_pad;
    max_y += y_pad;

    FILE* out = fopen("propagation_losses.svg", "w");
    if (!out) {
        fprintf(stderr, "Could not create propagation_losses.svg\n");
        free(pts);
        return 6;
    }

    fprintf(out,
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n",
            width, height, width, height);
    fprintf(out, "<rect width=\"100%%\" height=\"100%%\" fill=\"#f9fbfd\"/>\n");
    fprintf(out, "<text x=\"%d\" y=\"%d\" font-family=\"sans-serif\" font-size=\"24\" fill=\"#1f2937\">Earth-to-GEO Propagation Loss (1-60 GHz)</text>\n", 90, 28);

    const double x0 = margin_left;
    const double y0 = margin_top + plot_h;
    const double x1 = margin_left + plot_w;
    const double y1 = margin_top;

    fprintf(out, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#111827\" stroke-width=\"1.5\"/>\n", x0, y0, x1, y0);
    fprintf(out, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#111827\" stroke-width=\"1.5\"/>\n", x0, y0, x0, y1);

    for (int i = 0; i <= 6; ++i) {
        const double fx = min_x + (max_x - min_x) * i / 6.0;
        const double x = x0 + (fx - min_x) / (max_x - min_x) * plot_w;
        fprintf(out, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#d1d5db\" stroke-width=\"1\"/>\n", x, y0, x, y1);
        fprintf(out, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"13\" fill=\"#374151\">%.1f GHz</text>\n", x, y0 + 22.0, fx);
    }

    for (int i = 0; i <= 6; ++i) {
        const double fy = min_y + (max_y - min_y) * i / 6.0;
        const double y = y0 - (fy - min_y) / (max_y - min_y) * plot_h;
        fprintf(out, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#e5e7eb\" stroke-width=\"1\"/>\n", x0, y, x1, y);
        fprintf(out, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"end\" font-family=\"sans-serif\" font-size=\"13\" fill=\"#374151\">%.1f dB</text>\n", x0 - 8.0, y + 4.0, fy);
    }

    if (30.0 >= min_x && 30.0 <= max_x) {
        const double x_ref = x0 + (30.0 - min_x) / (max_x - min_x) * plot_w;
        fprintf(out, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#f59e0b\" stroke-dasharray=\"6,4\" stroke-width=\"1.4\"/>\n", x_ref, y0, x_ref, y1);
        fprintf(out, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"12\" fill=\"#f59e0b\">30 GHz</text>\n", x_ref, y1 + 14.0);
    }

    if (200.0 >= min_y && 200.0 <= max_y) {
        const double y_ref = y0 - (200.0 - min_y) / (max_y - min_y) * plot_h;
        fprintf(out, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#ef4444\" stroke-dasharray=\"6,4\" stroke-width=\"1.4\"/>\n", x0, y_ref, x1, y_ref);
        fprintf(out, "<text x=\"%.2f\" y=\"%.2f\" font-family=\"sans-serif\" font-size=\"12\" fill=\"#ef4444\">200 dB</text>\n", x0 + 8.0, y_ref - 6.0);
    }

    fprintf(out, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"15\" fill=\"#111827\">Frequency (GHz)</text>\n", (x0 + x1) * 0.5, y0 + 50.0);
    fprintf(out, "<text transform=\"translate(22,%.2f) rotate(-90)\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"15\" fill=\"#111827\">Loss (dB)</text>\n", (y0 + y1) * 0.5);

    const char* colors[3] = {"#f97316", "#0ea5e9", "#6b7280"};

    for (int series = 0; series < 3; ++series) {
        fprintf(out, "<polyline fill=\"none\" stroke=\"%s\" stroke-width=\"2.5\" points=\"", colors[series]);
        for (size_t i = 0; i < n; ++i) {
            const double x = x0 + (pts[i].f_ghz - min_x) / (max_x - min_x) * plot_w;
            double yv;
            if (series == 0) {
                yv = pts[i].fspl;
            } else if (series == 1) {
                yv = pts[i].fspl + pts[i].gas;
            } else {
                yv = pts[i].total;
            }
            const double y = y0 - (clamp(yv, min_y, max_y) - min_y) / (max_y - min_y) * plot_h;
            fprintf(out, "%.2f,%.2f ", x, y);
        }
        fprintf(out, "\"/>\n");
    }

    const double lx = x1 - 260.0;
    const double ly = y1 + 12.0;
    fprintf(out, "<rect x=\"%.2f\" y=\"%.2f\" width=\"240\" height=\"88\" fill=\"white\" stroke=\"#d1d5db\"/>\n", lx, ly);

    fprintf(out, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#f97316\" stroke-width=\"2.5\"/>\n", lx + 12.0, ly + 24.0, lx + 52.0, ly + 24.0);
    fprintf(out, "<text x=\"%.2f\" y=\"%.2f\" font-family=\"sans-serif\" font-size=\"13\" fill=\"#111827\">FSPL</text>\n", lx + 60.0, ly + 28.0);

    fprintf(out, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#0ea5e9\" stroke-width=\"2.5\"/>\n", lx + 12.0, ly + 46.0, lx + 52.0, ly + 46.0);
    fprintf(out, "<text x=\"%.2f\" y=\"%.2f\" font-family=\"sans-serif\" font-size=\"13\" fill=\"#111827\">FSPL + Gas</text>\n", lx + 60.0, ly + 50.0);

    fprintf(out, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#6b7280\" stroke-width=\"2.5\"/>\n", lx + 12.0, ly + 68.0, lx + 52.0, ly + 68.0);
    fprintf(out, "<text x=\"%.2f\" y=\"%.2f\" font-family=\"sans-serif\" font-size=\"13\" fill=\"#111827\">Total (FSPL+Gas+Rain)</text>\n", lx + 60.0, ly + 72.0);

    fprintf(out, "</svg>\n");

    fclose(out);
    free(pts);

    printf("Wrote propagation_losses.svg with %zu points\n", n);
    return 0;
}
