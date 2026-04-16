#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include "screen.h"
#include <stdbool.h>
#include "assets/primitive_points.h"
#include "surface_mesher.h"

extern int g_fps;

static int g_depth_buffer[WINDOW_WIDTH * WINDOW_HEIGHT];
static const int g_cube_faces[12][3] = {
    {0, 1, 2}, {0, 2, 3},
    {4, 6, 5}, {4, 7, 6},
    {0, 3, 7}, {0, 7, 4},
    {1, 5, 6}, {1, 6, 2},
    {3, 2, 6}, {3, 6, 7},
    {0, 4, 5}, {0, 5, 1}
};
static const uint32_t g_cube_face_colors[12] = {
    0x00CC4444, 0x00AA3333,
    0x0044CC44, 0x0033AA33,
    0x004444CC, 0x003333AA,
    0x00CCCC44, 0x00AAAA33,
    0x0044CCCC, 0x0033AAAA,
    0x00CC44CC, 0x00AA33AA
};
static SurfaceMesherResult g_demo_meshes[6];
static bool g_demo_meshes_initialized = false;
static const PrimitiveShapeAsset *g_demo_mesh_assets[6] = {
    &g_circle_shape,
    &g_pyramid_shape,
    &g_cylinder_shape,
    &g_box_shape,
    &g_sphere_shape,
    &g_stair_shape
};
static const int g_demo_mesh_resolutions[6] = {50, 50, 50, 50, 50, 50};
static const int g_demo_mesh_offsets[6][3] = {
    {-1100, 220, 0},
    {-420, 220, 0},
    {260, 220, 0},
    {980, 220, 0},
    {-700, -220, 0},
    {500, -220, 0}
};

typedef struct RendererFrameContext {
    int cam_sin_yaw;
    int cam_cos_yaw;
    int cam_sin_pitch;
    int cam_cos_pitch;
    int focal_length;
    int near_plane;
    int world_offset_z;
    int screen_center_x;
    int screen_center_y;
    int control_x;
    int control_y;
    int control_z;
} RendererFrameContext;

typedef struct DirtyRect {
    int x0;
    int y0;
    int x1;
    int y1;
} DirtyRect;

typedef struct RendererSceneState {
    int control_x;
    int control_y;
    int control_z;
    int cam_sin_yaw;
    int cam_cos_yaw;
    int cam_sin_pitch;
    int cam_cos_pitch;
    int theta_x;
    int theta_y;
    int theta_z;
} RendererSceneState;

static DirtyRect make_dirty_rect(int x0, int y0, int x1, int y1);
static void draw_cube(
    const int cube_vertices[8][3],
    const RendererFrameContext *frame,
    DirtyRect *out_dirty_rect
);

static inline int edge(int x0, int y0, int x1, int y1, int px, int py) {
    return (px - x0) * (y1 - y0) - (py - y0) * (x1 - x0);
}

typedef struct Edge {
    int x;
    int dx;
    int dy;
    int step;
    int err;
    int y0;
} Edge;

static inline void init_edge(Edge *e, int ax, int ay, int bx, int by) {
    e->x = ax;
    e->dx = bx - ax;
    e->dy = by - ay;
    e->step = (e->dx >= 0) ? 1 : -1;
    if (e->dx < 0) e->dx = -e->dx;
    e->err = 0;
    e->y0 = ay;
}

static inline void step_edge(Edge *e) {
    if (e->dy == 0) return;
    e->err += e->dx;
    while (e->err >= e->dy) {
        e->x += e->step;
        e->err -= e->dy;
    }
}

static inline void advance_edge_to(Edge *e, int y) {
    if (e->dy == 0) return;
    int count = y - e->y0;
    for (int i = 0; i < count; ++i) step_edge(e);
}

bool is_inside(
    int px, int py,
    int x1, int y1,
    int x2, int y2,
    int x3, int y3
) {
    int e1 = edge(x1, y1, x2, y2, px, py);
    int e2 = edge(x2, y2, x3, y3, px, py);
    int e3 = edge(x3, y3, x1, y1, px, py);

    // Check if all have same sign (inside or on edge)
    return (e1 >= 0 && e2 >= 0 && e3 >= 0) ||
           (e1 <= 0 && e2 <= 0 && e3 <= 0);
}


