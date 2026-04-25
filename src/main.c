#include "cpu.h"
#include "defs.h"
#include "input.h"
#include "instructions.h"
#include "mem.h"
#include "ppu.h"
#include "rom.h"
#include "sprite_window.h"
#include "tiledata_window.h"
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

typedef struct _Arg {
    char *desc;
    void (*func)(int *idx, int argc, char **argv);
    char *strings[3];
} Arg;

#define ARG(_func, _desc, ...)                         \
    (Arg) {                                            \
        _desc, (void (*)(int *, int, char **))_func, { \
            __VA_ARGS__                                \
        }                                              \
    }

void a_print_help();

bool step_debug = false;
long scale = 1;
bool tiledata_window_enabled = false;
bool sprite_viewer_enabled = false;

void a_step_debug() {
    step_debug = true;
}

void a_scale(int *idx, int argc, char **argv) {
    if (argc <= *idx + 1) {
        fprintf(stderr, "Missing argument for --size. Pass an integer as your next argument.\n");
        exit(1);
    }

    const char *size_str = argv[*idx + 1];
    long parsed_size = strtol(size_str, NULL, 10);

    if (parsed_size == -1) {
        fprintf(stderr, "Invalid size passed as argument to --size!\n");
        exit(1);
    }

    scale = parsed_size;

    (*idx)++;
}

void a_rom_info(int *_0, int _1, char **argv) {
    Rom *rom = rom_new(argv[1]);
    rom_info(rom);
    rom_free(rom);

    exit(0);
}

void a_tiledata_viewer() {
    tiledata_window_enabled = true;
}

void a_sprite_viewer() {
    sprite_viewer_enabled = true;
}

// clang-format off
Arg args[] = {
    ARG(a_print_help, "Print this message", "-h", "--help", NULL),
    ARG(a_scale, "Set the scale [-s LONG]", "-s", "--scale", NULL),
    ARG(a_step_debug, "Enable step-by-step debugging", "--step-debug", NULL),
    ARG(a_rom_info, "Display ROM info", "--rom-info", NULL),
    ARG(a_tiledata_viewer, "Enable the tile-data viewer", "-tdv", "--tile-data", NULL),
    ARG(a_sprite_viewer, "Enable the sprite viewer", "-sv", "--sprite-viewer", NULL),
};

const size_t args_len = sizeof(args) / sizeof(args[0]);
// clang-format on

#define ALIGN 25

void a_print_help() {
    printf("./gb ROM [options]\n\n");

    for (size_t i = 0; i < args_len; i++) {
        Arg arg = args[i];

        size_t c = 0;

        for (size_t j = 0; arg.strings[j] != NULL; j++) {
            c += strlen(arg.strings[j]);

            if (arg.strings[j + 1] == NULL) {
                printf("%s", arg.strings[j]);

                for (; c < ALIGN; c++)
                    printf("%c", ' ');
            }
            else {
                printf("%s|", arg.strings[j]);
                c += 1;
            }
        }

        printf("%s\n", arg.desc);
    }

    exit(1);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "No args provided! Pass a rom file as the first argument\n");
        a_print_help();
    }

    for (int i = 2; i < argc; i++) {
        char *arg_str = argv[i];

        for (int j = 0; j < args_len; j++) {
            Arg arg = args[j];

            for (int k = 0; arg.strings[k] != NULL; k++) {
                if (strcmp(arg_str, arg.strings[k]) == 0) {
                    arg.func(&i, argc, argv);

                    goto next_arg;
                }
            }
        }

        fprintf(stderr, "Unknown argument %s!\n", arg_str);
        a_print_help();

    next_arg:
    }

    char *rom_path = argv[1];
    Rom *rom = rom_new(rom_path);
    rom_info(rom);
    if (!rom) {
        fprintf(stderr, "Failed to load rom!\n");
        exit(1);
    }

    mem_init(rom);
    cpu_init(step_debug);
    instruction_init();

    GLFWwindow *window;

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(WIDTH * scale, HEIGHT * scale, "ppebulatorGB", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    input_init(window);

    if (tiledata_window_enabled)
        tiledata_window_init(window, scale);

    if (sprite_viewer_enabled)
        sprite_window_init(window, scale);

    glfwMakeContextCurrent(window);

    int cycles_this_frame = 0;
    struct timespec frame_start;

    while (!glfwWindowShouldClose(window)) {
        while (cycles_this_frame < CYCLES_PER_FRAME) {
            /* if (stop) { */
            /*     continue; */
            /*     cycles_this_frame += 1; */

            /*     // TODO: Check RESET Terminal? */
            /* } */

            int cycles = cpu_step();
            cycles_this_frame += cycles;

            timer_step(cycles);
            ppu_step(cycles);

            // Interrupts cost 5 cycles, if one fires we need to add an extra 5
            // cycles to keep timing intact
            if (cpu_run_interrupts()) {
                cycles_this_frame += 5;
                timer_step(5);
                ppu_step(5);
            }
        }

        if (tiledata_window_enabled)
            tiledata_window_draw();

        if (sprite_viewer_enabled)
            sprite_window_draw();

        glfwMakeContextCurrent(window);

        // TODO: Draw to a texture.
        glClear(GL_COLOR_BUFFER_BIT);
        glRasterPos2f(-1, 1);
        glPixelZoom(scale, -scale);
        // Draw framebuffer populated by the ppu
        glDrawPixels(WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, frame_buffer);
        glfwSwapBuffers(window);
        input_poll();

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
    }

    rom_free(rom);

    if (tiledata_window_enabled)
        tiledata_window_finish();

    glfwTerminate();
    return 0;
}
