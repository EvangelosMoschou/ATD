#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POINTS 8192

typedef struct PairPoint {
    double x;
    double y1;
    double y2;
} PairPoint;

typedef struct ScalarPoint {
    double x;
    double y;
} ScalarPoint;

static int load_pair_csv(const char* file_name, PairPoint* out, size_t* out_n) {
    FILE* f = fopen(file_name, "r");
    if (!f) return -1;

    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -2;
    }

    size_t n = 0;
    while (fgets(line, sizeof(line), f)) {
        if (n >= MAX_POINTS) {
            fclose(f);
            return -3;
        }
        PairPoint p;
        if (sscanf(line, "%lf,%lf,%lf", &p.x, &p.y1, &p.y2) == 3) {
            out[n++] = p;
        }
    }
    fclose(f);
    *out_n = n;
    return n > 0 ? 0 : -4;
}

static int load_time_csv(const char* file_name, PairPoint* out, size_t* out_n) {
    FILE* f = fopen(file_name, "r");
    if (!f) return -1;

    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -2;
    }

    size_t n = 0;
    while (fgets(line, sizeof(line), f)) {
        if (n >= MAX_POINTS) {
            fclose(f);
            return -3;
        }
        PairPoint p;
        double bq = 0.0;
        double aq = 0.0;
        if (sscanf(line, "%lf,%lf,%lf,%lf,%lf", &p.x, &p.y1, &bq, &p.y2, &aq) == 5) {
            out[n++] = p;
        }
    }
    fclose(f);
    *out_n = n;
    return n > 0 ? 0 : -4;
}

static int load_scalar_csv(const char* file_name, ScalarPoint* out, size_t* out_n) {
    FILE* f = fopen(file_name, "r");
    if (!f) return -1;

    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -2;
    }

    size_t n = 0;
    while (fgets(line, sizeof(line), f)) {
        if (n >= MAX_POINTS) {
            fclose(f);
            return -3;
        }
        ScalarPoint p;
        if (sscanf(line, "%lf,%lf", &p.x, &p.y) == 2) {
            out[n++] = p;
        }
    }
    fclose(f);
    *out_n = n;
    return n > 0 ? 0 : -4;
}

