#pragma once

#include "rom.h"
#include <stdint.h>

void mem_init(Rom *rom);
void mem_set_cart_bank0(uint8_t *cb0);
void mem_set_cart_bank1(uint8_t *cb1);

uint8_t mem_read_byte(uint16_t addr);
void mem_write_byte(uint16_t addr);