static void draw_triangle(int x0,int y0,int x1,int y1,int x2,int y2, uint32_t color) {
    // Sort vertices by y (ascending)
    if (y1 < y0) { int tx = x0; int ty = y0; x0 = x1; y0 = y1; x1 = tx; y1 = ty; }
    if (y2 < y0) { int tx = x0; int ty = y0; x0 = x2; y0 = y2; x2 = tx; y2 = ty; }
    if (y2 < y1) { int tx = x1; int ty = y1; x1 = x2; y1 = y2; x2 = tx; y2 = ty; }

    if (y0 == y2) return;

    int y_start = y0;
    int y_end = y2;
    if (y_end < 0 || y_start >= g_screen.height) return;
    if (y_start < 0) y_start = 0;
    if (y_end >= g_screen.height) y_end = g_screen.height - 1;

    Edge e02, e01, e12;
    init_edge(&e02, x0, y0, x2, y2);
    init_edge(&e01, x0, y0, x1, y1);
    init_edge(&e12, x1, y1, x2, y2);

    advance_edge_to(&e02, y_start);
    if (y_start < y1) {
        advance_edge_to(&e01, y_start);
    } else {
        advance_edge_to(&e12, y_start);
    }

    for (int y = y_start; y <= y_end; ++y) {
        int xa = e02.x;
        int xb = (y < y1) ? e01.x : e12.x;

        int x_start = (xa < xb) ? xa : xb;
        int x_end = (xa > xb) ? xa : xb;

        if (!(x_end < 0 || x_start >= g_screen.width)) {
            if (x_start < 0) x_start = 0;
            if (x_end >= g_screen.width) x_end = g_screen.width - 1;
            int row = y * g_screen.width;
            for (int x = x_start; x <= x_end; ++x) {
                g_screen.pixels[row + x] = color;
            }
        }

        step_edge(&e02);
        if (y + 1 <= y_end) {
            if (y < y1) {
                step_edge(&e01);
            } else {
                step_edge(&e12);
            }
        }
    }
}

static void draw_line(int x0,int y0,int x1,int y1,uint32_t color){
    if(x0==x1){
        for(int i = y0;i<y1;++i){
            g_screen.pixels[i*g_screen.width] = color;
        }
    }else{
        float slope = (float)(y0-y1)/(x0-x1);
        for (int i = x0;i<x1;++i){
            g_screen.pixels[i+((y0+(int)((i-x0)*slope))*g_screen.width)] = color;
        }
    }
}

static void draw_square(int x_pos, int y_pos, int size, uint32_t color) {
    if (size <= 0) return;
    int x0 = x_pos;
    int y0 = y_pos;
    int x1 = x_pos + size;
    int y1 = y_pos + size;
    draw_triangle(x0, y0, x1, y0, x1, y1, color);
    draw_triangle(x0, y0, x1, y1, x0, y1, color);
}

static void draw_block(int x, int y, int size, uint32_t color) {
    for (int py = 0; py < size; ++py) {
        int sy = y + py;
        if (sy < 0 || sy >= g_screen.height) continue;
        int row = sy * g_screen.width;
        for (int px = 0; px < size; ++px) {
            int sx = x + px;
            if (sx < 0 || sx >= g_screen.width) continue;
            g_screen.pixels[row + sx] = color;
        }
    }
}

static void draw_glyph_3x5(int x, int y, const uint8_t glyph[5], int scale, uint32_t color) {
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (glyph[row] & (1 << (2 - col))) {
                draw_block(x + col * scale, y + row * scale, scale, color);
            }
        }
    }
}

static void draw_fps_counter(void) {
    static const uint8_t glyph_f[5] = {7, 4, 6, 4, 4};
    static const uint8_t glyph_p[5] = {6, 5, 6, 4, 4};
    static const uint8_t glyph_s[5] = {3, 4, 2, 1, 6};
    static const uint8_t digit_glyphs[10][5] = {
        {7, 5, 5, 5, 7},
        {2, 6, 2, 2, 7},
        {7, 1, 7, 4, 7},
        {7, 1, 7, 1, 7},
        {5, 5, 7, 1, 1},
        {7, 4, 7, 1, 7},
        {7, 4, 7, 5, 7},
        {7, 1, 1, 1, 1},
        {7, 5, 7, 5, 7},
        {7, 5, 7, 1, 7}
    };

    int fps = g_fps;
    if (fps < 0) fps = 0;
    if (fps > 9999) fps = 9999;

    int digits[4];
    int digit_count = 0;
    do {
        digits[digit_count++] = fps % 10;
        fps /= 10;
    } while (fps > 0 && digit_count < 4);

    const int scale = 3;
    const int glyph_w = 3 * scale;
    const int glyph_h = 5 * scale;
    const int spacing = scale;
    const int total_width = (3 * glyph_w) + (digit_count * glyph_w) + ((2 + digit_count) * spacing);
    int x = g_screen.width - total_width - 8;
    int y = 8;
    const uint32_t color = 0x00FFFFFF;

    draw_glyph_3x5(x, y, glyph_f, scale, color);
    x += glyph_w + spacing;
    draw_glyph_3x5(x, y, glyph_p, scale, color);
    x += glyph_w + spacing;
    draw_glyph_3x5(x, y, glyph_s, scale, color);
    x += glyph_w + spacing;

    for (int i = digit_count - 1; i >= 0; --i) {
        draw_glyph_3x5(x, y, digit_glyphs[digits[i]], scale, color);
        x += glyph_w + spacing;
    }
}

