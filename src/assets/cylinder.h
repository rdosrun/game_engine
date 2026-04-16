#ifndef CYLINDER_H
#if !defined(RENDERER_H)
#include "../renderer.h"
#else
#define CYLINDER_H

#include <stdint.h>
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

static DirtyRect draw_cylinder_mesh(
    const PrimitiveCylinder *cylinder,
    const RendererFrameContext *frame,
    int segment_count,
    int offset_x,
    int offset_y,
    int offset_z,
    int theta_x,
    int theta_y,
    int theta_z,
    uint32_t cap_color,
    uint32_t side_color
) {
    DirtyRect cylinder_dirty = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);

    if (!cylinder || !frame || cylinder->radius <= 0 || cylinder->height <= 0) {
        return cylinder_dirty;
    }

    if (segment_count < 3) segment_count = 3;
    if (segment_count > 256) segment_count = 256;

    int center_x = cylinder->center[0];
    int center_y = cylinder->center[1];
    int center_z = cylinder->center[2];
    int radius = cylinder->radius;
    int half_height = cylinder->height / 2;
    int back_z = center_z - half_height;
    int front_z = center_z + half_height;

    for (int i = 0; i < segment_count; ++i) {
        int angle0 = (i * 256) / segment_count;
        int angle1 = ((i + 1) * 256) / segment_count;
        int x0 = center_x + ((radius * fixed_cos_u8(angle0)) >> 10);
        int y0 = center_y + ((radius * fixed_sin_u8(angle0)) >> 10);
        int x1 = center_x + ((radius * fixed_cos_u8(angle1)) >> 10);
        int y1 = center_y + ((radius * fixed_sin_u8(angle1)) >> 10);
        int cap_back[3][3] = {
            {center_x, center_y, back_z},
            {x1, y1, back_z},
            {x0, y0, back_z}
        };
        int cap_front[3][3] = {
            {center_x, center_y, front_z},
            {x0, y0, front_z},
            {x1, y1, front_z}
        };
        int side_a[3][3] = {
            {x0, y0, back_z},
            {x1, y1, back_z},
            {x1, y1, front_z}
        };
        int side_b[3][3] = {
            {x0, y0, back_z},
            {x1, y1, front_z},
            {x0, y0, front_z}
        };

        rotate_vertices(cap_back, 3, theta_x, theta_y, theta_z);
        translate_vertices(cap_back, 3, offset_x, offset_y, offset_z);
        cylinder_dirty = union_dirty_rects(
            cylinder_dirty,
            draw_mesh_triangle(cap_back, frame, cap_color)
        );

        rotate_vertices(cap_front, 3, theta_x, theta_y, theta_z);
        translate_vertices(cap_front, 3, offset_x, offset_y, offset_z);
        cylinder_dirty = union_dirty_rects(
            cylinder_dirty,
            draw_mesh_triangle(cap_front, frame, cap_color)
        );

        rotate_vertices(side_a, 3, theta_x, theta_y, theta_z);
        translate_vertices(side_a, 3, offset_x, offset_y, offset_z);
        cylinder_dirty = union_dirty_rects(
            cylinder_dirty,
            draw_mesh_triangle(side_a, frame, side_color)
        );

        rotate_vertices(side_b, 3, theta_x, theta_y, theta_z);
        translate_vertices(side_b, 3, offset_x, offset_y, offset_z);
        cylinder_dirty = union_dirty_rects(
            cylinder_dirty,
            draw_mesh_triangle(side_b, frame, side_color)
        );
    }

    return cylinder_dirty;
}

#endif // CYLINDER_H
#endif
