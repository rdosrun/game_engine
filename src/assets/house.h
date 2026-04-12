#ifndef HOUSE_H
#define HOUSE_H

#include "../renderer.h"

static void draw_house(float x, float y, float z, float size, uint32_t base_color, uint32_t roof_color) {
    float h = size;
    float w = size / 2.0f;

    // Define vertices for a 3D box (house base)
    // Vertices: 0-3 bottom, 4-7 top
    Point2D v[8];
    v[0] = project_point_3d(x - w, y,     z - w);
    v[1] = project_point_3d(x + w, y,     z - w);
    v[2] = project_point_3d(x + w, y,     z + w);
    v[3] = project_point_3d(x - w, y,     z + w);
    v[4] = project_point_3d(x - w, y - h, z - w);
    v[5] = project_point_3d(x + w, y - h, z - w);
    v[6] = project_point_3d(x + w, y - h, z + w);
    v[7] = project_point_3d(x - w, y - h, z + w);

    // Only draw if enough vertices are visible
    bool all_v = true;
    for(int i=0; i<8; i++) if(!v[i].visible) all_v = false;
    if(!all_v) return;

    // Draw faces (Front, Back, Left, Right)
    // Front: 0, 1, 5, 4
    draw_triangle(v[0].x, v[0].y, v[1].x, v[1].y, v[5].x, v[5].y, base_color);
    draw_triangle(v[0].x, v[0].y, v[5].x, v[5].y, v[4].x, v[4].y, base_color);
    // Back: 3, 2, 6, 7
    draw_triangle(v[3].x, v[3].y, v[2].x, v[2].y, v[6].x, v[6].y, base_color);
    draw_triangle(v[3].x, v[3].y, v[6].x, v[6].y, v[7].x, v[7].y, base_color);
    // Left: 0, 3, 7, 4
    draw_triangle(v[0].x, v[0].y, v[3].x, v[3].y, v[7].x, v[7].y, base_color);
    draw_triangle(v[0].x, v[0].y, v[7].x, v[7].y, v[4].x, v[4].y, base_color);
    // Right: 1, 2, 6, 5
    draw_triangle(v[1].x, v[1].y, v[2].x, v[2].y, v[6].x, v[6].y, base_color);
    draw_triangle(v[1].x, v[1].y, v[6].x, v[6].y, v[5].x, v[5].y, base_color);

    // Roof Top vertex
    Point2D tip = project_point_3d(x, y - h - h/2, z);
    if(tip.visible) {
        // Front roof
        draw_triangle(v[4].x, v[4].y, v[5].x, v[5].y, tip.x, tip.y, roof_color);
        // Back roof
        draw_triangle(v[7].x, v[7].y, v[6].x, v[6].y, tip.x, tip.y, roof_color);
        // Left roof
        draw_triangle(v[4].x, v[4].y, v[7].x, v[7].y, tip.x, tip.y, roof_color);
        // Right roof
        draw_triangle(v[5].x, v[5].y, v[6].x, v[6].y, tip.x, tip.y, roof_color);
    }
}

#endif // HOUSE_H
