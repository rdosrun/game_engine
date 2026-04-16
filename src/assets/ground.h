#ifndef GROUND_H
#if !defined(RENDERER_H)
#include "../renderer.h"
#else
#define GROUND_H

#include <stdint.h>

static DirtyRect draw_ground_tile_mesh(
    const int corners[4][3],
    const RendererFrameContext *frame,
    int offset_x,
    int offset_y,
    int offset_z,
    int theta_x,
    int theta_y,
    int theta_z,
    uint32_t color_a,
    uint32_t color_b
) {
    DirtyRect ground_dirty = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);

    if (!corners || !frame) {
        return ground_dirty;
    }

    int vertices[4][3] = {
        {corners[0][0], corners[0][1], corners[0][2]},
        {corners[1][0], corners[1][1], corners[1][2]},
        {corners[2][0], corners[2][1], corners[2][2]},
        {corners[3][0], corners[3][1], corners[3][2]}
    };
    static const int triangle_indices[2][3] = {
        {0, 1, 2},
        {0, 2, 3}
    };

    rotate_vertices(vertices, 4, theta_x, theta_y, theta_z);
    translate_vertices(vertices, 4, offset_x, offset_y, offset_z);

    for (int i = 0; i < 2; ++i) {
        int triangle_vertices[3][3] = {
            {
                vertices[triangle_indices[i][0]][0],
                vertices[triangle_indices[i][0]][1],
                vertices[triangle_indices[i][0]][2]
            },
            {
                vertices[triangle_indices[i][1]][0],
                vertices[triangle_indices[i][1]][1],
                vertices[triangle_indices[i][1]][2]
            },
            {
                vertices[triangle_indices[i][2]][0],
                vertices[triangle_indices[i][2]][1],
                vertices[triangle_indices[i][2]][2]
            }
        };

        ground_dirty = union_dirty_rects(
            ground_dirty,
            draw_mesh_triangle(triangle_vertices, frame, i == 0 ? color_a : color_b)
        );
    }

    return ground_dirty;
}

#endif // GROUND_H
#endif
