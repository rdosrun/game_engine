#ifndef GROUND_H
#define GROUND_H

#include "../renderer.h"

static void draw_ground(float center_x, float ground_y, float center_z, int tiles, float tile_size) {
    uint32_t color1 = 0x00113311; // Dark green
    uint32_t color2 = 0x00114411; // Slightly lighter green

    float half_size = (tiles * tile_size) / 2.0f;
    float start_x = center_x - half_size;
    float start_z = center_z - half_size;

    for (int z = 0; z < tiles; z++) {
        for (int x = 0; x < tiles; x++) {
            float x0 = start_x + x * tile_size;
            float z0 = start_z + z * tile_size;
            float x1 = x0 + tile_size;
            float z1 = z0 + tile_size;

            Point2D p0 = project_point_3d(x0, ground_y, z0);
            Point2D p1 = project_point_3d(x1, ground_y, z0);
            Point2D p2 = project_point_3d(x1, ground_y, z1);
            Point2D p3 = project_point_3d(x0, ground_y, z1);

            if (p0.visible && p1.visible && p2.visible && p3.visible) {
                uint32_t color = ((x + z) % 2 == 0) ? color1 : color2;
                
                // Draw tile as two triangles
                draw_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, color);
                draw_triangle(p0.x, p0.y, p2.x, p2.y, p3.x, p3.y, color);
            }
        }
    }
}

#endif // GROUND_H
