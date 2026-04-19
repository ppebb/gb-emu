#pragma once

#include "rom.h"
#include <stdint.h>

extern uint8_t io[0x80];
extern uint8_t ie;

void mem_init(Rom *rom);
void mem_set_cart_bank0(uint8_t *cb0);
void mem_set_cart_bank1(uint8_t *cb1);

uint8_t mem_read_byte(uint16_t addr);
void mem_write_byte(uint16_t addr, uint8_t data);

#define read_io(addr) io[addr - 0xFF00]
#define write_io(addr, val) io[addr - 0xFF00] = val

void mem_dma_transfer(uint8_t src);
