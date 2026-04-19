#include "cpu.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>

#define F_Z (1 << 7)
#define F_N (1 << 6)
#define F_H (1 << 5)
#define F_C (1 << 4)

#define F_ISZERO (regs.f & F_Z)
#define F_ISNEGATIVE (regs.f & F_N)
#define F_ISCARRY (regs.f & F_C)
#define F_ISHALFCARRY (regs.f & F_H)

#define F_ISSET(x) ((regs.f & (x)) != 0)
#define F_SET(x) (regs.f |= (x))
#define F_CLEAR(x) (regs.f &= ~(x))

typedef struct _Regs {
    union {
        struct {
            uint8_t f;
            uint8_t a;
        };
        uint16_t af;
    };

    union {
        struct {
            uint8_t c;
            uint8_t b;
        };
        uint16_t bc;
    };

    union {
        struct {
            uint8_t e;
            uint8_t d;
        };
        uint16_t de;
    };

    union {
        struct {
            uint8_t l;
            uint8_t h;
        };
        uint16_t hl;
    };

    uint16_t sp;
    uint16_t pc;

    bool halt;
} Regs;

Regs regs = (Regs){
    .af = 0,
    .bc = 0,
    .de = 0,
    .hl = 0,
    .pc = 0x100,
    .sp = 0,
};

bool _step_debug;
uint8_t current_instr_byte;
Instruction current_instr;

void cpu_init(bool step_debug) {
    _step_debug = step_debug;
}

int get_op_8(Target target) {
    switch (target) {
        case A:
            return regs.a;
        case B:
            return regs.b;
        case C:
            return regs.c;
        case D:
            return regs.d;
        case E:
            return regs.e;
        case F:
            return regs.f;
        case H:
            return regs.h;
        case L:
            return regs.l;
        case HL_AS_ADDR:
            return mem_read_byte(regs.hl);
        case D8:
            return mem_read_byte(regs.pc + 1);
        default:
            /* printf("Unknown target get_op_8: %s\n", target_str_map[target]); */
            return 0;
    }
}

int get_op_16(Target target) {
    switch (target) {
        case D16:
            return (uint16_t)mem_read_byte(regs.pc + 2) << 8 | mem_read_byte(regs.pc + 1);
        case HL:
            return (uint16_t)regs.l << 8 | regs.h;
        default:
            /* printf("Unknown target get_op_16: %s\n", target_str_map[target]); */
            return 0;
    }
}

void set_reg_8(Target target, uint8_t data) {
    switch (target) {
        case A:
            regs.a = data;
            break;
        case B:
            regs.b = data;
            break;
        case C:
            regs.c = data;
            break;
        case D:
            regs.d = data;
            break;
        case E:
            regs.e = data;
            break;
        case F:
            regs.f = data;
            break;
        case H:
            regs.h = data;
            break;
        case L:
            regs.l = data;
            break;
        default:
            fprintf(stderr, "Illegal write 8 to target %s\n", target_str_map[target]);

            cpu_print(regs.pc);
            exit(1);
    }
}

void set_reg_16(Target target, uint16_t data) {
    switch (target) {
        case AF:
            regs.af = data;
            break;
        case BC:
            regs.bc = data;
            break;
        case DE:
            regs.de = data;
            break;
        case HL:
            regs.hl = data;
            break;
        case SP:
            regs.sp = data;
            break;
        default:
            fprintf(stderr, "Illegal write 16 to target %s\n", target_str_map[target]);
            cpu_print(regs.pc);
            exit(1);
    }
}

void cpu_print(uint16_t next_pc) {
    if (current_instr.disasm == NULL)
        fprintf(stderr, "Unknown instruction at PC 0x%04x: 0x%02x\n", regs.pc, current_instr_byte);
    else
        printf("Instr 0x%04x: 0x%02x, %s\n", regs.pc, current_instr_byte, current_instr.disasm);

    printf("Z N H C\n");
    printf("%b %b %b %b\n", F_ISSET(F_Z), F_ISSET(F_N), F_ISSET(F_H), F_ISSET(F_C));
    printf("PC: $%04x\n", next_pc);
    printf("SP: $%04x\n", regs.sp);
    printf("A: $%02x, F: $%02x\n", regs.a, regs.f);
    printf("B: $%02x, C: $%02x\n", regs.b, regs.c);
    printf("D: $%02x, E: $%02x\n", regs.d, regs.e);
    printf("H: $%02x, L: $%02x\n", regs.h, regs.l);
}

