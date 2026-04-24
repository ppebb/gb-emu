#include "mem.h"
#include "defs.h"
#include "rom.h"
#include "timer.h"
#include <assert.h>
#include <stdbit.h>
#include <stdio.h>

// See https://gbdev.io/pandocs/Memory_Map.html

// TODO: Merge regions of memory which are unbanked to simplify r/w functions
uint8_t vram[0x2000];     /* 0x8000-0x999F */
uint8_t *ex_ram;          /* 0xA000-0xBFFF */
uint8_t wram_0[0x1000];   /* 0xC000-0xCFFF */
uint8_t wram_1[0x1000];   /* 0xD000-0xDFFF */
uint8_t echo_ram[0x1DFF]; /* 0xE000-0xFDFF */
uint8_t oam[0xA0];        /* 0xFE00-0xFE9F */
uint8_t io[0x80];         /* 0xFF00-0xFF7F */
uint8_t hram[0x80];       /* 0xFF80-0xFFFE */
uint8_t ie;               /* 0xFFFF */

// Rom memory is 0x0000-0x8000
Rom *_rom;

// Banking state
bool ram_enable = false;
uint8_t rom_bank_number = 0;

#define set_rom_bank_number_lo(lo) rom_bank_number = (rom_bank_number & (~0b00011111)) | (lo);
#define set_rom_bank_number_hi(hi) rom_bank_number = (rom_bank_number & (0b00011111)) | (hi);

void mem_init(Rom *rom) {
    _rom = rom;

    // Required initialization states
    write_io(0xFF05, 0x00);
    write_io(0xFF06, 0x00);
    write_io(0xFF07, 0x00);
    write_io(0xFF10, 0x80);
    write_io(0xFF11, 0xBF);
    write_io(0xFF12, 0xF3);
    write_io(0xFF14, 0xBF);
    write_io(0xFF16, 0x3F);
    write_io(0xFF17, 0x00);
    write_io(0xFF19, 0xBF);
    write_io(0xFF1A, 0x7F);
    write_io(0xFF1B, 0xFF);
    write_io(0xFF1C, 0x9F);
    write_io(0xFF1E, 0xBF);
    write_io(0xFF20, 0xFF);
    write_io(0xFF21, 0x00);
    write_io(0xFF22, 0x00);
    write_io(0xFF23, 0xBF);
    write_io(0xFF24, 0x77);
    write_io(0xFF25, 0xF3);
    write_io(0xFF26, 0xF1);
    write_io(0xFF40, 0x91);
    write_io(0xFF42, 0x00);
    write_io(0xFF43, 0x00);
#if DOCTOR
    write_io(0xFF44, 0x90);
#endif
    write_io(0xFF45, 0x00);
    write_io(0xFF47, 0xFC);
    write_io(0xFF48, 0xFF);
    write_io(0xFF49, 0xFF);
    write_io(0xFF4A, 0x00);
    write_io(0xFF4B, 0x00);
    ie = 0;
}

uint8_t mem_read_byte(uint16_t addr) {
    if (addr < 0x4000)
        return _rom->data[addr];

    if (addr < 0x8000)
        return _rom->data[(rom_bank_number * 0x4000) + (addr - 0x4000)];

    if (addr < 0xA000)
        return vram[addr - 0x8000];

    if (addr < 0xC000) {
        if (_rom->header.ram_banks != 0)
            return ex_ram[addr - 0xA000];
        else
            return 0;
    }

    if (addr < 0xD000)
        return wram_0[addr - 0xC000];

    if (addr < 0xE000)
        return wram_1[addr - 0xD000];

    if (addr < 0xFE00)
        return echo_ram[addr - 0xE000];

    if (addr < 0xFEA0)
        return oam[addr - 0xFE00];

    if (addr < 0xFF00) {
        /* fprintf(stderr, "Prohibited read from Unusable Ram: %04x\n", addr); */
        return 0;
    }

    if (addr < 0xFF80)
        return io[addr - 0xFF00];

    if (addr < 0xFFFF)
        return hram[addr - 0xFF80];

    if (addr == 0xFFFF)
        return ie;

    printf("Read from addr %04x\n", addr);
    return 0;
}

