#pragma once

#include <stddef.h>

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
    LCDC_BGWIN_ENABLE = 1 << 0,
    LCDC_OBJ_ENABLE = 1 << 1,
    LCDC_OBJ_SIZE = 1 << 2,
    LCDC_BG_TM_AREA = 1 << 3,
    LCDC_BGWIN_TD_AREA = 1 << 4,
    LCDC_WIN_ENABLE = 1 << 5,
    LCDC_WIN_TM_AREA = 1 << 6,
    LCDC_ENABLE = 1 << 7,
} LCDCBits;

#define lcd_get_mode() (LCDStatBits) read_io(LCD_STAT_ADDR) & 0b11
#define lcd_calc_mode(status, mode) ({ ((status) & 0xFC) | (mode); })

bool lcd_enabled();
size_t lcd_update_status(size_t scanline_counter);
