#ifndef RECTANGULAR_PRISM_H
#if !defined(RENDERER_H)
#include "../renderer.h"
#else
#define RECTANGULAR_PRISM_H

#include <stdint.h>
#include "primitive_points.h"

static DirtyRect draw_rectangular_prism_mesh(
    const PrimitiveBox *prism,
    const RendererFrameContext *frame,
    int offset_x,
    int offset_y,
    int offset_z,
    int theta_x,
    int theta_y,
    int theta_z,
    const uint32_t face_colors[6]
) {
    static const int face_indices[12][3] = {
        {0, 2, 1}, {0, 3, 2},
        {4, 5, 6}, {4, 6, 7},
        {0, 1, 5}, {0, 5, 4},
        {3, 7, 6}, {3, 6, 2},
        {0, 4, 7}, {0, 7, 3},
        {1, 2, 6}, {1, 6, 5}
    };
    DirtyRect prism_dirty = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);

    if (!prism || !frame || prism->width <= 0 || prism->height <= 0 || prism->depth <= 0) {
        return prism_dirty;
    }

    int half_width = prism->width / 2;
    int half_height = prism->height / 2;
    int half_depth = prism->depth / 2;
    int vertices[8][3] = {
        {prism->center[0] - half_width, prism->center[1] - half_height, prism->center[2] - half_depth},
        {prism->center[0] + half_width, prism->center[1] - half_height, prism->center[2] - half_depth},
        {prism->center[0] + half_width, prism->center[1] + half_height, prism->center[2] - half_depth},
        {prism->center[0] - half_width, prism->center[1] + half_height, prism->center[2] - half_depth},
        {prism->center[0] - half_width, prism->center[1] - half_height, prism->center[2] + half_depth},
        {prism->center[0] + half_width, prism->center[1] - half_height, prism->center[2] + half_depth},
        {prism->center[0] + half_width, prism->center[1] + half_height, prism->center[2] + half_depth},
        {prism->center[0] - half_width, prism->center[1] + half_height, prism->center[2] + half_depth}
    };

    rotate_vertices(vertices, 8, theta_x, theta_y, theta_z);
    translate_vertices(vertices, 8, offset_x, offset_y, offset_z);

    for (int i = 0; i < 12; ++i) {
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
        uint32_t color = face_colors ? face_colors[i / 2] : g_cube_face_colors[i % 12];

        prism_dirty = union_dirty_rects(
            prism_dirty,
            draw_mesh_triangle(triangle_vertices, frame, color)
        );
    }

    return prism_dirty;
}

#endif // RECTANGULAR_PRISM_H
#endif
