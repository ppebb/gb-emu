#include "tiledata_window.h"
#include "defs.h"
#include "ppu.h"
#include "src/mem.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define TD_WIDTH (16 * 8)
#define TD_HEIGHT (24 * 8)

int td_scale;
GLFWwindow *td_window;
Color td_frame_buffer[WIDTH * 2 * HEIGHT];

void tiledata_window_init(GLFWwindow *main, int scale) {
    td_scale = scale;

    td_window = glfwCreateWindow(TD_WIDTH * scale, TD_HEIGHT * scale, "Tiledata View", NULL, main);
    if (!td_window) {
        fprintf(stderr, "Failed to open tile-data window.\n");
        glfwTerminate();
        exit(1);
    }
}

void tiledata_draw_inner(uint16_t offset, uint8_t fb_y_offset) {
    for (uint8_t i = 0; i < 0x80; i++) {
        uint8_t tile_data[TILE_SIZE];

        for (uint8_t j = 0; j < TILE_SIZE; j++)
            tile_data[j] = mem_read_byte(offset + i * TILE_SIZE + j);

        for (uint8_t y = 0; y < 8; y++) {
            uint8_t tile_data_0 = tile_data[y * 2];
            uint8_t tile_data_1 = tile_data[y * 2 + 1];

            // Divided by 16 because of 16 tiles per row
            // Tiles are 8 px w/h
            uint8_t y_off = fb_y_offset + (i / 16) * 8 + y;

            for (int x = 0; x < 8; x++) {
                int color_bit = (x - 7) * -1;
                uint8_t color_idx = (tile_data_1 >> color_bit & 1) << 1;
                color_idx |= (tile_data_0 >> color_bit & 1);

                Color color = get_color(color_idx, 0);

                uint8_t x_off = (i % 16) * 8 + x;

                td_frame_buffer[y_off * TD_WIDTH + x_off] = color;
            }
        }
    }
}

void tiledata_window_draw() {
    tiledata_draw_inner(TM_AREA_A, 0);
    tiledata_draw_inner(TM_AREA_B, 8 * 8);
    tiledata_draw_inner(TM_AREA_B + 0x800, 16 * 8);

    glfwMakeContextCurrent(td_window);
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos2f(-1, 1);
    glPixelZoom(td_scale, -td_scale);
    glDrawPixels(TD_WIDTH, TD_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, td_frame_buffer);
    glfwSwapBuffers(td_window);
}

void tiledata_window_finish() {
    glfwDestroyWindow(td_window);
}
