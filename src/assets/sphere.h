#ifndef SPHERE_H
#if !defined(RENDERER_H)
#include "../renderer.h"
#else
#define SPHERE_H

#include <stdint.h>
#include "primitive_points.h"

static DirtyRect draw_sphere_mesh(
    const PrimitiveSphere *sphere,
    const RendererFrameContext *frame,
    int latitude_count,
    int longitude_count,
    int offset_x,
    int offset_y,
    int offset_z,
    int theta_x,
    int theta_y,
    int theta_z,
    uint32_t color_a,
    uint32_t color_b
) {
    DirtyRect sphere_dirty = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);

    if (!sphere || !frame || sphere->radius <= 0) {
        return sphere_dirty;
    }

    if (latitude_count < 3) latitude_count = 3;
    if (latitude_count > 128) latitude_count = 128;
    if (longitude_count < 3) longitude_count = 3;
    if (longitude_count > 256) longitude_count = 256;

    int center_x = sphere->center[0];
    int center_y = sphere->center[1];
    int center_z = sphere->center[2];
    int radius = sphere->radius;
    int top_cap_radius = (radius * fixed_sin_u8(128 / latitude_count)) >> 10;
    int bottom_cap_radius = top_cap_radius;
    int top_cap_center[3] = {
        center_x,
        center_y,
        center_z + ((radius * fixed_cos_u8(128 / latitude_count)) >> 10)
    };
    int bottom_cap_center[3] = {
        center_x,
        center_y,
        center_z + ((radius * fixed_cos_u8(128 - (128 / latitude_count))) >> 10)
    };

    sphere_dirty = union_dirty_rects(
        sphere_dirty,
        draw_mesh_circle(
            top_cap_center,
            top_cap_radius,
            frame,
            longitude_count,
            offset_x,
            offset_y,
            offset_z,
            theta_x,
            theta_y,
            theta_z,
            false,
            color_a
        )
    );

    sphere_dirty = union_dirty_rects(
        sphere_dirty,
        draw_mesh_circle(
            bottom_cap_center,
            bottom_cap_radius,
            frame,
            longitude_count,
            offset_x,
            offset_y,
            offset_z,
            theta_x,
            theta_y,
            theta_z,
            true,
            color_b
        )
    );

    for (int lat = 1; lat < latitude_count - 1; ++lat) {
        int polar0 = (lat * 128) / latitude_count;
        int polar1 = ((lat + 1) * 128) / latitude_count;
        int ring_radius0 = (radius * fixed_sin_u8(polar0)) >> 10;
        int ring_radius1 = (radius * fixed_sin_u8(polar1)) >> 10;
        int z0 = center_z + ((radius * fixed_cos_u8(polar0)) >> 10);
        int z1 = center_z + ((radius * fixed_cos_u8(polar1)) >> 10);

        for (int lon = 0; lon < longitude_count; ++lon) {
            int angle0 = (lon * 256) / longitude_count;
            int angle1 = ((lon + 1) * 256) / longitude_count;
            int x00 = center_x + ((ring_radius0 * fixed_cos_u8(angle0)) >> 10);
            int y00 = center_y + ((ring_radius0 * fixed_sin_u8(angle0)) >> 10);
            int x01 = center_x + ((ring_radius0 * fixed_cos_u8(angle1)) >> 10);
            int y01 = center_y + ((ring_radius0 * fixed_sin_u8(angle1)) >> 10);
            int x10 = center_x + ((ring_radius1 * fixed_cos_u8(angle0)) >> 10);
            int y10 = center_y + ((ring_radius1 * fixed_sin_u8(angle0)) >> 10);
            int x11 = center_x + ((ring_radius1 * fixed_cos_u8(angle1)) >> 10);
            int y11 = center_y + ((ring_radius1 * fixed_sin_u8(angle1)) >> 10);
            uint32_t color = ((lat + lon) & 1) ? color_a : color_b;

            int surface_a[3][3] = {
                {x00, y00, z0},
                {x10, y10, z1},
                {x11, y11, z1}
            };
            int surface_b[3][3] = {
                {x00, y00, z0},
                {x11, y11, z1},
                {x01, y01, z0}
            };

            rotate_vertices(surface_a, 3, theta_x, theta_y, theta_z);
            translate_vertices(surface_a, 3, offset_x, offset_y, offset_z);
            sphere_dirty = union_dirty_rects(
                sphere_dirty,
                draw_mesh_triangle(surface_a, frame, color)
            );

            rotate_vertices(surface_b, 3, theta_x, theta_y, theta_z);
            translate_vertices(surface_b, 3, offset_x, offset_y, offset_z);
            sphere_dirty = union_dirty_rects(
                sphere_dirty,
                draw_mesh_triangle(surface_b, frame, color)
            );
        }
    }

    return sphere_dirty;
}

#endif // SPHERE_H
#endif
