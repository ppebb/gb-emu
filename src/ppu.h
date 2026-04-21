#pragma once

#include "defs.h"
#include <stdint.h>

typedef struct _Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

extern Color frame_buffer[WIDTH * HEIGHT];

void ppu_step(int cycles);
