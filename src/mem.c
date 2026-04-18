#include "mem.h"
#include "rom.h"
#include <stdio.h>

// See https://gbdev.io/pandocs/Memory_Map.html

uint8_t *cart_bank0;    /* 0x0000-0x3FFF */
uint8_t *cart_bank1;    /* 0x4000-0x7FFF */
uint8_t vram[0x2000];   /* 0x8000-0x999F */
uint8_t *ex_ram;        /* 0xA000-0xBFFF */
uint8_t wram_0[0x1000]; /* 0xC000-0xCFFF */
uint8_t wram_1[0x1000]; /* 0xD000-0xDFFF */
uint8_t oam[0xA0];      /* 0xFE00-0xFE9F */

void mem_init(Rom *rom) {
    cart_bank0 = rom->data;
    // TODO: Bank switching
    cart_bank1 = rom->data + 0x4000;
}

void mem_set_cart_bank0(uint8_t *cb0) {
    cart_bank0 = cb0;
}

void mem_set_cart_bank1(uint8_t *cb1) {
    cart_bank1 = cb1;
}

uint8_t mem_read_byte(uint16_t addr) {
    if (addr < 0x4000)
        return cart_bank0[addr];

    if (addr < 0x8000)
        return cart_bank1[addr - 0x4000];

    if (addr < 0xA000)
        return vram[addr - 0x8000];

    if (addr < 0xC000)
        return ex_ram[addr - 0xA000];

    if (addr < 0xD000)
        return wram_0[addr - 0xC000];

    if (addr < 0xE000)
        return wram_1[addr - 0xD000];

    if (addr < 0xFE00) {
        fprintf(stderr, "Prohibited read to Echo Ram: %04x\n", addr);
        return 0;
    }

    if (addr < 0xFEA0)
        return oam[addr - 0xFE00];

    if (addr < 0xFE00) {
        fprintf(stderr, "Prohibited read to Unusable Ram: %04x\n", addr);
        return 0;
    }

    printf("Read from addr %04x\n", addr);
    return 0;
}
