#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "screen.h"
#include "control.h"

typedef struct {
    int x, y;
    float scale;
    bool visible;
} Point2D;

static void draw_square(int x_pos, int y_pos, int size, uint32_t color) {
    if (size <= 0) return;
    int start_y = (y_pos < 0) ? 0 : y_pos;
    int end_y = (y_pos + size > g_screen.height) ? g_screen.height : y_pos + size;
    int start_x = (x_pos < 0) ? 0 : x_pos;
    int end_x = (x_pos + size > g_screen.width) ? g_screen.width : x_pos + size;
    int width = end_x - start_x;
    if (width <= 0 || start_y >= end_y) return;
    uint32_t* first_row = &g_screen.pixels[start_y * g_screen.width + start_x];
    for (int x = 0; x < width; x++) first_row[x] = color;
    size_t row_size_bytes = width * sizeof(uint32_t);
    for (int y = start_y + 1; y < end_y; y++) {
        uint32_t* target_row = &g_screen.pixels[y * g_screen.width + start_x];
        memcpy(target_row, first_row, row_size_bytes);
    }
}

static void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
    if (y0 > y1) { int tx = x0; x0 = x1; x1 = tx; int ty = y0; y0 = y1; y1 = ty; }
    if (y0 > y2) { int tx = x0; x0 = x2; x2 = tx; int ty = y0; y0 = y2; y2 = ty; }
    if (y1 > y2) { int tx = x1; x1 = x2; x2 = tx; int ty = y1; y1 = y2; y2 = ty; }
    if (y0 == y2) return;
    int total_height = y2 - y0;
    for (int i = 0; i < total_height; i++) {
        bool second_half = i > y1 - y0 || y1 == y0;
        int segment_height = second_half ? y2 - y1 : y1 - y0;
        float alpha = (float)i / total_height;
        float beta  = (float)(i - (second_half ? y1 - y0 : 0)) / segment_height;
        int ax = x0 + (int)((x2 - x0) * alpha);
        int bx = second_half ? x1 + (int)((x2 - x1) * beta) : x0 + (int)((x1 - x0) * beta);
        if (ax > bx) { int tx = ax; ax = bx; bx = tx; }
        int draw_y = y0 + i;
        if (draw_y < 0 || draw_y >= g_screen.height) continue;
        int start_x = (ax < 0) ? 0 : ax;
        int end_x = (bx >= g_screen.width) ? g_screen.width - 1 : bx;
        for (int j = start_x; j <= end_x; j++) g_screen.pixels[draw_y * g_screen.width + j] = color;
    }
}

static Point2D project_point_3d(float x, float y, float z) {
    float focal_length = 400.0f;
    int center_x = g_screen.width / 2;
    int center_y = g_screen.height / 2;

    float cos_y = (float)cos(-g_control.yaw);
    float sin_y = (float)sin(-g_control.yaw);
    float cos_p = (float)cos(-g_control.pitch);
    float sin_p = (float)sin(-g_control.pitch);

    float tx = x - g_control.pos_x;
    float ty = y - g_control.pos_y;
    float tz = z - g_control.pos_z;

    float rx = tx * cos_y - tz * sin_y;
    float rz = tx * sin_y + tz * cos_y;
    float ry = ty * cos_p - rz * sin_p;
    float final_z = ty * sin_p + rz * cos_p;

    Point2D p = {0};
    if (final_z > 0.0f) {
        // Prevent division by zero if exactly 0
        float safe_z = (final_z == 0.0f) ? 0.001f : final_z;
        p.scale = focal_length / safe_z;
        p.x = center_x + (int)(rx * p.scale);
        p.y = center_y + (int)(ry * p.scale);
        p.visible = true;
    }
    return p;
}

#include "assets/assets.h"

static void clear_screen(uint32_t color) {
    for (int i = 0; i < g_screen.width * g_screen.height; i++) g_screen.pixels[i] = color;
}

static void render_pixels(void) {
    clear_screen(0x00000000);

    // Draw the ground plane (A large 20x20 tile grid)
    draw_ground(0.0f, 200.0f, 0.0f, 20, 200.0f);

    for (int z = -1000; z <= 1000; z += 500) {
        for (int x = -1000; x <= 1000; x += 500) {
            draw_house((float)x, 200.0f, (float)z, 100.0f, 0x00888888, 0x00FF0000);
        }
    }
}

#endif // RENDERER_H