static int write_pair_svg(const char* out_name, const char* title, const char* x_label, const char* y_label, const char* l1, const char* l2, const PairPoint* pts, size_t n) {
    const int width = 1150;
    const int height = 680;
    const int ml = 95;
    const int mr = 30;
    const int mt = 40;
    const int mb = 85;

    const double x0 = ml;
    const double x1 = width - mr;
    const double y0 = height - mb;
    const double y1 = mt;

    double xmin = DBL_MAX, xmax = -DBL_MAX;
    double ymin = DBL_MAX, ymax = -DBL_MAX;

    for (size_t i = 0; i < n; ++i) {
        if (pts[i].x < xmin) xmin = pts[i].x;
        if (pts[i].x > xmax) xmax = pts[i].x;
        if (pts[i].y1 < ymin) ymin = pts[i].y1;
        if (pts[i].y2 < ymin) ymin = pts[i].y2;
        if (pts[i].y1 > ymax) ymax = pts[i].y1;
        if (pts[i].y2 > ymax) ymax = pts[i].y2;
    }

    ymin -= 5.0;
    ymax += 5.0;

    FILE* f = fopen(out_name, "w");
    if (!f) return -1;

    fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n", width, height, width, height);
    fprintf(f, "<rect width=\"100%%\" height=\"100%%\" fill=\"#f9fbfd\"/>\n");
    fprintf(f, "<text x=\"95\" y=\"30\" font-family=\"sans-serif\" font-size=\"24\" fill=\"#1f2937\">%s</text>\n", title);

    fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#111827\" stroke-width=\"1.5\"/>\n", x0, y0, x1, y0);
    fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#111827\" stroke-width=\"1.5\"/>\n", x0, y0, x0, y1);

    for (int i = 0; i <= 8; ++i) {
        const double xv = xmin + (xmax - xmin) * i / 8.0;
        const double x = x0 + (xv - xmin) / (xmax - xmin) * (x1 - x0);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#e5e7eb\"/>\n", x, y0, x, y1);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"12\">%.2f</text>\n", x, y0 + 20.0, xv);
    }
    for (int i = 0; i <= 8; ++i) {
        const double yv = ymin + (ymax - ymin) * i / 8.0;
        const double y = y0 - (yv - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#e5e7eb\"/>\n", x0, y, x1, y);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"end\" font-family=\"sans-serif\" font-size=\"12\">%.1f</text>\n", x0 - 8.0, y + 4.0, yv);
    }

    if (0.0 >= xmin && 0.0 <= xmax) {
        const double xr = x0 + (0.0 - xmin) / (xmax - xmin) * (x1 - x0);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#6b7280\" stroke-dasharray=\"6,4\" stroke-width=\"1.4\"/>\n", xr, y0, xr, y1);
    }
    if (0.0 >= ymin && 0.0 <= ymax) {
        const double yr = y0 - (0.0 - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#6b7280\" stroke-dasharray=\"6,4\" stroke-width=\"1.4\"/>\n", x0, yr, x1, yr);
    }
    if (-3.0 >= ymin && -3.0 <= ymax) {
        const double yr = y0 - ((-3.0) - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#ef4444\" stroke-dasharray=\"5,4\" stroke-width=\"1.3\"/>\n", x0, yr, x1, yr);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" font-family=\"sans-serif\" font-size=\"12\" fill=\"#ef4444\">-3 dB</text>\n", x0 + 8.0, yr - 6.0);
    }

    fprintf(f, "<polyline fill=\"none\" stroke=\"#ea580c\" stroke-width=\"2.2\" points=\"");
    for (size_t i = 0; i < n; ++i) {
        const double x = x0 + (pts[i].x - xmin) / (xmax - xmin) * (x1 - x0);
        const double y = y0 - (pts[i].y1 - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "%.2f,%.2f ", x, y);
    }
    fprintf(f, "\"/>\n");

    fprintf(f, "<polyline fill=\"none\" stroke=\"#0ea5e9\" stroke-width=\"2.2\" points=\"");
    for (size_t i = 0; i < n; ++i) {
        const double x = x0 + (pts[i].x - xmin) / (xmax - xmin) * (x1 - x0);
        const double y = y0 - (pts[i].y2 - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "%.2f,%.2f ", x, y);
    }
    fprintf(f, "\"/>\n");

    fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"15\">%s</text>\n", (x0 + x1) * 0.5, y0 + 50.0, x_label);
    fprintf(f, "<text transform=\"translate(24,%.2f) rotate(-90)\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"15\">%s</text>\n", (y0 + y1) * 0.5, y_label);

    fprintf(f, "<rect x=\"%.2f\" y=\"%.2f\" width=\"250\" height=\"70\" fill=\"white\" stroke=\"#d1d5db\"/>\n", x1 - 270.0, y1 + 10.0);
    fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#ea580c\" stroke-width=\"2.2\"/>\n", x1 - 255.0, y1 + 30.0, x1 - 215.0, y1 + 30.0);
    fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" font-family=\"sans-serif\" font-size=\"13\">%s</text>\n", x1 - 206.0, y1 + 34.0, l1);
    fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#0ea5e9\" stroke-width=\"2.2\"/>\n", x1 - 255.0, y1 + 52.0, x1 - 215.0, y1 + 52.0);
    fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" font-family=\"sans-serif\" font-size=\"13\">%s</text>\n", x1 - 206.0, y1 + 56.0, l2);

    fprintf(f, "</svg>\n");
    fclose(f);
    printf("Wrote %s\n", out_name);
    return 0;
}

