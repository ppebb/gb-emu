#include "cpu.h"
#include "defs.h"
#include "instructions.h"
#include "mem.h"
#include "rom.h"
#include "src/ppu.h"
#include "timer.h"
#include <bits/time.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

static void diff_timespec(struct timespec *res, struct timespec *a, struct timespec *b) {
    res->tv_sec = a->tv_sec - b->tv_sec - (a->tv_nsec < b->tv_nsec);
    res->tv_nsec = a->tv_nsec - b->tv_nsec + (a->tv_nsec < b->tv_nsec) * 1E9;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "No args provided! Pass a rom file as the first argument\n");
        return 1;
    }

    char *rom_path = argv[1];
    Rom *rom = rom_new(rom_path);

    bool step_debug = false;

    for (size_t i = 2; i < argc; i++)
        if (strcmp("--step-debug", argv[i]) == 0)
            step_debug = true;

    mem_init(rom);
    cpu_init(step_debug);
    instruction_init();

    GLFWwindow *window;

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(WIDTH, HEIGHT, "ppebulatorGB", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    int cycles_this_frame = 0;
    struct timespec frame_start;

    while (!glfwWindowShouldClose(window)) {
        while (cycles_this_frame < CYCLES_PER_FRAME) {
            if (stop) {
                continue;
                cycles_this_frame += 1;

                // TODO: Check RESET Terminal?
            }

            int cycles = cpu_step();
            cycles_this_frame += cycles;

            timer_step(cycles);

            /* ppu_step(cycles); */

            // Interrupts cost 5 cycles, if one fires we need to add an extra 5
            // cycles to keep timing intact
            if (cpu_run_interrupts())
                cycles_this_frame += 5;
        }

        // TODO: Draw to a texture.
        glClear(GL_COLOR_BUFFER_BIT);
        glRasterPos2f(-1, 1);
        glPixelZoom(1, -1);
        // Draw framebuffer populated by the ppu
        glDrawPixels(WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, frame_buffer);
        glfwSwapBuffers(window);

        cycles_this_frame -= CYCLES_PER_FRAME;

        // Calculate time taken to run the correct number of cycles this frame
        struct timespec frame_end;
        clock_gettime(CLOCK_MONOTONIC, &frame_end);

        struct timespec diff;
        diff_timespec(&diff, &frame_end, &frame_start);

        const int interval = 1E9 / 60;
        struct timespec interveral_spec = { interval / 1E9, interval % (int)1E9 };
        struct timespec wait;
        diff_timespec(&wait, &interveral_spec, &diff);

        if (wait.tv_sec >= 0 || wait.tv_nsec > 0)
            clock_nanosleep(CLOCK_MONOTONIC, 0, &wait, NULL);
        else
            fprintf(stderr, "Frame took too long! %ld:%09ld\n", diff.tv_sec, diff.tv_nsec);

        clock_gettime(CLOCK_MONOTONIC, &frame_start);

        glfwPollEvents();
    }

    rom_free(rom);
    glfwTerminate();
    return 0;
}
