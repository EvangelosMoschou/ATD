#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_BER_POINTS 256
#define MAX_CONST_POINTS 10000

typedef struct BerPoint {
    double ebn0_db;
    double ber;
} BerPoint;

typedef struct IQPoint {
    double i;
    double q;
} IQPoint;

static double clamp(double v, double lo, double hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static int load_ber(BerPoint* out, size_t* out_n) {
    FILE* f = fopen("ber_8psk.csv", "r");
    if (!f) {
        return -1;
    }

    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -2;
    }

    size_t n = 0;
    while (fgets(line, sizeof(line), f)) {
        if (n >= MAX_BER_POINTS) {
            fclose(f);
            return -3;
        }

        BerPoint p;
        unsigned long long be = 0;
        unsigned long long tb = 0;
        if (sscanf(line, "%lf,%lf,%llu,%llu", &p.ebn0_db, &p.ber, &be, &tb) == 4) {
            out[n++] = p;
        }
    }

    fclose(f);
    *out_n = n;
    return n > 0 ? 0 : -4;
}

static int load_const(IQPoint* out, size_t* out_n) {
    FILE* f = fopen("constellation_8psk.csv", "r");
    if (!f) {
        return -1;
    }

    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -2;
    }

    size_t n = 0;
    while (fgets(line, sizeof(line), f)) {
        if (n >= MAX_CONST_POINTS) {
            break;
        }

        IQPoint p;
        unsigned tx = 0;
        unsigned rx = 0;
        if (sscanf(line, "%lf,%lf,%u,%u", &p.i, &p.q, &tx, &rx) == 4) {
            out[n++] = p;
        }
    }

    fclose(f);
    *out_n = n;
    return n > 0 ? 0 : -4;
}

