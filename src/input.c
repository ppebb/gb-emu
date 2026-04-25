#include "input.h"
#include "cpu.h"
#include "defs.h"
#include "mem.h"
#include <GLFW/glfw3.h>

// From https://gbdev.io/pandocs/Joypad_Input.html
typedef enum _JOYPAD_BITS {
    A_RIGHT = 1 << 0,
    B_LEFT = 1 << 1,
    SELECT_UP = 1 << 2,
    START_DOWN = 1 << 3,
    // These control what the above bits report
    DPAD = 1 << 4,
    SELECT = 1 << 5,
} JOYPAD_BITS;

// Bit 1 implies unpressed
// Only bottom 4 bits are used
uint8_t jp_dpad_bits = 0x0F;
uint8_t jp_select_bits = 0x0F;

#define SET_INPUT_DPAD(bits, action)                          \
    bool _d_##bits = (action == GLFW_RELEASE);                \
    jp_dpad_bits = (jp_dpad_bits & ~bits) | _d_##bits * bits; \
    if (!(jp_state & DPAD))                                   \
        cpu_req_interrupt(INT_JOYPAD);

#define SET_INPUT_SELECT(bits, action)                            \
    bool _s_##bits = (action == GLFW_RELEASE);                    \
    jp_select_bits = (jp_select_bits & ~bits) | _s_##bits * bits; \
    if (!(jp_state & SELECT))                                     \
        cpu_req_interrupt(INT_JOYPAD);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    uint8_t jp_state = read_io(JOYPAD_ADDR);

    if (action != GLFW_PRESS && action != GLFW_RELEASE)
        return;

    switch (key) {
        case GLFW_KEY_I: // UP
            SET_INPUT_DPAD(SELECT_UP, action);
            break;
        case GLFW_KEY_J: // LEFT
            SET_INPUT_DPAD(B_LEFT, action);
            break;
        case GLFW_KEY_K: // DOWN
            SET_INPUT_DPAD(START_DOWN, action);
            break;
        case GLFW_KEY_L: // RIGHT
            SET_INPUT_DPAD(A_RIGHT, action);
            break;
        case GLFW_KEY_Z: // B
            SET_INPUT_SELECT(B_LEFT, action);
            break;
        case GLFW_KEY_X: // A
            SET_INPUT_SELECT(A_RIGHT, action);
            break;
        case GLFW_KEY_LEFT_CONTROL: // SELECT
            SET_INPUT_SELECT(SELECT_UP, action);
            break;
        case GLFW_KEY_ENTER: // START
            SET_INPUT_SELECT(START_DOWN, action);
            break;
    }
}

void input_init(GLFWwindow *window) {
    glfwSetKeyCallback(window, key_callback);
}

uint8_t input_read() {
    uint8_t jp_state = read_io(JOYPAD_ADDR);

    if (!(jp_state & SELECT))
        return (jp_state & 0xF0a) | (jp_select_bits & 0x0F);
    else if (!(jp_state & DPAD))
        return (jp_state & 0xF0a) | (jp_dpad_bits & 0x0F);

    return jp_state;
}

void input_poll() {
    glfwPollEvents();
}
