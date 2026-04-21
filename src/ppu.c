#include "ppu.h"
#include "defs.h"
#include "mem.h"
#include "src/cpu.h"
#include <stddef.h>

Color frame_buffer[WIDTH * HEIGHT];

typedef enum _LCDStatBits {
    // Take up first two bits
    LCDS_H_BLANK = 0,
    LCDS_V_BLANK = 1,
    LCDS_SEARCHING = 2,
    LCDS_TRANSFER = 3,
    // Remaining indiv bits
    LCDS_LYC_LY_EQ = 1 << 2,
    LCDS_H_BLANK_INT = 1 << 3,
    LCDS_V_BLANK_INT = 1 << 4,
    LCDS_SEARCHING_INT = 1 << 5,
    LCDS_LYC_LY_EQ_INT = 1 << 6,
} LCDStatBits;

typedef enum _LCDCBits {
    LCDC_BGWIN_PRIO = 1 << 0,
    LCDC_OBJ = 1 << 1,
    LCDC_OBJ_SIZE = 1 << 2,
    LCDC_BG_TM_AREA = 1 << 3,
    LCDC_BGWIN_TD_AREA = 1 << 4,
    LCDC_WIN_ENABLE = 1 << 5,
    LCDC_WIN_TMA = 1 << 6,
    LCDC_ENABLE = 1 << 7,
} LCDCBits;

#define lcd_get_mode() (LCDStatBits) read_io(LCD_STAT_ADDR) & 0b11
#define lcd_calc_mode(status, mode) ({ ((status) & 0xFC) | (mode); })

// Cycle counts to set specific modes
#define SEARCHING_BOUNDS (SCLN_CYCLES - 80)
#define TRANSFER_BOUNDS (SEARCHING_BOUNDS - 172)

size_t scanline_counter = 0;

bool lcd_enabled() {
    return read_io(LCD_CTRL_ADDR) & LCDC_ENABLE;
}

void lcd_update_status() {
    uint8_t status = read_io(LCD_STAT_ADDR);

    if (!lcd_enabled()) {
        // If the lcd is off we're in mode 1, scanline is always 0.
        scanline_counter = SCLN_CYCLES;
        write_io(SCLN_ADDR, 0);
        status &= 0xFC | 1;
        write_io(LCD_STAT_ADDR, status);

        return;
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
}

void ppu_step(int cycles) {
    lcd_update_status();

    if (!lcd_enabled())
        return;

    scanline_counter += cycles;

    if (scanline_counter < SCLN_CYCLES)
        return;

    scanline_counter -= SCLN_CYCLES;

    read_io(SCLN_ADDR)++;
    uint8_t cur_line = read_io(SCLN_ADDR);

    if (cur_line == 144) {
        cpu_req_interrupt(INT_VBLANK);
        return;
    }

    if (cur_line >= SCLN_CNT) {
        write_io(SCLN_ADDR, 0);
        return;
    }

    if (cur_line >= SCLN_VISIBLE_CNT)
        return;

    // draw scln
}