static int write_ber_svg(const BerPoint* pts, size_t n) {
    const int width = 1100;
    const int height = 650;
    const int ml = 90;
    const int mr = 30;
    const int mt = 40;
    const int mb = 80;

    const double x0 = ml;
    const double x1 = width - mr;
    const double y0 = height - mb;
    const double y1 = mt;

    double xmin = DBL_MAX, xmax = -DBL_MAX;
    double ymin = DBL_MAX, ymax = -DBL_MAX;

    for (size_t i = 0; i < n; ++i) {
        xmin = fmin(xmin, pts[i].ebn0_db);
        xmax = fmax(xmax, pts[i].ebn0_db);
        const double y = log10(fmax(pts[i].ber, 1e-12));
        ymin = fmin(ymin, y);
        ymax = fmax(ymax, y);
    }

    ymin = floor(ymin);
    ymax = ceil(ymax);

    FILE* f = fopen("ber_8psk.svg", "w");
    if (!f) {
        return -1;
    }

    fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n", width, height, width, height);
    fprintf(f, "<rect width=\"100%%\" height=\"100%%\" fill=\"#f9fbfd\"/>\n");
    fprintf(f, "<text x=\"90\" y=\"30\" font-family=\"sans-serif\" font-size=\"24\" fill=\"#1f2937\">8-PSK BER Curve (AWGN)</text>\n");

    fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#111827\" stroke-width=\"1.5\"/>\n", x0, y0, x1, y0);
    fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#111827\" stroke-width=\"1.5\"/>\n", x0, y0, x0, y1);

    for (int i = 0; i <= 5; ++i) {
        const double xv = xmin + (xmax - xmin) * i / 5.0;
        const double x = x0 + (xv - xmin) / (xmax - xmin) * (x1 - x0);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#d1d5db\"/>\n", x, y0, x, y1);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"13\">%.1f dB</text>\n", x, y0 + 22.0, xv);
    }

    for (int p = (int)ymin; p <= (int)ymax; ++p) {
        const double y = y0 - ((double)p - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#e5e7eb\"/>\n", x0, y, x1, y);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"end\" font-family=\"sans-serif\" font-size=\"13\">1e%d</text>\n", x0 - 8.0, y + 4.0, p);
    }

    if (-3.0 >= ymin && -3.0 <= ymax) {
        const double y_ref = y0 - ((-3.0) - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#ef4444\" stroke-dasharray=\"6,4\" stroke-width=\"1.4\"/>\n", x0, y_ref, x1, y_ref);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" font-family=\"sans-serif\" font-size=\"12\" fill=\"#ef4444\">BER = 1e-3</text>\n", x0 + 8.0, y_ref - 6.0);
    }

    if (10.0 >= xmin && 10.0 <= xmax) {
        const double x_ref = x0 + (10.0 - xmin) / (xmax - xmin) * (x1 - x0);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#f59e0b\" stroke-dasharray=\"6,4\" stroke-width=\"1.4\"/>\n", x_ref, y0, x_ref, y1);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"12\" fill=\"#f59e0b\">10 dB</text>\n", x_ref, y1 + 14.0);
    }

    fprintf(f, "<polyline fill=\"none\" stroke=\"#0f766e\" stroke-width=\"2.5\" points=\"");
    for (size_t i = 0; i < n; ++i) {
        const double x = x0 + (pts[i].ebn0_db - xmin) / (xmax - xmin) * (x1 - x0);
        const double yv = log10(fmax(pts[i].ber, 1e-12));
        const double y = y0 - (yv - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "%.2f,%.2f ", x, y);
    }
    fprintf(f, "\"/>\n");

    fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"15\">Eb/N0 (dB)</text>\n", (x0 + x1) * 0.5, y0 + 52.0);
    fprintf(f, "<text transform=\"translate(22,%.2f) rotate(-90)\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"15\">BER (log10 scale)</text>\n", (y0 + y1) * 0.5);

    fprintf(f, "</svg>\n");
    fclose(f);
    return 0;
}

static int write_constellation_svg(const IQPoint* pts, size_t n) {
    const int width = 760;
    const int height = 760;
    const int margin = 70;

    const double xmin = -2.0;
    const double xmax = 2.0;
    const double ymin = -2.0;
    const double ymax = 2.0;

    FILE* f = fopen("constellation_8psk.svg", "w");
    if (!f) {
        return -1;
    }

    fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n", width, height, width, height);
    fprintf(f, "<rect width=\"100%%\" height=\"100%%\" fill=\"#fbfcfe\"/>\n");
    fprintf(f, "<text x=\"70\" y=\"30\" font-family=\"sans-serif\" font-size=\"24\" fill=\"#1f2937\">8-PSK Constellation (Rx Samples)</text>\n");

    const double x0 = margin;
    const double x1 = width - margin;
    const double y0 = height - margin;
    const double y1 = margin;

    const double x_axis = x0 + (0.0 - xmin) / (xmax - xmin) * (x1 - x0);
    const double y_axis = y0 - (0.0 - ymin) / (ymax - ymin) * (y0 - y1);

    fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#9ca3af\"/>\n", x0, y_axis, x1, y_axis);
    fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#9ca3af\"/>\n", x_axis, y0, x_axis, y1);

    for (int t = -2; t <= 2; ++t) {
        const double xv = (double)t;
        const double x = x0 + (xv - xmin) / (xmax - xmin) * (x1 - x0);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#e5e7eb\"/>\n", x, y0, x, y1);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"12\">%d</text>\n", x, y0 + 20.0, t);
    }
    for (int t = -2; t <= 2; ++t) {
        const double yv = (double)t;
        const double y = y0 - (yv - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#e5e7eb\"/>\n", x0, y, x1, y);
        fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"end\" font-family=\"sans-serif\" font-size=\"12\">%d</text>\n", x0 - 6.0, y + 4.0, t);
    }

    for (size_t i = 0; i < n; ++i) {
        const double x = x0 + (clamp(pts[i].i, xmin, xmax) - xmin) / (xmax - xmin) * (x1 - x0);
        const double y = y0 - (clamp(pts[i].q, ymin, ymax) - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"1.6\" fill=\"#2563eb\" fill-opacity=\"0.45\"/>\n", x, y);
    }

    for (int k = 0; k < 8; ++k) {
        const double th = 2.0 * 3.14159265358979323846 * (double)k / 8.0;
        const double i_ref = cos(th);
        const double q_ref = sin(th);
        const double x = x0 + (i_ref - xmin) / (xmax - xmin) * (x1 - x0);
        const double y = y0 - (q_ref - ymin) / (ymax - ymin) * (y0 - y1);
        fprintf(f, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"4.0\" fill=\"#dc2626\"/>\n", x, y);
    }

    fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"14\">In-Phase (I)</text>\n", (x0 + x1) * 0.5, y0 + 36.0);
    fprintf(f, "<text transform=\"translate(24,%.2f) rotate(-90)\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"14\">Quadrature (Q)</text>\n", (y0 + y1) * 0.5);

    fprintf(f, "</svg>\n");
    fclose(f);
    return 0;
}

int main(void) {
    BerPoint ber[MAX_BER_POINTS];
    IQPoint iq[MAX_CONST_POINTS];
    size_t nber = 0;
    size_t niq = 0;

    if (load_ber(ber, &nber) != 0) {
        fprintf(stderr, "Failed to read ber_8psk.csv\n");
        return 1;
    }

    if (load_const(iq, &niq) != 0) {
        fprintf(stderr, "Failed to read constellation_8psk.csv\n");
        return 2;
    }

    if (write_ber_svg(ber, nber) != 0) {
        fprintf(stderr, "Failed to write ber_8psk.svg\n");
        return 3;
    }

    if (write_constellation_svg(iq, niq) != 0) {
        fprintf(stderr, "Failed to write constellation_8psk.svg\n");
        return 4;
    }

    printf("Wrote ber_8psk.svg and constellation_8psk.svg\n");
    return 0;
}
