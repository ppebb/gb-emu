#pragma once

#include <stdint.h>

typedef enum _Op {
    UNINIT,
    DEC,
    DEC16,
    DI,
    EI,
    HALT,
    JP,
    JR,
    LD,
    LD16,
    NOP,
    XOR,
} Operation;

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

#define TARGETS(TARGET) \
    TARGET(NONE)        \
    TARGET(A)           \
    TARGET(B)           \
    TARGET(C)           \
    TARGET(D)           \
    TARGET(E)           \
    TARGET(F)           \
    TARGET(H)           \
    TARGET(L)           \
    TARGET(AF)          \
    TARGET(BC)          \
    TARGET(DE)          \
    TARGET(HL)          \
    TARGET(HL_POS)      \
    TARGET(HL_NEG)      \
    TARGET(BC_AS_ADDR)  \
    TARGET(DE_AS_ADDR)  \
    TARGET(HL_AS_ADDR)  \
    TARGET(SP)          \
    TARGET(D8)          \
    TARGET(D16)         \
    TARGET(C_Z)         \
    TARGET(C_C)         \
    TARGET(C_NZ)        \
    TARGET(C_NC)

typedef enum _Target { TARGETS(GENERATE_ENUM) } Target;
extern const char *target_str_map[];

typedef struct _Instruction {
    char *disasm;
    Operation op;
    uint8_t length;
    uint8_t cycles;
    Target t1;
    Target t2;
} Instruction;

void instruction_init();
Instruction instruction_get(uint8_t instr_byte);