static DirtyRect get_fps_counter_rect(void) {
    return make_dirty_rect(g_screen.width - 160, 8, g_screen.width - 8, 24);
}



static void clear_screen(uint32_t color) {
    for (int i = 0; i < g_screen.width * g_screen.height; ++i) g_screen.pixels[i] = color;
}

static void clear_depth_buffer(void) {
    for (int i = 0; i < g_screen.width * g_screen.height; ++i) g_depth_buffer[i] = 0x7FFFFFFF;
}

static DirtyRect make_dirty_rect(int x0, int y0, int x1, int y1) {
    DirtyRect rect;
    rect.x0 = x0;
    rect.y0 = y0;
    rect.x1 = x1;
    rect.y1 = y1;
    return rect;
}

static bool dirty_rect_is_valid(DirtyRect rect) {
    return rect.x0 < rect.x1 && rect.y0 < rect.y1;
}

static DirtyRect clamp_dirty_rect(DirtyRect rect) {
    if (rect.x0 < 0) rect.x0 = 0;
    if (rect.y0 < 0) rect.y0 = 0;
    if (rect.x1 > g_screen.width) rect.x1 = g_screen.width;
    if (rect.y1 > g_screen.height) rect.y1 = g_screen.height;
    if (rect.x0 > rect.x1) rect.x0 = rect.x1;
    if (rect.y0 > rect.y1) rect.y0 = rect.y1;
    return rect;
}

static DirtyRect union_dirty_rects(DirtyRect a, DirtyRect b) {
    if (!dirty_rect_is_valid(a)) return b;
    if (!dirty_rect_is_valid(b)) return a;
    return make_dirty_rect(
        a.x0 < b.x0 ? a.x0 : b.x0,
        a.y0 < b.y0 ? a.y0 : b.y0,
        a.x1 > b.x1 ? a.x1 : b.x1,
        a.y1 > b.y1 ? a.y1 : b.y1
    );
}

static DirtyRect full_screen_dirty_rect(void) {
    return make_dirty_rect(0, 0, g_screen.width, g_screen.height);
}

static bool renderer_scene_state_equal(RendererSceneState a, RendererSceneState b) {
    return a.control_x == b.control_x &&
           a.control_y == b.control_y &&
           a.control_z == b.control_z &&
           a.cam_sin_yaw == b.cam_sin_yaw &&
           a.cam_cos_yaw == b.cam_cos_yaw &&
           a.cam_sin_pitch == b.cam_sin_pitch &&
           a.cam_cos_pitch == b.cam_cos_pitch &&
           a.theta_x == b.theta_x &&
           a.theta_y == b.theta_y &&
           a.theta_z == b.theta_z;
}

static void clear_dirty_rect(DirtyRect rect, uint32_t color) {
    rect = clamp_dirty_rect(rect);
    if (!dirty_rect_is_valid(rect)) return;

    for (int y = rect.y0; y < rect.y1; ++y) {
        int row = y * g_screen.width;
        for (int x = rect.x0; x < rect.x1; ++x) {
            int index = row + x;
            g_screen.pixels[index] = color;
            g_depth_buffer[index] = 0x7FFFFFFF;
        }
    }
}

static int wrap_angle_u8(int angle) {
    return angle & 255;
}

static int trig_index_from_radians(float radians) {
    const int angle_scale = 41;
    return wrap_angle_u8((int)(radians * angle_scale));
}

static int fixed_sin_u8(int angle) {
    static const int sin_table[65] = {
        0, 25, 50, 75, 100, 125, 150, 175, 200, 224, 249, 273, 297, 321, 345, 369,
        392, 415, 438, 460, 483, 505, 526, 548, 569, 590, 610, 630, 650, 669, 688, 706,
        724, 742, 759, 775, 792, 807, 822, 837, 851, 865, 878, 891, 903, 915, 926, 936,
        946, 955, 964, 972, 980, 987, 993, 999, 1004, 1009, 1013, 1016, 1019, 1021, 1023, 1024,
        1024,
    };
    int wrapped = wrap_angle_u8(angle);
    int index = wrapped & 127;

    if (index > 64) {
        index = 128 - index;
    }

    int value = sin_table[index];
    if (wrapped > 128) {
        value = -value;
    }

    return value;
}

static int fixed_cos_u8(int angle) {
    return fixed_sin_u8(angle + 64);
}

