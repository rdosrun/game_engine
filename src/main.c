#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "screen.h"
#include "control.h"
#include "renderer.h"

// Initialize singleton instances
static uint32_t pixel_memory[WINDOW_WIDTH * WINDOW_HEIGHT];
Screen g_screen = {
    .pixels = pixel_memory,
    .width = WINDOW_WIDTH,
    .height = WINDOW_HEIGHT
};

Controller g_control = {
    .pos_x = 0.0f,
    .pos_y = 0.0f,
    .pos_z = -500.0f,
    .yaw = 0.0f,
    .pitch = 0.0f,
    .keys = {0},
    .mouse_down = false
};

static bool is_running = true;
static bool is_focused = true;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_KEYDOWN:
            if (wParam < 256) g_control.keys[wParam] = true;
            return 0;
        case WM_KEYUP:
            if (wParam < 256) g_control.keys[wParam] = false;
            return 0;
        case WM_SETFOCUS:
            is_focused = true;
            ShowCursor(FALSE);
            return 0;
        case WM_KILLFOCUS:
            is_focused = false;
            ShowCursor(TRUE);
            return 0;
        case WM_CLOSE:
            is_running = false;
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            is_running = false;
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    const wchar_t CLASS_NAME[] = L"Sample Window Class";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassW(&wc)) return 0;

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"3D View", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, g_screen.width, g_screen.height,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;
    ShowWindow(hwnd, nShowCmd);

    HDC hdc = GetDC(hwnd);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = g_screen.width;
    bmi.bmiHeader.biHeight = -g_screen.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    double target_frame_time = 1.0 / 60.0;
    LARGE_INTEGER last_time;
    QueryPerformanceCounter(&last_time);

    while (is_running) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Call the movement and look logic from control.h
        update_controls(hwnd, is_focused);

        render_pixels();

        StretchDIBits(
            hdc, 0, 0, g_screen.width, g_screen.height,
            0, 0, g_screen.width, g_screen.height,
            g_screen.pixels, &bmi, DIB_RGB_COLORS, SRCCOPY
        );

        LARGE_INTEGER current_time;
        QueryPerformanceCounter(&current_time);
        double elapsed_time = (double)(current_time.QuadPart - last_time.QuadPart) / frequency.QuadPart;

        if (elapsed_time < target_frame_time) {
            DWORD sleep_ms = (DWORD)((target_frame_time - elapsed_time) * 1000);
            if (sleep_ms > 0) Sleep(sleep_ms);
            do {
                QueryPerformanceCounter(&current_time);
                elapsed_time = (double)(current_time.QuadPart - last_time.QuadPart) / frequency.QuadPart;
            } while (elapsed_time < target_frame_time);
        }
        last_time = current_time;
    }

    ReleaseDC(hwnd, hdc);
    return 0;
}
