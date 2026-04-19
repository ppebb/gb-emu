#pragma once

#include "instructions.h"
#include <stdint.h>

void cpu_init(bool step_debug);
void cpu_print(uint16_t next_pc);
int cpu_step();

typedef enum _Interrupt {
    INT_VBLANK = 1 << 0,
    INT_STAT = 1 << 1,
    INT_TIMER = 1 << 2,
    INT_SERIAL = 1 << 3,
    INT_JOYPAD = 1 << 4,
} Interrupt;

void cpu_req_interrupt(Interrupt interrupt);
bool cpu_run_interrupts();

// Instructions
void cpu_dec(Instruction instr, uint8_t op1);
uint16_t cpu_jp(Instruction instr, uint16_t op2);
uint16_t cpu_jr(Instruction instr, int8_t op2);
void cpu_ld(Instruction instr, uint8_t op2);
void cpu_ld16(Instruction instr, uint16_t op2);
void cpu_xor(Instruction instr, uint8_t op1, uint8_t op2);
