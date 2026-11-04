#include "px_draw.h"

#include <stdbool.h>

#define COORD_MAX 4096 /* far enough off any canvas to keep a slope, near enough for 32-bit sums */
#define THICK_MAX 64
#define SUB       16 /* the filler's corners: sixteenths of a pixel */

typedef struct {
    int32_t x, y;
} sub_point_t;

static int clamp(int v, int lo, int hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

static int coord(int v) {
    return clamp(v, -COORD_MAX, COORD_MAX);
}

static uint32_t isqrt(uint64_t v) {
    uint32_t r = 0;
    for (uint32_t bit = 1u << 16; bit != 0; bit >>= 1) {
        uint32_t next = r | bit;
        if ((uint64_t) next * next <= v)
            r = next;
    }
    return r;
}

static void span(const px_canvas_t *c, int x0, int x1, int y, int black) {
    if (y < 0 || y >= c->h)
        return;
    x0 = x0 < 0 ? 0 : x0;
    x1 = x1 >= c->w ? c->w - 1 : x1;
    if (x0 > x1)
        return;
    uint8_t *row = c->bits + y * c->stride;
    int first = x0 / 8, last = x1 / 8;
    uint8_t left = (uint8_t) (0xFF >> (x0 % 8)), right = (uint8_t) (0xFF << (7 - x1 % 8));
    for (int i = first; i <= last; i++) {
        uint8_t mask = (uint8_t) ((i == first ? left : 0xFF) & (i == last ? right : 0xFF));
        row[i] = black ? (uint8_t) (row[i] | mask) : (uint8_t) (row[i] & ~mask);
    }
}

void px_draw_box(const px_canvas_t *c, int x, int y, int w, int h, int t, int black) {
    if (w <= 0 || h <= 0)
        return;
    x = coord(x), y = coord(y);
    w = clamp(w, 0, 2 * COORD_MAX), h = clamp(h, 0, 2 * COORD_MAX);
    t = clamp(t, 0, 2 * COORD_MAX); /* as thick as the widest is filled; none is too */
    int top = clamp(y, 0, c->h), bottom = clamp(y + h, 0, c->h);
    for (int row = top; row < bottom; row++) {
        if (t <= 0 || row < y + t || row >= y + h - t || w <= 2 * t) {
            span(c, x, x + w - 1, row, black);
        } else {
            span(c, x, x + t - 1, row, black);
            span(c, x + w - t, x + w - 1, row, black);
        }
    }
}

/* How far an ellipse reaches each way along row dy. Its edge runs half a
 * pixel beyond the radii, as a circle drawn pixel by pixel does. */
static int half_width(int rx, int ry, int dy) {
    uint64_t a = (uint64_t) (2 * rx + 1) * (uint64_t) (2 * rx + 1);
    uint64_t b = (uint64_t) (2 * ry + 1) * (uint64_t) (2 * ry + 1);
    return (int) isqrt(a * (b - 4 * (uint64_t) ((int64_t) dy * dy)) / (4 * b));
}

void px_draw_ellipse(const px_canvas_t *c, int x, int y, int rx, int ry, int t, int black) {
    if (rx < 0 || ry < 0)
        return;
    x = coord(x), y = coord(y);
    rx = clamp(rx, 0, COORD_MAX), ry = clamp(ry, 0, COORD_MAX);
    t = clamp(t, 0, COORD_MAX + 1); /* more than the largest radius is filled; none is too */
    bool hollow = t > 0 && t <= rx && t <= ry;
    int ix = rx - t, iy = ry - t;
    int top = clamp(y - ry, 0, c->h), bottom = clamp(y + ry + 1, 0, c->h);
    for (int row = top; row < bottom; row++) {
        int dy = row - y, outer = half_width(rx, ry, dy);
        if (hollow && dy >= -iy && dy <= iy) {
            /* In a long thin ellipse the two edges can share a pixel:
             * each side keeps t pixels, and meets the next row out. */
            int inner = half_width(ix, iy, dy), next = half_width(rx, ry, (dy < 0 ? -dy : dy) + 1);
            inner = inner > next ? next : inner;
            inner = inner > outer - t ? outer - t : inner;
            span(c, x - outer, x - inner - 1, row, black);
            span(c, x + inner + 1, x + outer, row, black);
        } else {
            span(c, x - outer, x + outer, row, black);
        }
    }
}

/* Corners in sixteenths of a pixel; even-odd, so a shape crossing itself
 * has holes. n is at most PX_POLY_MAX. */
static void fill_sub(const px_canvas_t *c, const sub_point_t *p, int n, int black) {
    int32_t ymin = p[0].y, ymax = p[0].y;
    for (int i = 1; i < n; i++) {
        ymin = p[i].y < ymin ? p[i].y : ymin;
        ymax = p[i].y > ymax ? p[i].y : ymax;
    }
    int top = clamp(ymin / SUB - 1, 0, c->h), bottom = clamp(ymax / SUB + 2, 0, c->h);
    for (int row = top; row < bottom; row++) {
        int32_t yc = row * SUB + SUB / 2, xs[PX_POLY_MAX];
        int count = 0;
        for (int i = 0; i < n; i++) {
            const sub_point_t *a = &p[i], *b = &p[(i + 1) % n];
            if ((a->y <= yc) == (b->y <= yc))
                continue;
            int32_t xc = a->x + (int32_t) ((int64_t) (yc - a->y) * (b->x - a->x) / (b->y - a->y));
            int at = count++;
            for (; at > 0 && xs[at - 1] > xc; at--)
                xs[at] = xs[at - 1];
            xs[at] = xc;
        }
        for (int i = 0; i + 1 < count; i += 2) /* the pixels whose centres are inside */
            span(c, (xs[i] - SUB / 2 + SUB - 1) >> 4, (xs[i + 1] - SUB / 2) >> 4, row, black);
    }
}

static int outcode(const px_canvas_t *c, int x, int y) {
    return (x < 0 ? 1 : 0) | (x >= c->w ? 2 : 0) | (y < 0 ? 4 : 0) | (y >= c->h ? 8 : 0);
}

/* Cohen-Sutherland: what's left of the line is on the canvas, so a polygon
 * of long edges far off it can't outlast the watchdog. */
static bool clip_line(const px_canvas_t *c, int *x0, int *y0, int *x1, int *y1) {
    int a = outcode(c, *x0, *y0), b = outcode(c, *x1, *y1);
    while (a | b) {
        if (a & b)
            return false;
        int out = a ? a : b, x, y;
        int dx = *x1 - *x0, dy = *y1 - *y0;
        if (out & 8) {
            y = c->h - 1, x = *x0 + dx * (y - *y0) / dy;
        } else if (out & 4) {
            y = 0, x = *x0 + dx * (y - *y0) / dy;
        } else if (out & 2) {
            x = c->w - 1, y = *y0 + dy * (x - *x0) / dx;
        } else {
            x = 0, y = *y0 + dy * (x - *x0) / dx;
        }
        if (out == a) {
            *x0 = x, *y0 = y, a = outcode(c, x, y);
        } else {
            *x1 = x, *y1 = y, b = outcode(c, x, y);
        }
    }
    return true;
}

static void thin_line(const px_canvas_t *c, int x0, int y0, int x1, int y1, int black) { /* Bresenham */
    if (!clip_line(c, &x0, &y0, &x1, &y1))
        return;
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        span(c, x0, x0, y0, black);
        if (x0 == x1 && y0 == y1)
            return;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* A round end or joint: 16 corners, a thousandth of the radius each. */
static void round_dot(const px_canvas_t *c, int x, int y, int t, int black) {
    static const int16_t ring[16][2] = {
            {1000, 0}, {924, 383}, {707, 707}, {383, 924}, {0, 1000}, {-383, 924}, {-707, 707}, {-924, 383},
            {-1000, 0}, {-924, -383}, {-707, -707}, {-383, -924}, {0, -1000}, {383, -924}, {707, -707}, {924, -383},
    };
    sub_point_t p[16];
    for (int i = 0; i < 16; i++) {
        p[i].x = x * SUB + SUB / 2 + ring[i][0] * t * SUB / 2000;
        p[i].y = y * SUB + SUB / 2 + ring[i][1] * t * SUB / 2000;
    }
    fill_sub(c, p, 16, black);
}

void px_draw_line(const px_canvas_t *c, int x0, int y0, int x1, int y1, int t, int black) {
    x0 = coord(x0), y0 = coord(y0), x1 = coord(x1), y1 = coord(y1);
    if (t <= 1) {
        thin_line(c, x0, y0, x1, y1, black);
        return;
    }
    t = t > THICK_MAX ? THICK_MAX : t;
    int dx = x1 - x0, dy = y1 - y0;
    int len = (int) isqrt((uint64_t) ((int64_t) dx * dx + (int64_t) dy * dy));
    if (len > 0) {
        int32_t ox = -dy * t * SUB / (2 * len), oy = dx * t * SUB / (2 * len);
        int32_t ax = x0 * SUB + SUB / 2, ay = y0 * SUB + SUB / 2;
        int32_t bx = x1 * SUB + SUB / 2, by = y1 * SUB + SUB / 2;
        sub_point_t p[4] = {{ax + ox, ay + oy}, {bx + ox, by + oy}, {bx - ox, by - oy}, {ax - ox, ay - oy}};
        fill_sub(c, p, 4, black);
    }
    round_dot(c, x0, y0, t, black);
    round_dot(c, x1, y1, t, black);
}

void px_draw_polygon(const px_canvas_t *c, const px_point_t *p, int n, int t, int black) {
    if (n < 1 || n > PX_POLY_MAX)
        return;
    if (t <= 0) {
        sub_point_t s[PX_POLY_MAX];
        for (int i = 0; i < n; i++) {
            s[i].x = coord(p[i].x) * SUB + SUB / 2;
            s[i].y = coord(p[i].y) * SUB + SUB / 2;
        }
        fill_sub(c, s, n, black);
    }
    for (int i = 0; i < n; i++) { /* a filled one too: its corners are its own */
        const px_point_t *a = &p[i], *b = &p[(i + 1) % n];
        px_draw_line(c, a->x, a->y, b->x, b->y, t, black);
    }
}