static int write_scalar_svg(const char* out_name, const char* title, const char* x_label, const char* y_label, const ScalarPoint* pts, size_t n) {
    const int width = 1150;
    const int height = 680;
    const int ml = 95;
    const int mr = 30;
    const int mt = 40;
    const int mb = 85;

    const double x0 = ml;
    const double x1 = width - mr;
    const double y0 = height - mb;
    const double y1 = mt;

    double xmin = DBL_MAX, xmax = -DBL_MAX;
    double ymin = DBL_MAX, ymax = -DBL_MAX;

    for (size_t i = 0; i < n; ++i) {
        if (pts[i].x < xmin) xmin = pts[i].x;
        if (pts[i].x > xmax) xmax = pts[i].x;
        if (pts[i].y < ymin) ymin = pts[i].y;
        if (pts[i].y > ymax) ymax = pts[i].y;
    }
    ymin -= 5.0;
    ymax += 5.0;

    FILE* f = fopen(out_name, "w");
    if (!f) return -1;

    fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n", width, height, width, height);
    fprintf(f, "<rect width=\"100%%\" height=\"100%%\" fill=\"#f9fbfd\"/>\n");
    fprintf(f, "<text x=\"95\" y=\"30\" font-family=\"sans-serif\" font-size=\"24\" fill=\"#1f2937\">%s</text>\n", title);
    fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#111827\" stroke-width=\"1.5\"/>\n", x0, y0, x1, y0);
    fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#111827\" stroke-width=\"1.5\"/>\n", x0, y0, x0, y1);

    for (int i = 0; i <= 8; ++i) {
        const double xv = xmin + (xmax - xmin) * i / 8.0;
        const double x = x0 + (xv - xmin) / (xmax - xmin) * (x1 - x0);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#e5e7eb\"/>\n", x, y0, x, y1);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"12\">%.2f</text>\n", x, y0 + 20.0, xv);
    }
    for (int i = 0; i <= 8; ++i) {
        const double yv = ymin + (ymax - ymin) * i / 8.0;
        const double y = y0 - (yv - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#e5e7eb\"/>\n", x0, y, x1, y);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"end\" font-family=\"sans-serif\" font-size=\"12\">%.1f dB</text>\n", x0 - 8.0, y + 4.0, yv);
    }

    if (0.0 >= xmin && 0.0 <= xmax) {
        const double xr = x0 + (0.0 - xmin) / (xmax - xmin) * (x1 - x0);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#6b7280\" stroke-dasharray=\"6,4\" stroke-width=\"1.4\"/>\n", xr, y0, xr, y1);
    }
    if (-3.0 >= ymin && -3.0 <= ymax) {
        const double yr = y0 - ((-3.0) - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#ef4444\" stroke-dasharray=\"5,4\" stroke-width=\"1.3\"/>\n", x0, yr, x1, yr);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" font-family=\"sans-serif\" font-size=\"12\" fill=\"#ef4444\">-3 dB</text>\n", x0 + 8.0, yr - 6.0);
    }

    fprintf(f, "<polyline fill=\"none\" stroke=\"#0f766e\" stroke-width=\"2.2\" points=\"");
    for (size_t i = 0; i < n; ++i) {
        const double x = x0 + (pts[i].x - xmin) / (xmax - xmin) * (x1 - x0);
        const double y = y0 - (pts[i].y - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "%.2f,%.2f ", x, y);
    }
    fprintf(f, "\"/>\n");

    fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"15\">%s</text>\n", (x0 + x1) * 0.5, y0 + 50.0, x_label);
    fprintf(f, "<text transform=\"translate(24,%.2f) rotate(-90)\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"15\">%s</text>\n", (y0 + y1) * 0.5, y_label);
    fprintf(f, "</svg>\n");
    fclose(f);
    printf("Wrote %s\n", out_name);
    return 0;
}

static int process_tag(const char* tag, const char* title) {
    PairPoint pair_pts[MAX_POINTS];
    ScalarPoint scalar_pts[MAX_POINTS];
    size_t n = 0;

    char in_name[128];
    char out_name[128];
    snprintf(in_name, sizeof(in_name), "spectrum_before_upconversion_%s.csv", tag);
    if (load_pair_csv(in_name, pair_pts, &n) != 0) return 1;
    snprintf(out_name, sizeof(out_name), "spectrum_before_upconversion_%s.svg", tag);
    if (write_pair_svg(out_name, "Spectrum Before Upconversion", "Frequency (GHz)", "Power (dB)", "Tx pre-upconversion", "Rx pre-upconversion", pair_pts, n) != 0) return 2;

    snprintf(in_name, sizeof(in_name), "spectrum_after_upconversion_%s.csv", tag);
    if (load_pair_csv(in_name, pair_pts, &n) != 0) return 3;
    snprintf(out_name, sizeof(out_name), "spectrum_after_upconversion_%s.svg", tag);
    if (write_pair_svg(out_name, "Spectrum After Upconversion", "Frequency (GHz)", "Power (dB)", "Tx RF", "Rx RF channel", pair_pts, n) != 0) return 4;

    snprintf(in_name, sizeof(in_name), "time_domain_%s.csv", tag);
    if (load_time_csv(in_name, pair_pts, &n) != 0) return 5;
    snprintf(out_name, sizeof(out_name), "time_domain_upconversion_%s.svg", tag);
    if (write_pair_svg(out_name, "Time Domain Before/After Upconversion", "Sample", "Amplitude", "Before upconversion (I)", "After upconversion (I)", pair_pts, n) != 0) return 6;

    snprintf(in_name, sizeof(in_name), "rrc_response_%s.csv", tag);
    if (load_scalar_csv(in_name, scalar_pts, &n) != 0) return 7;
    snprintf(out_name, sizeof(out_name), "rrc_response_%s.svg", tag);
    if (write_scalar_svg(out_name, "Raised Cosine Filter Response", "Frequency (GHz)", "Magnitude (dB)", scalar_pts, n) != 0) return 8;

    printf("Generated diagnostics for %s (%s)\n", tag, title);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [8|64|256]\n", argv[0]);
        return 2;
    }

    if (strcmp(argv[1], "8") == 0) return process_tag("8psk", "8-PSK");
    if (strcmp(argv[1], "64") == 0) return process_tag("64apsk", "64-APSK");
    if (strcmp(argv[1], "256") == 0) return process_tag("256apsk", "256-APSK");

    fprintf(stderr, "Usage: %s [8|64|256]\n", argv[0]);
    return 2;
}
