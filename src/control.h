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
            g_control.yaw -= (float)dx * sensitivity;
            g_control.pitch -= (float)dy * sensitivity;

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
    g_control.right_z = -(float)sin(g_control.yaw);

    // Handle Movement
    float move_speed = 5.0f;

    if (g_control.keys['W']) {
        g_control.pos_x -= g_control.dir_x * move_speed;
        g_control.pos_z += g_control.dir_z * move_speed;
    }
    if (g_control.keys['S']) {
        g_control.pos_x += g_control.dir_x * move_speed;
        g_control.pos_z -= g_control.dir_z * move_speed;
    }
    if (g_control.keys['A']) {
        g_control.pos_x -= g_control.right_x * move_speed;
        g_control.pos_z += g_control.right_z * move_speed;
    }
    if (g_control.keys['D']) {
        g_control.pos_x += g_control.right_x * move_speed;
        g_control.pos_z -= g_control.right_z * move_speed;
    }


    // Vertical movement
    if (g_control.keys['Q']) g_control.pos_y -= move_speed;
    if (g_control.keys['E']) g_control.pos_y += move_speed;
}

#endif // CONTROL_H