static void draw_circle(int center_x, int center_y, int radius, int triangle_count, uint32_t color) {
    if (radius <= 0) return;
    if (triangle_count < 3) triangle_count = 3;
    if (triangle_count > 256) triangle_count = 256;

    for (int i = 0; i < triangle_count; ++i) {
        int angle0 = (i * 256) / triangle_count;
        int angle1 = ((i + 1) * 256) / triangle_count;
        int x0 = center_x + ((radius * fixed_cos_u8(angle0)) >> 10);
        int y0 = center_y + ((radius * fixed_sin_u8(angle0)) >> 10);
        int x1 = center_x + ((radius * fixed_cos_u8(angle1)) >> 10);
        int y1 = center_y + ((radius * fixed_sin_u8(angle1)) >> 10);

        draw_triangle(center_x, center_y, x0, y0, x1, y1, color);
    }
}

static void draw_triangle_depth(
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    int depth,
    uint32_t color
) {
    if (y1 < y0) { int tx = x0; int ty = y0; x0 = x1; y0 = y1; x1 = tx; y1 = ty; }
    if (y2 < y0) { int tx = x0; int ty = y0; x0 = x2; y0 = y2; x2 = tx; y2 = ty; }
    if (y2 < y1) { int tx = x1; int ty = y1; x1 = x2; y1 = y2; x2 = tx; y2 = ty; }

    if (y0 == y2) return;

    int y_start = y0;
    int y_end = y2;
    if (y_end < 0 || y_start >= g_screen.height) return;
    if (y_start < 0) y_start = 0;
    if (y_end >= g_screen.height) y_end = g_screen.height - 1;

    Edge e02, e01, e12;
    init_edge(&e02, x0, y0, x2, y2);
    init_edge(&e01, x0, y0, x1, y1);
    init_edge(&e12, x1, y1, x2, y2);

    advance_edge_to(&e02, y_start);
    if (y_start < y1) {
        advance_edge_to(&e01, y_start);
    } else {
        advance_edge_to(&e12, y_start);
    }

    for (int y = y_start; y <= y_end; ++y) {
        int xa = e02.x;
        int xb = (y < y1) ? e01.x : e12.x;

        int x_start = (xa < xb) ? xa : xb;
        int x_end = (xa > xb) ? xa : xb;

        if (!(x_end < 0 || x_start >= g_screen.width)) {
            if (x_start < 0) x_start = 0;
            if (x_end >= g_screen.width) x_end = g_screen.width - 1;
            int row = y * g_screen.width;
            for (int x = x_start; x <= x_end; ++x) {
                int index = row + x;
                if (depth < g_depth_buffer[index]) {
                    g_depth_buffer[index] = depth;
                    g_screen.pixels[index] = color;
                }
            }
        }

        step_edge(&e02);
        if (y + 1 <= y_end) {
            if (y < y1) {
                step_edge(&e01);
            } else {
                step_edge(&e12);
            }
        }
    }
}

static void rotate_vertices(int (*vertices)[3], int vertex_count, int theta_x, int theta_y, int theta_z) {
    const int fp_shift = 10;
    const int sin_x = fixed_sin_u8(theta_x);
    const int cos_x = fixed_cos_u8(theta_x);
    const int sin_y = fixed_sin_u8(theta_y);
    const int cos_y = fixed_cos_u8(theta_y);
    const int sin_z = fixed_sin_u8(theta_z);
    const int cos_z = fixed_cos_u8(theta_z);

    for (int i = 0; i < vertex_count; ++i) {
        int x = vertices[i][0];
        int y = vertices[i][1];
        int z = vertices[i][2];

        int rot_x = x;
        int rot_y = ((y * cos_x) - (z * sin_x)) >> fp_shift;
        int rot_z = ((y * sin_x) + (z * cos_x)) >> fp_shift;

        int yaw_x = ((rot_x * cos_y) + (rot_z * sin_y)) >> fp_shift;
        int yaw_y = rot_y;
        int yaw_z = ((-rot_x * sin_y) + (rot_z * cos_y)) >> fp_shift;

        vertices[i][0] = ((yaw_x * cos_z) - (yaw_y * sin_z)) >> fp_shift;
        vertices[i][1] = ((yaw_x * sin_z) + (yaw_y * cos_z)) >> fp_shift;
        vertices[i][2] = yaw_z;
    }
}

static void translate_vertices(int (*vertices)[3], int vertex_count, int offset_x, int offset_y, int offset_z) {
    for (int i = 0; i < vertex_count; ++i) {
        vertices[i][0] += offset_x;
        vertices[i][1] += offset_y;
        vertices[i][2] += offset_z;
    }
}