void cpu_step() {
    uint8_t instr_byte = mem_read_byte(regs.pc);

    /* if (regs.pc == 0x021B) */
    /*     _step_debug = true; */

    Instruction instr = instruction_get(instr_byte);
    current_instr_byte = instr_byte;
    current_instr = instr;

    if (instr.disasm == NULL) {
        cpu_print(regs.pc);
        exit(1);
    }

    uint8_t op1_8 = get_op_8(instr.t1);
    uint8_t op2_8 = get_op_8(instr.t2);

    /* uint16_t op1_16 = get_op_16(instr.t1); */
    uint16_t op2_16 = get_op_16(instr.t2);

    uint16_t next_pc = regs.pc + instr.length;

    switch (instr.op) {
        case DEC:
            cpu_dec(instr, op1_8);
            break;
        case JP:
            next_pc = cpu_jp(instr, op2_16);
            break;
        case JR:
            next_pc = cpu_jr(instr, op2_8);
            break;
        case LD:
            cpu_ld(instr, op2_8);
            break;
        case LD16:
            cpu_ld16(instr, op2_16);
            break;
        case NOP:
            break;
        case XOR:
            cpu_xor(instr, op1_8, op2_8);
            break;
        default: {
            fprintf(stderr, "Unhandled instruction at PC 0x%04x: %d\n", regs.pc, instr.op);
            cpu_print(regs.pc);
            exit(1);
        }
    }

    if (instr.t1 == HL_NEG || instr.t2 == HL_NEG)
        regs.hl--;

    if (instr.t1 == HL_POS || instr.t2 == HL_POS)
        regs.hl++;

    if (_step_debug) {
        cpu_print(next_pc);
        fgetc(stdin);
    }

    regs.pc = next_pc;
}

void cpu_dec(Instruction instr, uint8_t op1) {
    if (op1 & 0x0f)
        F_CLEAR(F_H);
    else
        F_SET(F_H);

    uint8_t res = op1 - 1;

    if (instr.t1 == HL_AS_ADDR)
        mem_write_byte(regs.hl, res);
    else
        set_reg_8(instr.t1, res);

    if (!res)
        F_SET(F_Z);
    else
        F_CLEAR(F_Z);

    F_SET(F_N);
}

uint16_t cpu_jp(Instruction instr, uint16_t op2) {
    bool jump = false;

    switch (instr.t1) {
        case NONE:
            jump = true;
            break;
        default:
            fprintf(stderr, "Unknown target passed to cpu_jp: %s\n", target_str_map[instr.t1]);
            break;
    }

    if (jump)
        return op2;

    return regs.pc += instr.length;
}

uint16_t cpu_jr(Instruction instr, int8_t op2) {
    bool jump = false;

    switch (instr.t1) {
        case NONE:
            jump = true;
            break;
        case C_NZ:
            jump = !F_ISSET(F_Z);
            break;
        default:
            fprintf(stderr, "Unknown target passed to cpu_jr: %s\n", target_str_map[instr.t1]);
            break;
    }

    if (jump)
        return regs.pc + op2 + 2;

    return regs.pc += instr.length;
}

void cpu_ld(Instruction instr, uint8_t op2) {
    switch (instr.t1) {
        case BC_AS_ADDR:
            mem_write_byte(regs.bc, op2);
            break;
        case DE_AS_ADDR:
            mem_write_byte(regs.de, op2);
            break;
        case HL_AS_ADDR:
        case HL_POS:
        case HL_NEG:
            mem_write_byte(regs.hl, op2);
            break;
        default:
            set_reg_8(instr.t1, op2);
            break;
    }
}

void cpu_ld16(Instruction instr, uint16_t op2) {
    set_reg_16(instr.t1, op2);
}

void cpu_xor(Instruction instr, uint8_t op1, uint8_t op2) {
    regs.a = op1 ^ op2;
    if (!regs.a)
        F_SET(F_Z);
    else
        F_CLEAR(F_Z);

    F_CLEAR(F_N | F_H | F_C);
}
