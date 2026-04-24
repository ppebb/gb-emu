#include "sprite_window.h"
#include "defs.h"
#include "mem.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define S_WIDTH 40
#define S_HEIGHT 64

int sv_scale;
GLFWwindow *sv_window;
Color sv_frame_buffer[S_WIDTH * S_HEIGHT];

// TODO: Handle two tile sprite mode

void sprite_window_init(GLFWwindow *main, int scale) {
    sv_scale = scale * 2;

    sv_window = glfwCreateWindow(S_WIDTH * sv_scale, S_HEIGHT * sv_scale, "Sprite Viewer", NULL, main);
    if (!sv_window) {
        fprintf(stderr, "Failed to open sprite window.\n");
        glfwTerminate();
        exit(1);
    }
}

void sprite_window_draw() {
    OAMEntry *oam = mem_get_oam_entries();

    for (size_t i = 0; i < OAM_LEN; i++) {
        OAMEntry entry = oam[i];
        uint8_t tile_id = entry.tile_idx;
        uint16_t tile_addr = TM_AREA_A + tile_id * TILE_SIZE;

        uint8_t tile_data[TILE_SIZE];

        // TODO: Abstract tile drawing algorithm shared by sprite_window and tiledata_window
        // TODO: Rename to sprite_viewer and tiledata_viewer
        // TODO: Show metadata on click in console?
        // TODO: Handle any OAM bits for drawing
        for (uint8_t j = 0; j < TILE_SIZE; j++)
            tile_data[j] = mem_read_byte(tile_addr + j);

        for (uint8_t y = 0; y < 8; y++) {
            uint8_t tile_data_0 = tile_data[y * 2];
            uint8_t tile_data_1 = tile_data[y * 2 + 1];

            // Divided by 16 because of 16 tiles per row
            // Tiles are 8 px w/h
            uint8_t y_off = (i / 5) * 8 + y;

            for (int x = 0; x < 8; x++) {
                int color_bit = (x - 7) * -1;
                uint8_t color_idx = (tile_data_1 >> color_bit & 1) << 1;
                color_idx |= (tile_data_0 >> color_bit & 1);

                Color color = get_color(color_idx, entry.dmg_palette ? 2 : 1);

                uint8_t x_off = (i % 5) * 8 + x;

                sv_frame_buffer[y_off * S_WIDTH + x_off] = color;
            }
        }
    }

    glfwMakeContextCurrent(sv_window);
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos2f(-1, 1);
    glPixelZoom(sv_scale, -sv_scale);
    glDrawPixels(S_WIDTH, S_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, sv_frame_buffer);
    glfwSwapBuffers(sv_window);
}

void sprite_window_finish() {
    glfwDestroyWindow(sv_window);
}
