#include "src/instructions.h"
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
    else
        pattern = "%s (%s, %s)";

    const char *t1_str = target_str_map[t1];
    const char *t2_str = target_str_map[t2];

    int n = snprintf(NULL, 0, pattern, op, t1_str, t2_str) + 1;
    char *ret = malloc(n);
    snprintf(ret, n, pattern, op, t1_str, t2_str);

    return ret;
}

#define si(_op, _length, _cycles, _t1, _t2)                                                                          \
    (Instruction) {                                                                                                  \
        .disasm = fmt_disasm(#_op, _t1, _t2), .op = _op, .length = _length, .cycles = _cycles, .t1 = _t1, .t2 = _t2, \
    }

void instruction_init() {
    // operation, len, cycles, op1, op2

    // dec
    instructions[0x05] = si(DEC, 1, 1, B, NONE);
    instructions[0x15] = si(DEC, 1, 1, D, NONE);
    instructions[0x25] = si(DEC, 1, 1, H, NONE);
    instructions[0x35] = si(DEC, 1, 3, HL_AS_ADDR, NONE);

    instructions[0x0D] = si(DEC, 1, 1, C, NONE);
    instructions[0x1D] = si(DEC, 1, 1, E, NONE);
    instructions[0x2D] = si(DEC, 1, 1, L, NONE);
    instructions[0x3D] = si(DEC, 1, 1, A, NONE);

    // jp
    instructions[0xC3] = si(JP, 3, 4, NONE, D16);

    // jr
    // WARNING: Inconsistent number of cycles. Currently set to the upper bound
    instructions[0x20] = si(JR, 2, 3, C_NZ, D8);
    instructions[0x30] = si(JR, 2, 3, C_NC, D8);

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
    instructions[0x36] = si(LD, 2, 2, HL_AS_ADDR, D8);

    instructions[0x0A] = si(LD, 1, 2, A, BC_AS_ADDR);
    instructions[0x1A] = si(LD, 1, 2, A, DE_AS_ADDR);
    instructions[0x2A] = si(LD, 1, 2, A, HL_POS);
    instructions[0x3A] = si(LD, 1, 2, A, HL_NEG);

    instructions[0x0E] = si(LD, 2, 2, C, D8);
    instructions[0x1E] = si(LD, 2, 2, E, D8);
    instructions[0x2E] = si(LD, 2, 2, L, D8);
    instructions[0x3E] = si(LD, 2, 2, A, D8);

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

    // xor
    Target xor_targets[8] = { B, C, D, E, H, L, HL_AS_ADDR, A };
    for (size_t i = 0; i < 8; i++) {
        Target target = xor_targets[i];
        instructions[0xA8 + i] = si(XOR, 1, target == HL_AS_ADDR ? 2 : 1, A, target);
    }
}

Instruction instruction_get(uint8_t instr_byte) {
    return instructions[instr_byte];
}
