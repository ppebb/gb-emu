#include "timer.h"
#include "defs.h"
#include "mem.h"

int timer_ctr = 1024;
int cur_freq = 4096;
int divider_ctr = 0;

bool is_clk_enabled(uint8_t tmc) {
    return tmc & 0b100;
}

int freq(uint8_t tmc) {
    switch (tmc & 0b11) {
        case 0b00:
            return 4096;
        case 0b01:
            return 262144;
        case 0b10:
            return 65536;
        case 0b11:
            return 16384;
    }

    __builtin_unreachable();
}

void timer_divider_step(int cycles) {
    divider_ctr += cycles;

    if (divider_ctr >= 255) {
        divider_ctr = 0;
        write_io(DIV_ADDR, read_io(DIV_ADDR) + 1);
    }
}

void timer_step(int cycles) {
    timer_divider_step(cycles);

    uint8_t tmc = read_io(TMC_ADDR);

    if (!is_clk_enabled(tmc))
        return;
}

void timer_reset(uint8_t tmc) {
    if (freq(tmc) == cur_freq)
        return;

    switch (tmc & 0b11) {
        case 0b00:
            timer_ctr = 1024;
            break;
        case 0b01:
            timer_ctr = 16;
            break;
        case 0b10:
            timer_ctr = 64;
            break;
        case 0b11:
            timer_ctr = 256;
            break;
    }
}
