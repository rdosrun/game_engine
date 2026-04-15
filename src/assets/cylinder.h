#ifndef CYLINDER_H
#define CYLINDER_H

#include <stdint.h>
#include "../renderer.h"
#include "primitive_points.h"

/*
Draws a simple projected cylinder asset in screen space.

`triangle_count` controls how many triangles draw_circle uses for each cap.
This is intentionally a 2D projected helper like the other asset draw helpers:
build/project your cylinder center first, then call this with screen coords.
*/
static void draw_cylinder_projected(
    int center_x,
    int center_y,
    const PrimitiveCylinder *cylinder,
    int triangle_count,
    uint32_t cap_color,
    uint32_t side_color
) {
    if (!cylinder) return;

    int radius = cylinder->radius;
    int half_height = cylinder->height / 2;
    int top_y = center_y - half_height;
    int bottom_y = center_y + half_height;
    int left_x = center_x - radius;
    int right_x = center_x + radius;

    draw_circle(center_x, bottom_y, radius, triangle_count, cap_color);

    draw_triangle(left_x, top_y, right_x, top_y, right_x, bottom_y, side_color);
    draw_triangle(left_x, top_y, right_x, bottom_y, left_x, bottom_y, side_color);

    draw_circle(center_x, top_y, radius, triangle_count, cap_color);
}

#endif // CYLINDER_H
