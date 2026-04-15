#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>
#include <math.h>
#include <windows.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    float pos_x, pos_y, pos_z;
    float yaw, pitch;

    // Forward vectors for movement
    float dir_x, dir_y, dir_z;
    float right_x, right_z;

    bool keys[256];
    int mouse_x, mouse_y;
    bool mouse_down;
} Controller;

extern Controller g_control;

static bool collides_with_scene(float x, float y, float z) {
    const float player_radius = 24.0f;
    const float player_height = 60.0f;
    const float cube_half_size = 120.0f;
    const float cube_centers_x[5] = {240.0f, 480.0f, 720.0f, 960.0f, 1200.0f};
    const float cube_center_y = 0.0f;
    const float cube_center_z = 900.0f;

    for (int i = 0; i < 5; ++i) {
        float min_x = cube_centers_x[i] - cube_half_size - player_radius;
        float max_x = cube_centers_x[i] + cube_half_size + player_radius;
        float min_y = cube_center_y - cube_half_size - player_height;
        float max_y = cube_center_y + cube_half_size + player_height;
        float min_z = cube_center_z - cube_half_size - player_radius;
        float max_z = cube_center_z + cube_half_size + player_radius;

        if (x >= min_x && x <= max_x &&
            y >= min_y && y <= max_y &&
            z >= min_z && z <= max_z) {
            return true;
        }
    }

    return false;
}

static void try_move_controller(float delta_x, float delta_y, float delta_z) {
    float next_x = g_control.pos_x + delta_x;
    float next_y = g_control.pos_y + delta_y;
    float next_z = g_control.pos_z + delta_z;

    if (!collides_with_scene(next_x, g_control.pos_y, g_control.pos_z)) {
        g_control.pos_x = next_x;
    }
    if (!collides_with_scene(g_control.pos_x, next_y, g_control.pos_z)) {
        g_control.pos_y = next_y;
    }
    if (!collides_with_scene(g_control.pos_x, g_control.pos_y, next_z)) {
        g_control.pos_z = next_z;
    }
}

static void update_controls(HWND hwnd, bool is_focused) {
    if (is_focused) {
        // Handle Mouse Look (Additive)
        POINT center = { 400, 300 }; // Use center of screen (or window)
        ClientToScreen(hwnd, &center);

        POINT pt;
        GetCursorPos(&pt);

        int dx = pt.x - center.x;
        int dy = pt.y - center.y;

        if (dx != 0 || dy != 0) {
            float sensitivity = 0.002f;
            g_control.yaw += (float)dx * sensitivity;
            g_control.pitch += (float)dy * sensitivity;

            // Clamp pitch to prevent flipping
            if (g_control.pitch > (float)M_PI/2.1f) g_control.pitch = (float)M_PI/2.1f;
            if (g_control.pitch < -(float)M_PI/2.1f) g_control.pitch = -(float)M_PI/2.1f;

            SetCursorPos(center.x, center.y);
        }
    }

    // Update Direction Vectors
    g_control.dir_x = (float)sin(g_control.yaw);
    g_control.dir_z = (float)cos(g_control.yaw);

    // Right vector is 90 degrees offset from yaw
    g_control.right_x = (float)cos(g_control.yaw);
    g_control.right_z = (float)sin(g_control.yaw);

    // Handle Movement
    float move_speed = 5.0f;
    float move_x = 0.0f;
    float move_y = 0.0f;
    float move_z = 0.0f;

    if (g_control.keys['W']) {
        move_x += g_control.dir_x * move_speed;
        move_z += g_control.dir_z * move_speed;
    }
    if (g_control.keys['S']) {
        move_x -= g_control.dir_x * move_speed;
        move_z -= g_control.dir_z * move_speed;
    }
    if (g_control.keys['A']) {
        move_x -= g_control.right_x * move_speed;
        move_z += g_control.right_z * move_speed;
    }
    if (g_control.keys['D']) {
        move_x += g_control.right_x * move_speed;
        move_z -= g_control.right_z * move_speed;
    }


    // Vertical movement
    if (g_control.keys['Q']) move_y -= move_speed;
    if (g_control.keys['E']) move_y += move_speed;

    try_move_controller(move_x, move_y, move_z);
}

#endif // CONTROL_H
