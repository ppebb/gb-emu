#include "instructions.h"
#include <stdio.h>
#include <stdlib.h>

const char *target_str_map[] = { TARGETS(GENERATE_STRING) };

Instruction instructions[512] = { 0 };

char *fmt_disasm(char *op, Target t1, Target t2) {
    char *pattern;

    if (t1 == NONE && t2 == NONE)
        pattern = "%s";
    else if (t2 == NONE)
        pattern = "%s (%s)";
    else if (t1 == NONE)
        pattern = "%s (%s)";
    else
        pattern = "%s (%s, %s)";

    const char *t1_str = target_str_map[t1];
    const char *t2_str = target_str_map[t2];

    char *ret;

    if (t1 == NONE && t2 != NONE) {
        int n = snprintf(NULL, 0, pattern, op, t2_str) + 1;
        ret = malloc(n);
        snprintf(ret, n, pattern, op, t2_str);
    }
    else {
        int n = snprintf(NULL, 0, pattern, op, t1_str, t2_str) + 1;
        ret = malloc(n);
        snprintf(ret, n, pattern, op, t1_str, t2_str);
    }

    return ret;
}

#define si(_op, _length, _cycles, _t1, _t2)                                                                  \
    (Instruction) {                                                                                          \
        .disasm = fmt_disasm(#_op, _t1, _t2), .op = _op, .length = _length, .vlc = false, .cycles = _cycles, \
        .t1 = _t1, .t2 = _t2,                                                                                \
    }

#define sij(_op, _length, t_cycles, nt_cycles, _t1, _t2)                                 \
    (Instruction) {                                                                      \
        .disasm = fmt_disasm(#_op, _t1, _t2), .op = _op, .length = _length, .vlc = true, \
        .jump_cycles = { t_cycles, nt_cycles }, .t1 = _t1, .t2 = _t2,                    \
    }

Target consec_targets[8] = { B, C, D, E, H, L, HL_AS_ADDR, A };

#define def_consec_ops(base_addr, op)                                                     \
    for (size_t i = 0; i < 8; i++) {                                                      \
        Target target = consec_targets[i];                                                \
        instructions[base_addr + i] = si(op, 1, target == HL_AS_ADDR ? 2 : 1, A, target); \
    }

#define def_consec_ops_16_none(base_addr, op)                                                        \
    for (size_t i = 0; i < 8; i++) {                                                                 \
        Target target = consec_targets[i];                                                           \
        instructions[base_addr + 0x100 + i] = si(op, 2, target == HL_AS_ADDR ? 4 : 2, target, NONE); \
    }

#define def_consec_ops_16_n(base_addr, op, n)                                                     \
    for (size_t i = 0; i < 8; i++) {                                                              \
        Target target = consec_targets[i];                                                        \
        instructions[base_addr + 0x100 + i] = si(op, 2, target == HL_AS_ADDR ? 4 : 2, target, n); \
    }

#define def_consec_ops_16_bit(base_addr, op, n)                                                   \
    for (size_t i = 0; i < 8; i++) {                                                              \
        Target target = consec_targets[i];                                                        \
        instructions[base_addr + 0x100 + i] = si(op, 2, target == HL_AS_ADDR ? 3 : 2, target, n); \
    }

#define undefined()            \
    (Instruction) {            \
        "UNDEFINED", UNDEFINED \
    }

void instruction_init() {
    // operation, len, cycles, op1, op2

    // instructions vaguely ordered alphabetically by function
    // https://izik1.github.io/gbops/index.html

    // undefined
    instructions[0xCB] = undefined();
    instructions[0xD3] = undefined();
    instructions[0xDB] = undefined();
    instructions[0xDD] = undefined();
    instructions[0xE3] = undefined();
    instructions[0xE4] = undefined();
    instructions[0xEB] = undefined();
    instructions[0xEC] = undefined();
    instructions[0xED] = undefined();
    instructions[0xF4] = undefined();
    instructions[0xFC] = undefined();
    instructions[0xFD] = undefined();

    // and
    def_consec_ops(0xA0, AND);
    instructions[0xE6] = si(AND, 2, 2, A, D8);

    // adc
    def_consec_ops(0x88, ADC);
    instructions[0xCE] = si(ADC, 2, 2, A, D8);

    // add
    instructions[0x09] = si(ADD16, 1, 2, HL, BC);
    instructions[0x19] = si(ADD16, 1, 2, HL, DE);
    instructions[0x29] = si(ADD16, 1, 2, HL, HL);
    instructions[0x39] = si(ADD16, 1, 2, HL, SP);
    instructions[0xE8] = si(ADD_SPD8, 2, 4, SP, D8);

    def_consec_ops(0x80, ADD);
    instructions[0xC6] = si(ADD, 2, 2, A, D8);

    // call
    instructions[0xC4] = sij(CALL, 3, 6, 3, C_NZ, D16);
    instructions[0xCC] = sij(CALL, 3, 6, 3, C_Z, D16);
    instructions[0xCD] = si(CALL, 3, 6, NONE, D16);
    instructions[0xD4] = sij(CALL, 3, 6, 3, C_NC, D16);
    instructions[0xDC] = sij(CALL, 3, 6, 3, C_C, D16);

    // ccf
    instructions[0x3F] = si(CCF, 1, 1, NONE, NONE);

    // cp
    def_consec_ops(0xB8, CP);
    instructions[0xFE] = si(CP, 2, 2, A, D8);

    // cpl
    instructions[0x2F] = si(CPL, 1, 1, NONE, NONE);

    // daa
    instructions[0x27] = si(DAA, 1, 1, NONE, NONE);

    // dec
    instructions[0x05] = si(DEC, 1, 1, B, NONE);
    instructions[0x15] = si(DEC, 1, 1, D, NONE);
    instructions[0x25] = si(DEC, 1, 1, H, NONE);
    instructions[0x35] = si(DEC, 1, 3, HL_AS_ADDR, NONE);

    instructions[0x0B] = si(DEC16, 1, 2, BC, NONE);
    instructions[0x1B] = si(DEC16, 1, 2, DE, NONE);
    instructions[0x2B] = si(DEC16, 1, 2, HL, NONE);
    instructions[0x3B] = si(DEC16, 1, 2, SP, NONE);

    instructions[0x0D] = si(DEC, 1, 1, C, NONE);
    instructions[0x1D] = si(DEC, 1, 1, E, NONE);
    instructions[0x2D] = si(DEC, 1, 1, L, NONE);
    instructions[0x3D] = si(DEC, 1, 1, A, NONE);

    // di, ei
    instructions[0xF3] = si(DI, 1, 1, NONE, NONE);
    instructions[0xFB] = si(EI, 1, 1, NONE, NONE);

    // halt
    instructions[0x76] = si(HALT, 1, 1, NONE, NONE);

    // inc
    instructions[0x03] = si(INC16, 1, 2, BC, NONE);
    instructions[0x13] = si(INC16, 1, 2, DE, NONE);
    instructions[0x23] = si(INC16, 1, 2, HL, NONE);
    instructions[0x33] = si(INC16, 1, 2, SP, NONE);

    instructions[0x04] = si(INC, 1, 1, B, NONE);
    instructions[0x14] = si(INC, 1, 1, D, NONE);
    instructions[0x24] = si(INC, 1, 1, H, NONE);
    instructions[0x34] = si(INC, 1, 3, HL_AS_ADDR, NONE);

    instructions[0x0C] = si(INC, 1, 1, C, NONE);
    instructions[0x1C] = si(INC, 1, 1, E, NONE);
    instructions[0x2C] = si(INC, 1, 1, L, NONE);
    instructions[0x3C] = si(INC, 1, 1, A, NONE);

    // jp
    instructions[0xC2] = sij(JP, 3, 4, 3, C_NZ, D16);
    instructions[0xC3] = si(JP, 3, 4, NONE, D16);
    instructions[0xCA] = sij(JP, 3, 4, 3, C_Z, D16);
    instructions[0xD2] = sij(JP, 3, 4, 3, C_NC, D16);
    instructions[0xDA] = sij(JP, 3, 4, 3, C_C, D16);
    instructions[0xE9] = si(JP, 3, 1, NONE, HL);

    // jr
    instructions[0x18] = si(JR, 2, 3, NONE, D8);
    instructions[0x20] = sij(JR, 2, 3, 2, C_NZ, D8);
    instructions[0x28] = sij(JR, 2, 3, 2, C_Z, D8);
    instructions[0x30] = sij(JR, 2, 3, 2, C_NC, D8);
    instructions[0x38] = sij(JR, 2, 3, 2, C_C, D8);

    // ld
    instructions[0x01] = si(LD16, 3, 3, BC, D16);
    instructions[0x11] = si(LD16, 3, 3, DE, D16);
    instructions[0x21] = si(LD16, 3, 3, HL, D16);
    instructions[0x31] = si(LD16, 3, 3, SP, D16);

    instructions[0x02] = si(LD, 1, 2, BC_AS_ADDR, A);
    instructions[0x12] = si(LD, 1, 2, DE_AS_ADDR, A);
    instructions[0x22] = si(LD, 1, 2, HL_POS, A);
    instructions[0x32] = si(LD, 1, 2, HL_NEG, A);

    instructions[0x06] = si(LD, 2, 2, B, D8);
    instructions[0x16] = si(LD, 2, 2, D, D8);
    instructions[0x26] = si(LD, 2, 2, H, D8);
    instructions[0x36] = si(LD, 2, 3, HL_AS_ADDR, D8);

    instructions[0x08] = si(LD16, 3, 5, D16_AS_ADDR, SP);

    instructions[0x0A] = si(LD, 1, 2, A, BC_AS_ADDR);
    instructions[0x1A] = si(LD, 1, 2, A, DE_AS_ADDR);
    instructions[0x2A] = si(LD, 1, 2, A, HL_POS);
    instructions[0x3A] = si(LD, 1, 2, A, HL_NEG);

    instructions[0x0E] = si(LD, 2, 2, C, D8);
    instructions[0x1E] = si(LD, 2, 2, E, D8);
    instructions[0x2E] = si(LD, 2, 2, L, D8);
    instructions[0x3E] = si(LD, 2, 2, A, D8);

    instructions[0xE0] = si(LD, 2, 3, D8_AS_ADDR, A);
    instructions[0xF0] = si(LD, 2, 3, A, D8_AS_ADDR);
    instructions[0xE2] = si(LD, 1, 2, C_AS_ADDR, A);
    instructions[0xF2] = si(LD, 1, 2, A, C_AS_ADDR);
    instructions[0xEA] = si(LD, 3, 4, D16_AS_ADDR, A);
    instructions[0xFA] = si(LD, 3, 4, A, D16_AS_ADDR);

    instructions[0xF8] = si(LD_SP_PLUS_D8, 2, 3, SP, D8);
    instructions[0xF9] = si(LD16, 1, 2, SP, HL);

    Target ld_targets[8] = { B, C, D, E, H, L, HL_AS_ADDR, A };
    for (size_t i = 0; i < 8; i++) {
        for (size_t j = 0; j < 8; j++) {
            Target t1 = ld_targets[i];
            Target t2 = ld_targets[j];

            if (t1 == HL_AS_ADDR && t2 == HL_AS_ADDR)
                continue;

            instructions[0x40 + (i * 8) + j] = si(LD, 1, t1 == HL_AS_ADDR || t2 == HL_AS_ADDR ? 2 : 1, t1, t2);
        }
    }

    // nop
    instructions[0x0] = si(NOP, 1, 1, NONE, NONE);

    // or
    def_consec_ops(0xB0, OR);
    instructions[0xF6] = si(OR, 2, 2, A, D8);

    // pop
    instructions[0xC1] = si(POP, 1, 3, BC, NONE);
    instructions[0xD1] = si(POP, 1, 3, DE, NONE);
    instructions[0xE1] = si(POP, 1, 3, HL, NONE);
    instructions[0xF1] = si(POP, 1, 3, AF, NONE);

    // push
    instructions[0xC5] = si(PUSH, 1, 4, BC, NONE);
    instructions[0xD5] = si(PUSH, 1, 4, DE, NONE);
    instructions[0xE5] = si(PUSH, 1, 4, HL, NONE);
    instructions[0xF5] = si(PUSH, 1, 4, AF, NONE);

    // ret
    // WARN: This also has the upper bound for cycle count.
    instructions[0xC0] = sij(RET, 1, 5, 2, C_NZ, NONE);
    instructions[0xC8] = sij(RET, 1, 5, 2, C_Z, NONE);
    instructions[0xC9] = si(RET, 1, 4, NONE, NONE);
    instructions[0xD0] = sij(RET, 1, 5, 2, C_NC, NONE);
    instructions[0xD8] = sij(RET, 1, 5, 2, C_C, NONE);

    // reti
    instructions[0xD9] = si(RETI, 1, 4, NONE, NONE);

    // rlc
    instructions[0x07] = si(RLCA, 1, 1, A, NONE);

    // rl
    instructions[0x17] = si(RLA, 1, 1, A, NONE);

    // rrc
    instructions[0x0F] = si(RRCA, 1, 1, A, NONE);

    // rr
    instructions[0x1F] = si(RRA, 1, 1, A, NONE);

    // rst
    instructions[0xC7] = si(RST, 1, 4, 0, NONE);
    instructions[0xCF] = si(RST, 1, 4, 1, NONE);
    instructions[0xD7] = si(RST, 1, 4, 2, NONE);
    instructions[0xDF] = si(RST, 1, 4, 3, NONE);
    instructions[0xE7] = si(RST, 1, 4, 4, NONE);
    instructions[0xEF] = si(RST, 1, 4, 5, NONE);
    instructions[0xF7] = si(RST, 1, 4, 6, NONE);
    instructions[0xFF] = si(RST, 1, 4, 7, NONE);

    // scf
    instructions[0x37] = si(SCF, 1, 1, NONE, NONE);

    // stop
    instructions[0x10] = si(STOP, 2, 1, NONE, NONE);

    // sub
    def_consec_ops(0x90, SUB);
    instructions[0xD6] = si(SUB, 2, 2, A, D8);

    // sbc
    def_consec_ops(0x98, SBC);
    instructions[0xDE] = si(SBC, 2, 2, A, D8);

    // xor
    def_consec_ops(0xA8, XOR);
    instructions[0xEE] = si(XOR, 2, 2, A, D8);

    // 16-bit ops

    def_consec_ops_16_none(0x00, RLC);
    def_consec_ops_16_none(0x08, RRC);
    def_consec_ops_16_none(0x10, RL);
    def_consec_ops_16_none(0x18, RR);
    def_consec_ops_16_none(0x18, RR);
    def_consec_ops_16_none(0x20, SLA);
    def_consec_ops_16_none(0x28, SRA);
    def_consec_ops_16_none(0x30, SWAP);
    def_consec_ops_16_none(0x38, SRL);
    for (size_t j = 0; j < 8; j++) {
        def_consec_ops_16_bit(0x40 + j * 8, BIT, j);
        def_consec_ops_16_n(0x80 + j * 8, RES, j);
        def_consec_ops_16_n(0xC0 + j * 8, SET, j);
    }
}

Instruction instruction_get(uint16_t instr_byte) {
    return instructions[instr_byte];
}
