#pragma once

#include "instructions.h"
#include <stdint.h>

extern bool stop;

void cpu_init(bool step_debug);
void cpu_print(uint16_t next_pc, uint8_t cycles);
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

typedef struct _JumpResult {
    bool taken;
    uint16_t pc;
} JumpResult;

bool cpu_cond(Target t);

// Instructions
void cpu_and(Instruction instr, uint8_t op1, uint8_t op2);
void cpu_adc(Instruction instr, uint8_t op1, uint8_t op2);
void cpu_add(Instruction instr, uint8_t op1, uint8_t op2);
void cpu_add16(Instruction instr, uint16_t op1, uint16_t op2);
JumpResult cpu_call(Instruction instr, uint16_t op2);
void cpu_ccf();
void cpu_cp(Instruction instr, uint8_t op1, uint8_t op2);
void cpu_cpl();
void cpu_daa();
void cpu_dec(Instruction instr, uint8_t op1);
void cpu_dec16(Instruction instr, uint16_t op1);
void cpu_inc(Instruction instr, uint8_t op1);
void cpu_inc16(Instruction instr, uint16_t op1);
JumpResult cpu_jp(Instruction instr, uint16_t op2);
JumpResult cpu_jr(Instruction instr, int8_t op2);
void cpu_ld(Instruction instr, uint16_t op1, uint8_t op2);
void cpu_ld16(Instruction instr, uint16_t op2);
void cpu_or(Instruction instr, uint8_t op1, uint8_t op2);
void cpu_pop(Instruction instr);
void cpu_push(Instruction instr, uint16_t op1);
JumpResult cpu_ret(Instruction instr);
JumpResult cpu_reti(Instruction instr);
void cpu_rlc(Instruction instr, uint8_t op);
void cpu_rl(Instruction instr, uint8_t op);
void cpu_rrc(Instruction instr, uint8_t op);
void cpu_rr(Instruction instr, uint8_t op);
JumpResult cpu_rst(Instruction instr);
void cpu_sbc(Instruction instr, uint8_t op1, uint8_t op2);
void cpu_scf();
void cpu_sub(Instruction instr, uint8_t op1, uint8_t op2);
void cpu_xor(Instruction instr, uint8_t op1, uint8_t op2);

// 16-bit opcodes
void cpu_bit(Instruction instr, uint8_t op);
void cpu_res(Instruction instr, uint8_t op);
void cpu_set(Instruction instr, uint8_t op);
void cpu_sla(Instruction instr, uint8_t op);
void cpu_sra(Instruction instr, uint8_t op);
void cpu_srl(Instruction instr, uint8_t op);
void cpu_swap(Instruction instr, uint8_t op);
