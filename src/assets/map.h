#ifndef MAP_H
#if !defined(RENDERER_H)
#include "../renderer.h"
#else
#define MAP_H

#include <stdint.h>
#include "primitive_points.h"

#define MAP_ROWS 3
#define MAP_COLS 5
#define MAP_CELL_SPACING_X 260
#define MAP_CELL_SPACING_Z 260
#define MAP_ORIGIN_X -520
#define MAP_ORIGIN_Y 0
#define MAP_ORIGIN_Z -260

#define MAP_SHAPE_EMPTY 0
#define MAP_SHAPE_CYLINDER 1
#define MAP_SHAPE_SPHERE 2
#define MAP_SHAPE_PRISM 3
#define MAP_SHAPE_PYRAMID 4

static const int g_map_shape_indices[MAP_ROWS][MAP_COLS] = {
    {MAP_SHAPE_CYLINDER, MAP_SHAPE_SPHERE,   MAP_SHAPE_PRISM,    MAP_SHAPE_PYRAMID, MAP_SHAPE_EMPTY},
    {MAP_SHAPE_EMPTY,    MAP_SHAPE_PRISM,    MAP_SHAPE_EMPTY,    MAP_SHAPE_SPHERE,  MAP_SHAPE_CYLINDER},
    {MAP_SHAPE_PYRAMID,  MAP_SHAPE_EMPTY,    MAP_SHAPE_CYLINDER, MAP_SHAPE_EMPTY,   MAP_SHAPE_PRISM}
};

static const int g_map_color_indices[MAP_ROWS][MAP_COLS] = {
    {0, 1, 2, 3, 0},
    {0, 4, 0, 5, 1},
    {3, 0, 2, 0, 4}
};

static const uint32_t g_map_palette[][6] = {
    {0x00DDDD66, 0x0066AADD, 0x00DDDD66, 0x0066AADD, 0x00DDDD66, 0x0066AADD},
    {0x00DD7777, 0x0077DD99, 0x00DD7777, 0x0077DD99, 0x00DD7777, 0x0077DD99},
    {0x00CC7755, 0x00AA5533, 0x0055CC77, 0x00339955, 0x005577CC, 0x00335599},
    {0x007777DD, 0x00DD9955, 0x00CC7744, 0x00AA5533, 0x00EEBB66, 0x00EEBB66},
    {0x002F7D32, 0x006B4A2D, 0x002F7D32, 0x006B4A2D, 0x002F7D32, 0x006B4A2D},
    {0x00DD55AA, 0x0055AADD, 0x00AA55DD, 0x00DD8855, 0x0055DD88, 0x008855DD}
};

static const int g_map_palette_count = (int)(sizeof(g_map_palette) / sizeof(g_map_palette[0]));

static int map_color_index_clamped(int color_index) {
    if (color_index < 0) return 0;
    if (color_index >= g_map_palette_count) return g_map_palette_count - 1;
    return color_index;
}

static DirtyRect draw_map_mesh(
    const RendererFrameContext *frame,
    int theta_x,
    int theta_y,
    int theta_z
) {
    DirtyRect map_dirty = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);

    if (!frame) {
        return map_dirty;
    }

    for (int row = 0; row < MAP_ROWS; ++row) {
        for (int col = 0; col < MAP_COLS; ++col) {
            int shape_index = g_map_shape_indices[row][col];
            int color_index = map_color_index_clamped(g_map_color_indices[row][col]);
            const uint32_t *colors = g_map_palette[color_index];
            int offset_x = MAP_ORIGIN_X + (col * MAP_CELL_SPACING_X);
            int offset_y = MAP_ORIGIN_Y;
            int offset_z = MAP_ORIGIN_Z + (row * MAP_CELL_SPACING_Z);
            DirtyRect cell_dirty = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);

            if (shape_index == MAP_SHAPE_CYLINDER) {
                static const PrimitiveCylinder map_cylinder = {{0, 0, 0}, 72, 150, 24};
                cell_dirty = draw_cylinder_mesh(
                    &map_cylinder,
                    frame,
                    24,
                    offset_x,
                    offset_y,
                    offset_z,
                    theta_x,
                    theta_y,
                    theta_z,
                    colors[0],
                    colors[1]
                );
            } else if (shape_index == MAP_SHAPE_SPHERE) {
                static const PrimitiveSphere map_sphere = {{0, 0, 0}, 78, 16};
                cell_dirty = draw_sphere_mesh(
                    &map_sphere,
                    frame,
                    12,
                    24,
                    offset_x,
                    offset_y,
                    offset_z,
                    theta_x,
                    theta_y,
                    theta_z,
                    colors[0],
                    colors[1]
                );
            } else if (shape_index == MAP_SHAPE_PRISM) {
                static const PrimitiveBox map_prism = {{0, 0, 0}, 150, 110, 130, 32};
                cell_dirty = draw_rectangular_prism_mesh(
                    &map_prism,
                    frame,
                    offset_x,
                    offset_y,
                    offset_z,
                    theta_x,
                    theta_y,
                    theta_z,
                    colors
                );
            } else if (shape_index == MAP_SHAPE_PYRAMID) {
                static const PrimitivePyramid map_pyramid = {{0, 0, -65}, 150, 130, 140, 32};
                cell_dirty = draw_pyramid_mesh(
                    &map_pyramid,
                    frame,
                    offset_x,
                    offset_y,
                    offset_z,
                    theta_x,
                    theta_y,
                    theta_z,
                    colors
                );
            }

            map_dirty = union_dirty_rects(map_dirty, cell_dirty);
        }
    }

    return map_dirty;
}

#endif // MAP_H
#endif