static void init_demo_meshes(void) {
    if (g_demo_meshes_initialized) return;

    for (int i = 0; i < 6; ++i) {
        PrimitiveGeneratedPoints generated = primitive_shape_generate_points(g_demo_mesh_assets[i]);
        g_demo_meshes[i] = surface_mesher_build(
            generated.points,
            generated.point_count,
            900,
            g_demo_mesh_resolutions[i]
        );
        primitive_generated_points_free(&generated);
    }

    g_demo_meshes_initialized = true;
}

static DirtyRect draw_mesh_triangle(
    const int triangle_vertices[3][3],
    const RendererFrameContext *frame,
    uint32_t color
) {
    const int fp_shift = 10;
    int camera_x[3];
    int camera_y[3];
    int camera_z[3];
    int screen_x[3];
    int screen_y[3];
    DirtyRect dirty_rect = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);

    for (int i = 0; i < 3; ++i) {
        int final_x = triangle_vertices[i][0];
        int final_y = triangle_vertices[i][1];
        int final_z = triangle_vertices[i][2] + frame->world_offset_z;

        int translated_x = final_x - frame->control_x;
        int translated_y = final_y - frame->control_y;
        int translated_z = final_z - frame->control_z;

        int view_x = ((translated_x * frame->cam_cos_yaw) - (translated_z * frame->cam_sin_yaw)) >> fp_shift;
        int view_z = ((translated_x * frame->cam_sin_yaw) + (translated_z * frame->cam_cos_yaw)) >> fp_shift;
        int view_y = ((translated_y * frame->cam_cos_pitch) - (view_z * frame->cam_sin_pitch)) >> fp_shift;
        int depth = ((translated_y * frame->cam_sin_pitch) + (view_z * frame->cam_cos_pitch)) >> fp_shift;

        if (depth <= frame->near_plane) {
            return make_dirty_rect(g_screen.width, g_screen.height, 0, 0);
        }

        camera_x[i] = view_x;
        camera_y[i] = view_y;
        camera_z[i] = depth;
        screen_x[i] = frame->screen_center_x + (view_x * frame->focal_length) / depth;
        screen_y[i] = frame->screen_center_y - (view_y * frame->focal_length) / depth;

        if (screen_x[i] < dirty_rect.x0) dirty_rect.x0 = screen_x[i];
        if (screen_y[i] < dirty_rect.y0) dirty_rect.y0 = screen_y[i];
        if (screen_x[i] > dirty_rect.x1) dirty_rect.x1 = screen_x[i];
        if (screen_y[i] > dirty_rect.y1) dirty_rect.y1 = screen_y[i];
    }

    {
        int ab_x = camera_x[1] - camera_x[0];
        int ab_y = camera_y[1] - camera_y[0];
        int ab_z = camera_z[1] - camera_z[0];
        int ac_x = camera_x[2] - camera_x[0];
        int ac_y = camera_y[2] - camera_y[0];
        int ac_z = camera_z[2] - camera_z[0];
        int64_t normal_x = (int64_t)ab_y * ac_z - (int64_t)ab_z * ac_y;
        int64_t normal_y = (int64_t)ab_z * ac_x - (int64_t)ab_x * ac_z;
        int64_t normal_z = (int64_t)ab_x * ac_y - (int64_t)ab_y * ac_x;
        int64_t facing = normal_x * camera_x[0] + normal_y * camera_y[0] + normal_z * camera_z[0];

        if (facing <= 0) {
            return make_dirty_rect(g_screen.width, g_screen.height, 0, 0);
        }
    }

    if (dirty_rect_is_valid(dirty_rect)) {
        int depth = (camera_z[0] + camera_z[1] + camera_z[2]) / 3;
        dirty_rect.x0 -= 2;
        dirty_rect.y0 -= 2;
        dirty_rect.x1 += 3;
        dirty_rect.y1 += 3;
        dirty_rect = clamp_dirty_rect(dirty_rect);

        draw_triangle_depth(screen_x[0], screen_y[0], screen_x[1], screen_y[1], screen_x[2], screen_y[2], depth, color);
    }

    return dirty_rect;
}

static bool project_mesh_point(
    const int point[3],
    const RendererFrameContext *frame,
    int *screen_x,
    int *screen_y,
    int *depth
) {
    const int fp_shift = 10;
    int final_x = point[0];
    int final_y = point[1];
    int final_z = point[2] + frame->world_offset_z;
    int translated_x = final_x - frame->control_x;
    int translated_y = final_y - frame->control_y;
    int translated_z = final_z - frame->control_z;
    int view_x = ((translated_x * frame->cam_cos_yaw) - (translated_z * frame->cam_sin_yaw)) >> fp_shift;
    int view_z = ((translated_x * frame->cam_sin_yaw) + (translated_z * frame->cam_cos_yaw)) >> fp_shift;
    int view_y = ((translated_y * frame->cam_cos_pitch) - (view_z * frame->cam_sin_pitch)) >> fp_shift;
    int point_depth = ((translated_y * frame->cam_sin_pitch) + (view_z * frame->cam_cos_pitch)) >> fp_shift;

    if (point_depth <= frame->near_plane) {
        return false;
    }

    if (screen_x) *screen_x = frame->screen_center_x + (view_x * frame->focal_length) / point_depth;
    if (screen_y) *screen_y = frame->screen_center_y - (view_y * frame->focal_length) / point_depth;
    if (depth) *depth = point_depth;
    return true;
}

