#include "timer.h"
#include "defs.h"
#include "mem.h"
#include "src/cpu.h"

int timer_ctr = 0;
int divider_ctr = 0;

bool is_clk_enabled(uint8_t tmc) {
    return tmc & 0b100;
}

void timer_divider_step(int cycles) {
    divider_ctr += cycles;

    if (divider_ctr >= 64) {
        divider_ctr -= 0;
        read_io(DIV_ADDR)++;
    }
}

void timer_step(int cycles) {
    timer_divider_step(cycles);

    uint8_t tmc = read_io(TMC_ADDR);

    if (!is_clk_enabled(tmc))
        return;

    timer_ctr += cycles;

    int period = timer_get_period(tmc);

    if (timer_ctr < period)
        return;

    timer_ctr -= period;

    if (read_io(TIMA_ADDR) == 0xFF) {
        write_io(TIMA_ADDR, read_io(TMA_ADDR));
        cpu_req_interrupt(INT_TIMER);
    }
    else
        read_io(TIMA_ADDR)++;
}

void timer_reset(uint8_t tmc) {
    timer_ctr = 0;
}

int timer_get_period(uint8_t tmc) {
    switch (tmc & 0b11) {
        case 0b00:
            return 256;
        case 0b01:
            return 4;
        case 0b10:
            return 16;
        case 0b11:
            return 64;
    }

    __builtin_unreachable();
}
