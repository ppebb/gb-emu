#include "ppu.h"
#include "defs.h"
#include "lcd.h"
#include "mem.h"
#include "src/cpu.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

void ppu_draw_scanline();
void ppu_draw_bg(uint8_t ctrl);
void ppu_draw_sprites();

Color frame_buffer[WIDTH * HEIGHT];
size_t scanline_counter = 0;

void ppu_step(int cycles) {
    // scanline_counter may be reset if the lcd is disabled
    scanline_counter = lcd_update_status(scanline_counter);

    if (!lcd_enabled())
        return;

    scanline_counter += cycles;

    if (scanline_counter < SCLN_CYCLES)
        return;

    scanline_counter -= SCLN_CYCLES;

    uint8_t cur_line = read_io(SCLN_ADDR);

    if (cur_line < SCLN_VISIBLE_CNT) {
        ppu_draw_scanline();

        read_io(SCLN_ADDR)++;
        return;
    }

    if (cur_line == SCLN_VISIBLE_CNT) {
        cpu_req_interrupt(INT_VBLANK);

        read_io(SCLN_ADDR)++;
        return;
    }

    if (cur_line >= SCLN_CNT) {
        write_io(SCLN_ADDR, 0);

        read_io(SCLN_ADDR)++;
        return;
    }

    if (cur_line >= SCLN_VISIBLE_CNT) {
        read_io(SCLN_ADDR)++;
        return;
    }

    read_io(SCLN_ADDR)++;
}

void ppu_draw_scanline() {
    uint8_t ctrl = read_io(LCD_CTRL_ADDR);

    if (ctrl & LCDC_BGWIN_ENABLE)
        ppu_draw_bg(ctrl);

    if (ctrl & LCDC_OBJ_ENABLE)
        ppu_draw_sprites();
}

uint16_t locate_tile(int8_t tile_id, uint8_t ctrl) {
    // LCDC_BG_TM_AREA controls whether tiles begin at 0x8000 or 0x8800
    bool is_area_a = ctrl & LCDC_BGWIN_TD_AREA;
    size_t region_start = is_area_a ? TM_AREA_A : TM_AREA_B;
    size_t offset = !is_area_a * TM_AREA_B_OFFSET;

    return region_start + (tile_id + offset) * TILE_SIZE;
}

Color colors[4] = { COLOR_WHITE, COLOR_LIGHT_GRAY, COLOR_DARK_GRAY, COLOR_BLACK };
uint16_t palette_addrs[3] = { BGP_ADDR, OBP0_ADDR, OBP1_ADDR };

Color get_color(uint8_t color_idx, uint8_t palette_idx) {
    uint8_t palette = mem_read_byte(palette_addrs[palette_idx]);
    int hi = color_idx * 2 + 1;
    int lo = color_idx * 2;

    int color = (palette & (1 << hi)) << 1;
    color |= (palette & (1 << lo));

    return colors[color];
}

void ppu_draw_bg(uint8_t ctrl) {
    uint16_t bg_mem = 0;
    bool window = false;

    bool tile_unsigned = !(ctrl & LCDC_BG_TM_AREA);

    uint8_t scy = read_io(SCY_ADDR);
    uint8_t scx = read_io(SCX_ADDR);
    uint8_t winy = read_io(WINY_ADDR);
    int16_t winx = read_io(WINX_ADDR) - 7;
    uint8_t scln = read_io(SCLN_ADDR);

    // draw the window if it is enabled and we are drawing within the window
    // position
    window = (ctrl & LCDC_WIN_ENABLE) && winy <= scln;

    if (window)
        bg_mem = (ctrl & LCDC_WIN_TM_AREA) ? BGWIN_AREA_A : BGWIN_AREA_B;
    else
        bg_mem = (ctrl & LCDC_BG_TM_AREA) ? BGWIN_AREA_A : BGWIN_AREA_B;

    uint8_t y_pos = 0;

    if (window)
        y_pos = scln - winy;
    else
        y_pos = scy + scln;

    uint16_t tile_row = ((uint8_t)(y_pos / 8)) * 32;

    for (size_t i = 0; i < WIDTH; i++) {
        uint8_t x_pos = i + scx;

        if (window)
            x_pos = i - winx;

        uint16_t tile_col = x_pos / 8;
        int16_t tile_id;
        uint16_t tile_addr = bg_mem + tile_row + tile_col;

        if (tile_unsigned)
            tile_id = (uint8_t)mem_read_byte(tile_addr);
        else
            tile_id = (int8_t)mem_read_byte(tile_addr);

        uint16_t tile_loc = locate_tile(tile_id, ctrl);

        uint8_t line = y_pos % 8;
        line *= 2;
        uint8_t tile_data_0 = mem_read_byte(tile_loc + line);
        uint8_t tile_data_1 = mem_read_byte(tile_loc + line + 1);

        int color_bit = x_pos % 8;
        color_bit -= 7;
        color_bit *= -1;

        // Color bits are stored in the color_bit'th place of tile_data_1 and
        // tile_data_0 as if layered on top of each other
        uint8_t color_idx = (tile_data_1 >> color_bit & 1) << 1;
        color_idx |= (tile_data_0 >> color_bit & 1);

        Color color = get_color(color_idx, 0);

        assert(scln < HEIGHT && i < WIDTH);

        frame_buffer[scln * WIDTH + i] = color;
    }
}

void ppu_draw_sprites() {
}
