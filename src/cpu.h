#pragma once

#include "instructions.h"
#include <stdint.h>

void cpu_init(bool step_debug);
void cpu_print(uint16_t next_pc);
void cpu_step();

void cpu_dec(Instruction instr, uint8_t op1);
uint16_t cpu_jp(Instruction instr, uint16_t op2);
uint16_t cpu_jr(Instruction instr, int8_t op2);
void cpu_ld(Instruction instr, uint8_t op2);
void cpu_ld16(Instruction instr, uint16_t op2);
void cpu_xor(Instruction instr, uint8_t op1, uint8_t op2);