void mem_write_byte(uint16_t addr, uint8_t data) {
    // TODO: Unhardcode if we want to support something other than MBC1 :)

    // RAM and ROM banking for MBC1
    if (_rom->header.hw_bits & MBC1) {
        // https://gbdev.io/pandocs/MBC1.html#00001fff--ram-enable-write-only
        // Enable ram banking if 0xA is written to 0x0000-0x2000
        if (addr < 0x2000) {
            ram_enable = (data & 0xF) == 0xA;
            return;
        }

        // https://gbdev.io/pandocs/MBC1.html#20003fff--rom-bank-number-write-only
        // Swap ROM banks for 0x2000-0x4000
        if (addr < 0x4000) {
            uint8_t lo5 = data & 0b00011111;
            // If the lo5 bits are set to 0, they are interpreted as requesting
            // bank 1
            if (lo5 == 0) {
                set_rom_bank_number_lo(1);
                return;
            }

            // If we have 8 rom banks, we need only 3 bits to address all 8
            // banks. stdc_bit_width_uc tells us the number of bits we need to
            // address n (n-1, since 0 is a bank) banks, (1 << bits) - 1 will
            // give us bits 1s at the end of the bitstring.
            uint8_t mask = (1 << stdc_bit_width_uc(_rom->header.rom_banks - 1)) - 1;
            set_rom_bank_number_lo(data & mask);
            return;
        }

        if (addr < 0x6000) {
            // check rom banking mode or something
            // noop for now, see
            // https://gbdev.io/pandocs/MBC1.html#40005fff--ram-bank-number--or--upper-bits-of-rom-bank-number-write-only
        }

        if (addr < 0x8000) {
            // change rom banking mode
            // not needed on small games
        }
    }

    if (addr < 0x8000)
        return;

    if (addr < 0xA000) {
        vram[addr - 0x8000] = data;
        return;
    }

    if (addr < 0xC000) {
        if (_rom->header.ram_banks != 0)
            ex_ram[addr - 0xA000] = data;
        else
            return;
    }

    if (addr < 0xD000) {
        wram_0[addr - 0xC000] = data;
        return;
    }

    if (addr < 0xE000) {
        wram_1[addr - 0xD000] = data;
        return;
    }

    if (addr < 0xFE00) {
        echo_ram[addr - 0xE000] = data;
        mem_write_byte(addr - 0x2000, data);
        return;
    }

    if (addr < 0xFEA0) {
        oam[addr - 0xFE00] = data;
        return;
    }

    if (addr < 0xFE00) {
        fprintf(stderr, "Prohibited write to Unusable Ram: %04x\n", addr);
        return;
    }

    // Trap certain IO addresses

    if (addr == DIV_ADDR) {
        write_io(DIV_ADDR, 0);
        return;
    }

    if (addr == TMC_ADDR) {
        write_io(TMC_ADDR, data);
        timer_reset(data);
        return;
    }

    if (addr == SCLN_ADDR) {
        write_io(SCLN_ADDR, 0);
        return;
    }

    if (addr == DMA_TRANSFER_ADDR) {
        mem_dma_transfer(data);
        return;
    }

    if (addr < 0xFF80) {
        io[addr - 0xFF00] = data;

#ifndef DOCTOR
        if (io[0x02] == 0x81) {
            char c = io[0x01];
            printf("%c", c);
            io[0x02] = 0x0;
        }
#endif
        return;
    }

    if (addr < 0xFFFF) {
        hram[addr - 0xFF80] = data;
        return;
    }

    if (addr == 0xFFFF) {
        ie = data;
        return;
    }

    printf("Write to addr %04x\n", addr);
}

// Copies data from src * 0x100 to OAM
void mem_dma_transfer(uint8_t src) {
    // https://gbdev.io/pandocs/OAM_DMA_Transfer.html

    // TODO: Takes 160 cycles. Unsure if we need to use this number
    uint16_t src_addr = src << 8;
    for (size_t i = 0; i < 0xA0; i++)
        oam[i] = mem_read_byte(src_addr + i);
}

uint16_t mem_read_short(uint16_t addr) {
    uint16_t ret = mem_read_byte(addr) | mem_read_byte(addr + 1) << 8;
    /* printf("Read short %04x from %04x\n", ret, addr); */
    return ret;
}

void mem_write_short(uint16_t addr, uint16_t data) {
    /* printf("Write short %04x to %04x\n", data, addr); */
    mem_write_byte(addr, (uint8_t)data);
    mem_write_byte(addr + 1, data >> 8);
}
