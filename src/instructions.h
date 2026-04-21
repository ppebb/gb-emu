#pragma once

#include <stdint.h>

typedef enum _Op {
    UNINIT,
    ADC,
    ADD,
    ADD16,
    AND,
    CALL,
    CCF,
    CP,
    CPL,
    DAA,
    DEC,
    DEC16,
    DI,
    EI,
    HALT,
    INC,
    INC16,
    JP,
    JR,
    LD,
    LD16,
    NOP,
    OR,
    POP,
    PUSH,
    RET,
    RETI,
    RLC,
    RLCA,
    RL,
    RLA,
    RRC,
    RRCA,
    RR,
    RRA,
    RST,
    SCF,
    STOP,
    SBC,
    SUB,
    XOR,

    // 16 bit ops (prefix 0xCB)
    BIT,
    RES,
    SET,
    SLA,
    SRA,
    SRL,
    SWAP,

    UNDEFINED,
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
    TARGET(C_AS_ADDR)   \
    TARGET(BC_AS_ADDR)  \
    TARGET(DE_AS_ADDR)  \
    TARGET(HL_AS_ADDR)  \
    TARGET(SP)          \
    TARGET(D8)          \
    TARGET(SP_PLUS_D8)  \
    TARGET(D8_AS_ADDR)  \
    TARGET(D16)         \
    TARGET(D16_AS_ADDR) \
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
    bool vlc; // variable length cycles
    union {
        uint8_t cycles;
        struct JC {
            uint8_t taken_cycles;
            uint8_t not_taken_cycles;
        } jump_cycles;
    };
    Target t1;
    Target t2;
} Instruction;

void instruction_init();
Instruction instruction_get(uint16_t instr_byte);
