#pragma once

#include "defs.h"
#include <stdint.h>

typedef struct _Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

#define COLOR_WHITE ((Color){ .r = 255, .g = 255, .b = 255 })
#define COLOR_LIGHT_GRAY ((Color){ .r = 192, .g = 192, .b = 192 })
#define COLOR_DARK_GRAY ((Color){ .r = 96, .g = 96, .b = 96 })
#define COLOR_BLACK ((Color){ .r = 0, .g = 0, .b = 0 })

typedef struct __attribute__((packed)) _OAMEntry {
    uint8_t y_pos;
    uint8_t x_pos;
    uint8_t tile_idx;
    uint8_t cgb_palette : 3;
    bool bank : 1;
    bool dmg_palette : 1;
    bool x_flip : 1;
    bool y_flip : 1;
    bool priority : 1;
} OAMEntry;

extern Color frame_buffer[WIDTH * HEIGHT];

Color get_color(uint8_t color_idx, uint8_t palette_idx);
void ppu_step(int cycles);
