#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef struct {
    uint32_t* pixels;
    int width;
    int height;
} Screen;

// Global singleton instance
extern Screen g_screen;

#endif // SCREEN_H
