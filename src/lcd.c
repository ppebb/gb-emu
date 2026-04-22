#include "lcd.h"
#include "cpu.h"
#include "defs.h"
#include "mem.h"
#include <stddef.h>

// Cycle counts to set specific modes
#define SEARCHING_BOUNDS (SCLN_CYCLES - 80)
#define TRANSFER_BOUNDS (SEARCHING_BOUNDS - 172)

bool lcd_enabled() {
    return read_io(LCD_CTRL_ADDR) & LCDC_ENABLE;
}

size_t lcd_update_status(size_t scanline_counter) {
    uint8_t status = read_io(LCD_STAT_ADDR);

    if (!lcd_enabled()) {
        // If the lcd is off we're in mode 1, scanline is always 0.
        scanline_counter = SCLN_CYCLES;
        write_io(SCLN_ADDR, 0);
        status &= 0xFC | 1;
        write_io(LCD_STAT_ADDR, status);

        return scanline_counter;
    }

    uint8_t cur_line = read_io(SCLN_ADDR);
    LCDStatBits cur_mode = lcd_get_mode();

    LCDStatBits next_mode = 0;
    bool request_interrupt = false;

    // vblank
    if (cur_line >= SCLN_VISIBLE_CNT) {
        next_mode = LCDS_V_BLANK;
        status = lcd_calc_mode(status, LCDS_V_BLANK);
        request_interrupt = status & LCDS_V_BLANK_INT;
    }
    // searching
    else if (scanline_counter >= SEARCHING_BOUNDS) {
        next_mode = LCDS_SEARCHING;
        status = lcd_calc_mode(status, LCDS_SEARCHING);
        request_interrupt = status & LCDS_SEARCHING_INT;
    }
    // transfer
    else if (scanline_counter >= TRANSFER_BOUNDS) {
        next_mode = LCDS_TRANSFER;
        status = lcd_calc_mode(status, LCDS_SEARCHING);
        // No searching interrupt
    }
    // hblank
    else {
        next_mode = LCDS_H_BLANK;
        status = lcd_calc_mode(status, LCDS_H_BLANK);
        request_interrupt = status & LCDS_H_BLANK_INT;
    }

    if (request_interrupt && next_mode != cur_mode)
        cpu_req_interrupt(INT_STAT);

    // set coincidence flag
    if (cur_line == read_io(LCD_LYC_ADDR)) {
        status |= LCDS_LYC_LY_EQ;

        if (status & LCDS_LYC_LY_EQ_INT)
            cpu_req_interrupt(INT_STAT);
    }
    else
        status &= !LCDS_LYC_LY_EQ;

    write_io(LCD_STAT_ADDR, status);

    return scanline_counter;
}