static int rotate_circle_vertices(
    const int center[3],
    int radius,
    int triangle_count,
    int theta_x,
    int theta_y,
    int theta_z,
    int circle_vertices[257][3]
) {
    if (radius <= 0) return 0;
    if (triangle_count < 3) triangle_count = 3;
    if (triangle_count > 256) triangle_count = 256;

    circle_vertices[0][0] = center[0];
    circle_vertices[0][1] = center[1];
    circle_vertices[0][2] = center[2];

    for (int i = 0; i < triangle_count; ++i) {
        int angle = (i * 256) / triangle_count;
        circle_vertices[i + 1][0] = center[0] + ((radius * fixed_cos_u8(angle)) >> 10);
        circle_vertices[i + 1][1] = center[1] + ((radius * fixed_sin_u8(angle)) >> 10);
        circle_vertices[i + 1][2] = center[2];
    }

    rotate_vertices(circle_vertices, triangle_count + 1, theta_x, theta_y, theta_z);
    return triangle_count;
}

static DirtyRect draw_mesh_circle(
    const int center[3],
    int radius,
    const RendererFrameContext *frame,
    int triangle_count,
    int offset_x,
    int offset_y,
    int offset_z,
    int theta_x,
    int theta_y,
    int theta_z,
    bool flip_winding,
    uint32_t color
) {
    int circle_vertices[257][3];
    DirtyRect circle_dirty = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);
    int clamped_triangle_count;

    if (!frame || radius <= 0) {
        return circle_dirty;
    }

    clamped_triangle_count = rotate_circle_vertices(
        center,
        radius,
        triangle_count,
        theta_x,
        theta_y,
        theta_z,
        circle_vertices
    );
    translate_vertices(circle_vertices, clamped_triangle_count + 1, offset_x, offset_y, offset_z);

    for (int i = 0; i < clamped_triangle_count; ++i) {
        int next = (i + 1) % clamped_triangle_count;
        int triangle_vertices[3][3] = {
            {circle_vertices[0][0], circle_vertices[0][1], circle_vertices[0][2]},
            {
                circle_vertices[flip_winding ? next + 1 : i + 1][0],
                circle_vertices[flip_winding ? next + 1 : i + 1][1],
                circle_vertices[flip_winding ? next + 1 : i + 1][2]
            },
            {
                circle_vertices[flip_winding ? i + 1 : next + 1][0],
                circle_vertices[flip_winding ? i + 1 : next + 1][1],
                circle_vertices[flip_winding ? i + 1 : next + 1][2]
            }
        };

        circle_dirty = union_dirty_rects(
            circle_dirty,
            draw_mesh_triangle(triangle_vertices, frame, color)
        );
    }

    return circle_dirty;
}

static DirtyRect draw_meshed_asset(
    const SurfaceMesherResult *mesh,
    const RendererFrameContext *frame,
    int offset_x,
    int offset_y,
    int offset_z,
    int theta_x,
    int theta_y,
    int theta_z
) {
    DirtyRect asset_dirty = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);

    if (!mesh || !mesh->success) {
        return asset_dirty;
    }

    for (size_t i = 0; i < mesh->triangle_count; ++i) {
        int triangle_vertices[3][3];
        DirtyRect triangle_dirty;

        memcpy(triangle_vertices, mesh->triangles[i].vertices, sizeof(triangle_vertices));
        rotate_vertices(triangle_vertices, 3, theta_x, theta_y, theta_z);
        translate_vertices(triangle_vertices, 3, offset_x, offset_y, offset_z);
        triangle_dirty = draw_mesh_triangle(triangle_vertices, frame, g_cube_face_colors[i % 12]);
        asset_dirty = union_dirty_rects(asset_dirty, triangle_dirty);
    }

    return asset_dirty;
}

