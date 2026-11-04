#ifndef PX_DRAW_H
#define PX_DRAW_H

#include <stdint.h>

#include "px_app.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Shapes drawn into any 1-bit picture: the screen for apps (app.c), the
 * simulator's (a copy of this file, tools/sync_framework.sh), Plus42's
 * display later. No state and no allocation. Off-canvas parts are left out.
 *
 * t is the thickness: 0 filled, 1 one pixel, n pixels. Boxes, circles and
 * ellipses grow inward, keeping their outer size; lines and polygon edges
 * are centred on the line, with round ends. */
typedef struct {
    uint8_t *bits; /* 1 = black, rows of stride bytes, top bit leftmost */
    int w, h, stride;
} px_canvas_t;

void px_draw_box(const px_canvas_t *c, int x, int y, int w, int h, int t, int black);
void px_draw_ellipse(const px_canvas_t *c, int x, int y, int rx, int ry, int t, int black);
void px_draw_line(const px_canvas_t *c, int x0, int y0, int x1, int y1, int t, int black);
void px_draw_polygon(const px_canvas_t *c, const px_point_t *p, int n, int t, int black);

#ifdef __cplusplus
}
#endif

#endif
