#ifndef GROUND_H
#define GROUND_H

#include "../renderer.h"

static void draw_ground_tile(Point2D p0, Point2D p1, Point2D p2, Point2D p3, uint32_t color) {
    draw_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, color);
    draw_triangle(p0.x, p0.y, p2.x, p2.y, p3.x, p3.y, color);
}

#endif // GROUND_H