static void draw_cube(
    const int cube_vertices[8][3],
    const RendererFrameContext *frame,
    DirtyRect *out_dirty_rect
) {
    const int fp_shift = 10;
    int camera_x[8];
    int camera_y[8];
    int camera_z[8];
    int screen_x[8];
    int screen_y[8];
    bool vertex_visible[8];
    DirtyRect dirty_rect = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);

    for (int i = 0; i < 8; ++i) {
        int final_x = cube_vertices[i][0];
        int final_y = cube_vertices[i][1];
        int final_z = cube_vertices[i][2] + frame->world_offset_z;

        int translated_x = final_x - frame->control_x;
        int translated_y = final_y - frame->control_y;
        int translated_z = final_z - frame->control_z;

        int view_x = ((translated_x * frame->cam_cos_yaw) - (translated_z * frame->cam_sin_yaw)) >> fp_shift;
        int view_z = ((translated_x * frame->cam_sin_yaw) + (translated_z * frame->cam_cos_yaw)) >> fp_shift;
        int view_y = ((translated_y * frame->cam_cos_pitch) - (view_z * frame->cam_sin_pitch)) >> fp_shift;
        int depth = ((translated_y * frame->cam_sin_pitch) + (view_z * frame->cam_cos_pitch)) >> fp_shift;

        camera_x[i] = view_x;
        camera_y[i] = view_y;
        camera_z[i] = depth;
        vertex_visible[i] = depth > frame->near_plane;

        if (vertex_visible[i]) {
            screen_x[i] = frame->screen_center_x + (view_x * frame->focal_length) / depth;
            screen_y[i] = frame->screen_center_y - (view_y * frame->focal_length) / depth;
            if (screen_x[i] < dirty_rect.x0) dirty_rect.x0 = screen_x[i];
            if (screen_y[i] < dirty_rect.y0) dirty_rect.y0 = screen_y[i];
            if (screen_x[i] > dirty_rect.x1) dirty_rect.x1 = screen_x[i];
            if (screen_y[i] > dirty_rect.y1) dirty_rect.y1 = screen_y[i];
        }
    }

    if (dirty_rect_is_valid(dirty_rect)) {
        dirty_rect.x0 -= 2;
        dirty_rect.y0 -= 2;
        dirty_rect.x1 += 3;
        dirty_rect.y1 += 3;
        dirty_rect = clamp_dirty_rect(dirty_rect);
    }

    if (out_dirty_rect) {
        *out_dirty_rect = dirty_rect;
    }

    int sorted_faces[12];
    int face_depths[12];
    int face_count = 0;

    for (int i = 0; i < 12; ++i) {
        int a = g_cube_faces[i][0];
        int b = g_cube_faces[i][1];
        int c = g_cube_faces[i][2];

        if (!vertex_visible[a] || !vertex_visible[b] || !vertex_visible[c]) continue;

        int ab_x = camera_x[b] - camera_x[a];
        int ab_y = camera_y[b] - camera_y[a];
        int ab_z = camera_z[b] - camera_z[a];
        int ac_x = camera_x[c] - camera_x[a];
        int ac_y = camera_y[c] - camera_y[a];
        int ac_z = camera_z[c] - camera_z[a];
        int64_t normal_x = (int64_t)ab_y * ac_z - (int64_t)ab_z * ac_y;
        int64_t normal_y = (int64_t)ab_z * ac_x - (int64_t)ab_x * ac_z;
        int64_t normal_z = (int64_t)ab_x * ac_y - (int64_t)ab_y * ac_x;
        int64_t facing = normal_x * camera_x[a] + normal_y * camera_y[a] + normal_z * camera_z[a];

        if (facing <= 0) continue;

        sorted_faces[face_count] = i;
        face_depths[face_count] = (camera_z[a] + camera_z[b] + camera_z[c]) / 3;
        ++face_count;
    }

    for (int i = 1; i < face_count; ++i) {
        int face_index = sorted_faces[i];
        int depth = face_depths[i];
        int j = i - 1;

        while (j >= 0 && face_depths[j] < depth) {
            sorted_faces[j + 1] = sorted_faces[j];
            face_depths[j + 1] = face_depths[j];
            --j;
        }

        sorted_faces[j + 1] = face_index;
        face_depths[j + 1] = depth;
    }

    for (int i = 0; i < face_count; ++i) {
        int face = sorted_faces[i];
        int a = g_cube_faces[face][0];
        int b = g_cube_faces[face][1];
        int c = g_cube_faces[face][2];

        draw_triangle_depth(
            screen_x[a], screen_y[a],
            screen_x[b], screen_y[b],
            screen_x[c], screen_y[c],
            face_depths[i],
            g_cube_face_colors[face]
        );
    }
}

#include "assets/cylinder.h"
#include "assets/sphere.h"
#include "assets/rectangular_prism.h"
#include "assets/pyramid.h"
#include "assets/ground.h"
#include "assets/map.h"

