#ifndef PYRAMID_H
#if !defined(RENDERER_H)
#include "../renderer.h"
#else
#define PYRAMID_H

#include <stdint.h>
#include "primitive_points.h"

static DirtyRect draw_pyramid_mesh(
    const PrimitivePyramid *pyramid,
    const RendererFrameContext *frame,
    int offset_x,
    int offset_y,
    int offset_z,
    int theta_x,
    int theta_y,
    int theta_z,
    const uint32_t face_colors[5]
) {
    static const int face_indices[6][3] = {
        {0, 2, 1},
        {0, 3, 2},
        {0, 1, 4},
        {1, 2, 4},
        {2, 3, 4},
        {3, 0, 4}
    };
    DirtyRect pyramid_dirty = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);

    if (!pyramid || !frame || pyramid->base_width <= 0 || pyramid->base_depth <= 0 || pyramid->height <= 0) {
        return pyramid_dirty;
    }

    int half_width = pyramid->base_width / 2;
    int half_depth = pyramid->base_depth / 2;
    int vertices[5][3] = {
        {
            pyramid->base_center[0] - half_width,
            pyramid->base_center[1] - half_depth,
            pyramid->base_center[2]
        },
        {
            pyramid->base_center[0] + half_width,
            pyramid->base_center[1] - half_depth,
            pyramid->base_center[2]
        },
        {
            pyramid->base_center[0] + half_width,
            pyramid->base_center[1] + half_depth,
            pyramid->base_center[2]
        },
        {
            pyramid->base_center[0] - half_width,
            pyramid->base_center[1] + half_depth,
            pyramid->base_center[2]
        },
        {
            pyramid->base_center[0],
            pyramid->base_center[1],
            pyramid->base_center[2] + pyramid->height
        }
    };

    rotate_vertices(vertices, 5, theta_x, theta_y, theta_z);
    translate_vertices(vertices, 5, offset_x, offset_y, offset_z);

    for (int i = 0; i < 6; ++i) {
        int triangle_vertices[3][3] = {
            {
                vertices[face_indices[i][0]][0],
                vertices[face_indices[i][0]][1],
                vertices[face_indices[i][0]][2]
            },
            {
                vertices[face_indices[i][1]][0],
                vertices[face_indices[i][1]][1],
                vertices[face_indices[i][1]][2]
            },
            {
                vertices[face_indices[i][2]][0],
                vertices[face_indices[i][2]][1],
                vertices[face_indices[i][2]][2]
            }
        };
        uint32_t color = face_colors ? face_colors[i < 2 ? 0 : i - 1] : g_cube_face_colors[i % 12];

        pyramid_dirty = union_dirty_rects(
            pyramid_dirty,
            draw_mesh_triangle(triangle_vertices, frame, color)
        );
    }

    return pyramid_dirty;
}

#endif // PYRAMID_H
#endif