static DirtyRect render_pixels(void) {
    static int theta_x = 0;
    static int theta_y = 0;
    static int theta_z = 0;
    static bool dirty_initialized = false;
    static DirtyRect previous_scene_dirty = {0, 0, 0, 0};
    static RendererSceneState previous_scene_state = {0};
    static int previous_fps = -1;
    RendererFrameContext frame;
    RendererSceneState scene_state;

    init_demo_meshes();

    theta_x = wrap_angle_u8(theta_x + 1);
    theta_y = wrap_angle_u8(theta_y + 1);
    theta_z = wrap_angle_u8(theta_z + 1);

    frame.focal_length = (g_screen.width * 98) / 100;
    frame.near_plane = 0;
    frame.world_offset_z = 900;
    frame.screen_center_x = g_screen.width / 2;
    frame.screen_center_y = g_screen.height / 2;
    frame.control_x = (int)g_control.pos_x;
    frame.control_y = (int)g_control.pos_y;
    frame.control_z = (int)g_control.pos_z;

    {
        int yaw_index = trig_index_from_radians(g_control.yaw);
        int pitch_index = trig_index_from_radians(g_control.pitch);
        frame.cam_sin_yaw = fixed_sin_u8(yaw_index);
        frame.cam_cos_yaw = fixed_cos_u8(yaw_index);
        frame.cam_sin_pitch = fixed_sin_u8(pitch_index);
        frame.cam_cos_pitch = fixed_cos_u8(pitch_index);
    }

    scene_state.control_x = frame.control_x;
    scene_state.control_y = frame.control_y;
    scene_state.control_z = frame.control_z;
    scene_state.cam_sin_yaw = frame.cam_sin_yaw;
    scene_state.cam_cos_yaw = frame.cam_cos_yaw;
    scene_state.cam_sin_pitch = frame.cam_sin_pitch;
    scene_state.cam_cos_pitch = frame.cam_cos_pitch;
    scene_state.theta_x = theta_x;
    scene_state.theta_y = theta_y;
    scene_state.theta_z = theta_z;

    {
        DirtyRect fps_dirty = get_fps_counter_rect();
        bool scene_changed = !dirty_initialized ||
            !renderer_scene_state_equal(scene_state, previous_scene_state);
        bool fps_changed = !dirty_initialized || g_fps != previous_fps;
        DirtyRect present_dirty = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);
        DirtyRect current_scene_dirty = make_dirty_rect(g_screen.width, g_screen.height, 0, 0);

        if (!scene_changed && !fps_changed) {
            return present_dirty;
        }

        if (!dirty_initialized) {
            clear_screen(0x00000000);
            clear_depth_buffer();
            present_dirty = full_screen_dirty_rect();
        } else {
            if (scene_changed) {
                clear_dirty_rect(previous_scene_dirty, 0x00000000);
                present_dirty = union_dirty_rects(present_dirty, previous_scene_dirty);
            }
            if (fps_changed) {
                clear_dirty_rect(fps_dirty, 0x00000000);
                present_dirty = union_dirty_rects(present_dirty, fps_dirty);
            }
        }

        if (scene_changed) {
            /*for (int i = 0; i < 6; ++i) {
                DirtyRect asset_dirty = draw_meshed_asset(
                    &g_demo_meshes[i],
                    &frame,
                    g_demo_mesh_offsets[i][0],
                    g_demo_mesh_offsets[i][1],
                    g_demo_mesh_offsets[i][2],
                    theta_x,
                    theta_y,
                    theta_z
                );
                current_scene_dirty = union_dirty_rects(current_scene_dirty, asset_dirty);
            }*/
            current_scene_dirty = union_dirty_rects(
                current_scene_dirty,
                draw_map_mesh(&frame, theta_x, theta_y, theta_z)
            );

            {
                static const int example_ground[4][3] = {
                    {-520, -340, -160},
                    { 520, -340, -160},
                    { 520,  340, -160},
                    {-520,  340, -160}
                };
                DirtyRect ground_dirty = draw_ground_tile_mesh(
                    example_ground,
                    &frame,
                    0,
                    -480,
                    0,
                    64,
                    0,
                    0,
                    0x002F7D32,
                    0x006B4A2D
                );
                current_scene_dirty = union_dirty_rects(current_scene_dirty, ground_dirty);
            }

            previous_scene_dirty = current_scene_dirty;
            previous_scene_state = scene_state;
            present_dirty = union_dirty_rects(present_dirty, current_scene_dirty);
        }

        if (fps_changed || scene_changed) {
            draw_fps_counter();
            present_dirty = union_dirty_rects(present_dirty, fps_dirty);
            previous_fps = g_fps;
        }

        dirty_initialized = true;
        return clamp_dirty_rect(present_dirty);
    }
}

#endif // RENDERER_H
