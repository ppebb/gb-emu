#include "cpu.h"
#include "defs.h"
#include "mem.h"
#include "src/instructions.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum _Flags {
    F_ZERO = 7,
    F_NEG = 6,
    F_HALF = 5,
    F_CARRY = 4,
} Flags;

#define F_ISSET(x)                   \
    ({                               \
        __typeof__(x) _x = 1 << (x); \
        (regs.f & (_x)) != 0;        \
    })

#define F_SET(x, val)                                   \
    ({                                                  \
        __typeof__(x) _x = (x);                         \
        __typeof__(x) _sx = 1 << _x;                    \
        regs.f = (regs.f & (~_sx)) | (!!((val)) << _x); \
    })

#define F_ENABLE(x) F_SET(x, 1);
#define F_CLEAR(x) F_SET(x, 0)

#define mem_read_d8() mem_read_byte(regs.pc + 1)
#define mem_read_d16() ((uint16_t)mem_read_byte(regs.pc + 2) << 8 | mem_read_byte(regs.pc + 1))

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

    bool ime;
    bool ime_next_cycle;
    bool halt;
} Regs;

Regs regs = (Regs){
    .af = 0x1180,
    .bc = 0x0000,
    .de = 0xff56,
    .hl = 0x000d,
    .pc = 0x100,
    .sp = 0xFFFE,
    .ime = false,
    .ime_next_cycle = false,
    .halt = false,
};

bool _step_debug;
Instruction current_instr;

bool stop = false;

void cpu_init(bool step_debug) {
    _step_debug = step_debug;
}

uint8_t get_op_8(Target target) {
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
        case C_AS_ADDR:
            return mem_read_byte(regs.c + 0xFF00);
        case BC_AS_ADDR:
            return mem_read_byte(regs.bc);
        case DE_AS_ADDR:
            return mem_read_byte(regs.de);
        case HL_POS:
        case HL_NEG:
        case HL_AS_ADDR:
            return mem_read_byte(regs.hl);
        case SP:
            return (uint8_t)regs.sp;
        case D8:
            return mem_read_d8();
        case D8_AS_ADDR:
            return mem_read_byte(0xFF00 + mem_read_d8());
        case D16_AS_ADDR:
            return mem_read_byte(mem_read_d16());
        default:
            /* printf("Unknown target get_op_8: %s\n", target_str_map[target]); */
            return 0x67;
    }
}

uint16_t get_op_16(Target target) {
    switch (target) {
        case D16:
            return mem_read_d16();
        case BC:
            return regs.bc;
        case DE:
            return regs.de;
        case HL:
            return regs.hl;
        case AF:
            return regs.af;
        case SP:
            return regs.sp;
        case D8:
            return mem_read_d8();
        default:
            /* printf("Unknown target get_op_16: %s\n", target_str_map[target]); */
            return 0x69;
    }
}

void set_dest_8(Target target, uint8_t data) {
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
            regs.f = data & 0xF0;
            break;
        case H:
            regs.h = data;
            break;
        case L:
            regs.l = data;
            break;
        case C_AS_ADDR:
            mem_write_byte(regs.c + 0xFF00, data);
            break;
        case BC_AS_ADDR:
            mem_write_byte(regs.bc, data);
            break;
        case DE_AS_ADDR:
            mem_write_byte(regs.de, data);
            break;
        case HL_AS_ADDR:
        case HL_POS:
        case HL_NEG:
            mem_write_byte(regs.hl, data);
            break;
        case D8_AS_ADDR:
            mem_write_byte(0xFF00 + mem_read_d8(), data);
            break;
        case D16_AS_ADDR:
            mem_write_byte(mem_read_d16(), data);
            break;
        default:
            fprintf(stderr, "Illegal write 8 to target %s\n", target_str_map[target]);

            cpu_print(regs.pc, 0);
            exit(1);
    }
}

void set_dest_16(Target target, uint16_t data) {
    switch (target) {
        case AF:
            regs.af = data & 0xFFF0;
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
        case D16_AS_ADDR:
            mem_write_short(mem_read_d16(), data);
            break;
        default:
            fprintf(stderr, "Illegal write 16 to target %s\n", target_str_map[target]);
            cpu_print(regs.pc, 0);
            exit(1);
    }
}

void cpu_print(uint16_t next_pc, uint8_t cycles) {
    if (current_instr.disasm == NULL)
        fprintf(stderr, "Unknown instruction at PC 0x%04x: 0x%02x\n", regs.pc, mem_read_byte(regs.pc));
    else {
        printf("Instr 0x%04x, %d: ", regs.pc, cycles);
        for (size_t i = 0; i < current_instr.length; i++)
            printf("%02x ", mem_read_byte(regs.pc + i));
        printf("; %s\n", current_instr.disasm);
    }

    printf("Z N H C\n");
    printf("%b %b %b %b\n", F_ISSET(F_ZERO), F_ISSET(F_NEG), F_ISSET(F_HALF), F_ISSET(F_CARRY));
    printf("PC: $%04x\n", next_pc);
    printf("SP: $%04x\n", regs.sp);
    printf("A: $%02x, F: $%02x\n", regs.a, regs.f);
    printf("B: $%02x, C: $%02x\n", regs.b, regs.c);
    printf("D: $%02x, E: $%02x\n", regs.d, regs.e);
    printf("H: $%02x, L: $%02x\n", regs.h, regs.l);
    printf("IME: %b, HALT: %b\n", regs.ime, regs.halt);
}

uint16_t cpu_pop_short() {
    uint16_t ret = mem_read_short(regs.sp);
    regs.sp += 2;
    return ret;
}

void cpu_push_short(uint16_t data) {
    regs.sp -= 2;
    mem_write_short(regs.sp, data);
}

uint16_t breakpoint = 0xFFFF;

int cpu_step() {
    if (regs.halt)
        return 1;

    if (regs.ime_next_cycle) {
        regs.ime_next_cycle = false;
        regs.ime = true;
    }

#ifdef DOCTOR
    printf(
        "A:%02x F:%02x B:%02x C:%02x D:%02x E:%02x H:%02x L:%02x SP:%04x PC:%04x PCMEM:%02x,%02x,%02x,%02x\n", regs.a,
        regs.f, regs.b, regs.c, regs.d, regs.e, regs.h, regs.l, regs.sp, regs.pc, mem_read_byte(regs.pc),
        mem_read_byte(regs.pc + 1), mem_read_byte(regs.pc + 2), mem_read_byte(regs.pc + 3));
#endif

    if (regs.pc == breakpoint)
        _step_debug = true;

    uint16_t instr_byte = mem_read_byte(regs.pc);

    if (instr_byte == 0xCB)
        instr_byte = (uint16_t)mem_read_byte(regs.pc + 1) + 0x100;

    Instruction instr = instruction_get(instr_byte);
    current_instr = instr;

    if (instr.disasm == NULL) {
        cpu_print(regs.pc, 0);
        exit(1);
    }

    uint8_t op1_8 = get_op_8(instr.t1);
    uint8_t op2_8 = get_op_8(instr.t2);

    uint16_t op1_16 = get_op_16(instr.t1);
    uint16_t op2_16 = get_op_16(instr.t2);

    JumpResult jump_result = (JumpResult){
        false,
        regs.pc + instr.length,
    };

    switch (instr.op) {
        case AND:
            cpu_and(instr, op1_8, op2_8);
            break;
        case ADC:
            cpu_adc(instr, op1_8, op2_8);
            break;
        case ADD:
            cpu_add(instr, op1_8, op2_8);
            break;
        case ADD16:
            cpu_add16(instr, op1_16, op2_16);
            break;
        case ADD_SPD8:
            // add16 with different carry flag logic
            cpu_add_spd8(instr, op1_16, op2_16);
            break;
        case CALL:
            jump_result = cpu_call(instr, op2_16);
            break;
        case CCF:
            cpu_ccf();
            break;
        case CP:
            cpu_cp(instr, op1_8, op2_8);
            break;
        case CPL:
            cpu_cpl();
            break;
        case DAA:
            cpu_daa();
            break;
        case DEC:
            cpu_dec(instr, op1_8);
            break;
        case DEC16:
            cpu_dec16(instr, op1_16);
            break;
        case DI:
            regs.ime = false;
            break;
        case EI:
            regs.ime_next_cycle = true;
            break;
        case HALT:
            regs.halt = true;
            break;
        case INC:
            cpu_inc(instr, op1_8);
            break;
        case INC16:
            cpu_inc16(instr, op1_16);
            break;
        case JP:
            jump_result = cpu_jp(instr, op2_16);
            break;
        case JR:
            jump_result = cpu_jr(instr, op2_8);
            break;
        case LD:
            cpu_ld(instr, op2_8);
            break;
        case LD16:
            cpu_ld16(instr, op2_16);
            break;
        case LD_SP_PLUS_D8:
            cpu_ld_sp_plus_d8(instr, op1_16, op2_8);
            break;
        case NOP:
            break;
        case OR:
            cpu_or(instr, op1_8, op2_8);
            break;
        case POP:
            cpu_pop(instr);
            break;
        case PUSH:
            cpu_push(instr, op1_16);
            break;
        case RET:
            jump_result = cpu_ret(instr);
            break;
        case RETI:
            jump_result = cpu_reti(instr);
            break;
        case RLC:
            cpu_rlc(instr, op1_8);
            break;
        case RLCA:
            cpu_rlc(instr, op1_8);
            F_CLEAR(F_ZERO);
            break;
        case RL:
            cpu_rl(instr, op1_8);
            break;
        case RLA:
            cpu_rl(instr, op1_8);
            F_CLEAR(F_ZERO);
            break;
        case RRC:
            cpu_rrc(instr, op1_8);
            break;
        case RRCA:
            cpu_rrc(instr, op1_8);
            F_CLEAR(F_ZERO);
            break;
        case RR:
            cpu_rr(instr, op1_8);
            break;
        case RRA:
            cpu_rr(instr, op1_8);
            F_CLEAR(F_ZERO);
            break;
        case RST:
            jump_result = cpu_rst(instr);
            break;
        case SCF:
            cpu_scf();
            break;
        case SBC:
            cpu_sbc(instr, op1_8, op2_8);
            break;
        case SUB:
            cpu_sub(instr, op1_8, op2_8);
            break;
        case STOP:
            stop = true;
            break;
        case XOR:
            cpu_xor(instr, op1_8, op2_8);
            break;
        case BIT:
            cpu_bit(instr, op1_8);
            break;
        case RES:
            cpu_res(instr, op1_8);
            break;
        case SET:
            cpu_set(instr, op1_8);
            break;
        case SLA:
            cpu_sla(instr, op1_8);
            break;
        case SRA:
            cpu_sra(instr, op1_8);
            break;
        case SRL:
            cpu_srl(instr, op1_8);
            break;
        case SWAP:
            cpu_swap(instr, op1_8);
            break;
        default: {
            fprintf(stderr, "Unhandled instruction at PC 0x%04x\n", regs.pc);
            cpu_print(regs.pc, 0);
            exit(1);
        }
    }

    if (instr.t1 == HL_NEG || instr.t2 == HL_NEG)
        regs.hl--;

    if (instr.t1 == HL_POS || instr.t2 == HL_POS)
        regs.hl++;

    uint8_t cycles;

    if (!instr.vlc)
        cycles = instr.cycles;
    else {
        // If the instruction is variable cycle, we need to select the correct count
        if (jump_result.taken)
            cycles = instr.jump_cycles.taken_cycles;
        else
            cycles = instr.jump_cycles.not_taken_cycles;
    }

    if (_step_debug) {
        cpu_print(jump_result.pc, cycles);

        while (true) {
            char *line = NULL;
            size_t size;
            getline(&line, &size, stdin);

            if (size == 0 || line == NULL)
                break;

            if (line[0] == '\n')
                break;

            // TODO: Make this not suck
            if (line[0] == 'm' && line[1] == 'e' && line[2] == 'm') {
                uint16_t addr = strtol(line + 3, NULL, 16);

                addr -= addr % 16;

                printf("ADDR   ");
                for (uint16_t i = 0; i < 16; i++)
                    printf("%02x ", i);
                printf("\n%04x:  ", addr);

                for (uint16_t i = 0; i < 16; i++)
                    printf("%02x ", mem_read_byte(addr + i));
                printf("\n");
            }
            else if (line[0] == 'b') {
                uint16_t addr = strtol(line + 1, NULL, 16);
                breakpoint = addr;
                printf("Breakpoint set to 0x%04x\n", addr);
            }
            else if (line[0] == 's') {
                _step_debug = !_step_debug;
                if (_step_debug)
                    printf("_step_debug enabled\n");
                else
                    printf("_step_debug disabled\n");
            }
        }
    }

    regs.pc = jump_result.pc;

    return cycles;
}

void cpu_req_interrupt(Interrupt interrupt) {
    uint8_t req = read_io(IRR_ADDR);
    req |= interrupt;
    write_io(IRR_ADDR, req);
}

bool cpu_run_interrupts() {
    uint8_t reqs = read_io(IRR_ADDR);

    // If the enabled interrupts do not overlap with the existing interrupt
    // requests, then no interrupts can be services
    if (!(ie & reqs))
        return false;

    // Exit halt if there is any overlap
    regs.halt = false;

    // If the master interrupt is disabled, do not service interrupts even if
    // we've exited halt
    if (!regs.ime)
        return false;

    uint8_t ie_flags = ie;

    for (size_t i = 0; i < 5; i++) {
        Interrupt interrupt = 1 << i;

        if (!(ie_flags & interrupt && reqs & interrupt))
            continue;

        regs.ime = false;
        reqs &= ~interrupt;
        write_io(IRR_ADDR, reqs);

        cpu_push_short(regs.pc);

        switch (interrupt) {
            case INT_VBLANK:
                regs.pc = 0x40;
                break;
            case INT_STAT:
                regs.pc = 0x48;
                break;
            case INT_TIMER:
                regs.pc = 0x50;
                break;
            case INT_SERIAL:
                regs.pc = 0x58;
                break;
            case INT_JOYPAD:
                regs.pc = 0x60;
                break;
        }

        return true;
    }

    return false;
}

bool cpu_cond(Target t) {
    switch (t) {
        case NONE:
            return true;
        case C_NZ:
            return !F_ISSET(F_ZERO);
        case C_Z:
            return F_ISSET(F_ZERO);
        case C_NC:
            return !F_ISSET(F_CARRY);
        case C_C:
            return F_ISSET(F_CARRY);
        default:
            fprintf(stderr, "Unknown target passed to cpu_cond: %s\n", target_str_map[t]);
            exit(1);
    }
}

void cpu_and(Instruction instr, uint8_t op1, uint8_t op2) {
    F_CLEAR(F_NEG);

    uint8_t res = op1 & op2;

    F_SET(F_ZERO, !res);
    F_ENABLE(F_HALF);
    F_CLEAR(F_CARRY);

    regs.a = res;
}

void cpu_adc(Instruction instr, uint8_t op1, uint8_t op2) {
    F_CLEAR(F_NEG);

    uint16_t res = op1 + op2 + F_ISSET(F_CARRY);

    F_SET(F_ZERO, !(uint8_t)res);
    F_SET(F_HALF, (op1 & 0x0F) + (op2 & 0x0F) + F_ISSET(F_CARRY) > 0x0F);
    F_SET(F_CARRY, res & 0xFF00);

    regs.a = (uint8_t)res;
}

void cpu_add(Instruction instr, uint8_t op1, uint8_t op2) {
    F_CLEAR(F_NEG);

    uint16_t res = op1 + op2;

    F_SET(F_ZERO, !(uint8_t)res);
    F_SET(F_CARRY, res & 0xFF00);
    F_SET(F_HALF, (op1 & 0x0F) + (op2 & 0x0F) > 0x0F);

    regs.a = (uint8_t)res;
}

void cpu_add16(Instruction instr, uint16_t op1, uint16_t op2) {
    F_CLEAR(F_NEG);

    uint32_t res = op1 + op2;

    F_SET(F_CARRY, res & 0xFFFF0000);
    F_SET(F_HALF, (op1 & 0x0FFF) + (op2 & 0x0FFF) > 0x0FFF);

    set_dest_16(instr.t1, (uint16_t)res);
}

void cpu_add_spd8(Instruction instr, uint16_t op1, int8_t op2) {
    F_CLEAR(F_ZERO);
    F_CLEAR(F_NEG);

    uint32_t res = op1 + op2;

    F_SET(F_CARRY, (op1 & 0x00FF) + (op2 & 0x00FF) > 0x00FF);
    F_SET(F_HALF, (op1 & 0x000F) + (op2 & 0x000F) > 0x000F);

    set_dest_16(instr.t1, (uint16_t)res);
}

JumpResult cpu_call(Instruction instr, uint16_t op2) {
    bool call = cpu_cond(instr.t1);

    // The address of the following instruction, pushed onto the stack
    uint16_t next_pc = regs.pc + instr.length;

    if (call) {
        cpu_push_short(next_pc);
        next_pc = op2;
    }

    return (JumpResult){
        call,
        next_pc,
    };
}

void cpu_ccf() {
    F_CLEAR(F_NEG);
    F_CLEAR(F_HALF);

    F_SET(F_CARRY, !F_ISSET(F_CARRY));
}

void cpu_cp(Instruction instr, uint8_t op1, uint8_t op2) {
    // performs op1 - op2
    F_SET(F_ZERO, op1 == op2);
    F_ENABLE(F_NEG);
    // Sub carry occurs for signed underflow
    F_SET(F_HALF, (op2 & 0x0f) > (op1 & 0x0f));
    F_SET(F_CARRY, op2 > op1);
}

void cpu_cpl() {
    F_ENABLE(F_NEG);
    F_ENABLE(F_HALF);

    regs.a = ~regs.a;
}

// See https://blog.ollien.com/posts/gb-daa/
void cpu_daa() {
    uint8_t offset = 0;
    uint8_t a = regs.a;
    bool half = F_ISSET(F_HALF);
    bool carry = F_ISSET(F_CARRY);
    bool sub = F_ISSET(F_NEG);

    if ((!sub && (a & 0x0F) > 0x09) || half)
        offset |= 0x06;

    bool new_carry = false;
    if ((!sub && a > 0x99) || carry) {
        offset |= 0x60;
        new_carry = true;
    }

    uint16_t res = sub ? a - offset : a + offset;

    regs.a = res;

    F_SET(F_ZERO, !regs.a);
    F_SET(F_CARRY, new_carry);
    F_CLEAR(F_HALF);
}

void cpu_dec(Instruction instr, uint8_t op1) {
    F_SET(F_HALF, !(op1 & 0x0f));

    uint8_t res = op1 - 1;

    set_dest_8(instr.t1, res);

    F_SET(F_ZERO, !res);
    F_ENABLE(F_NEG);
}

void cpu_dec16(Instruction instr, uint16_t op1) {
    set_dest_16(instr.t1, op1 - 1);
}

void cpu_inc(Instruction instr, uint8_t op1) {
    F_SET(F_HALF, (op1 & 0x0f) == 0x0f);

    uint8_t res = op1 + 1;

    set_dest_8(instr.t1, res);

    F_SET(F_ZERO, !res);
    F_CLEAR(F_NEG);
}

void cpu_inc16(Instruction instr, uint16_t op1) {
    set_dest_16(instr.t1, op1 + 1);
}

JumpResult cpu_jp(Instruction instr, uint16_t op2) {
    bool jump = cpu_cond(instr.t1);

    return (JumpResult){
        jump,
        jump ? op2 : regs.pc + instr.length,
    };
}

JumpResult cpu_jr(Instruction instr, int8_t op2) {
    bool jump = cpu_cond(instr.t1);

    return (JumpResult){
        jump,
        jump ? regs.pc + op2 + 2 : regs.pc + instr.length,
    };
}

void cpu_ld(Instruction instr, uint8_t op2) {
    set_dest_8(instr.t1, op2);
}

void cpu_ld16(Instruction instr, uint16_t op2) {
    set_dest_16(instr.t1, op2);
}

void cpu_ld_sp_plus_d8(Instruction instr, uint16_t sp, int8_t d8) {
    F_CLEAR(F_ZERO);
    F_CLEAR(F_NEG);

    uint32_t res = sp + d8;

    F_SET(F_CARRY, (sp & 0x00FF) + (d8 & 0x00FF) > 0x00FF);
    F_SET(F_HALF, (sp & 0x000F) + (d8 & 0x000F) > 0x000F);

    regs.hl = (uint16_t)res;
}

void cpu_or(Instruction instr, uint8_t op1, uint8_t op2) {
    regs.a = op1 | op2;

    F_SET(F_ZERO, !regs.a);
    F_CLEAR(F_NEG);
    F_CLEAR(F_HALF);
    F_CLEAR(F_CARRY);
}

void cpu_pop(Instruction instr) {
    set_dest_16(instr.t1, cpu_pop_short());
}

void cpu_push(Instruction instr, uint16_t op1) {
    cpu_push_short(op1);
}

JumpResult cpu_ret(Instruction instr) {
    bool should_ret = cpu_cond(instr.t1);

    uint16_t next_pc = regs.pc + instr.length;

    if (should_ret)
        next_pc = cpu_pop_short();

    return (JumpResult){
        should_ret,
        next_pc,
    };
}

JumpResult cpu_reti(Instruction instr) {
    bool should_ret = cpu_cond(instr.t1);

    uint16_t next_pc = regs.pc + instr.length;

    if (should_ret) {
        next_pc = cpu_pop_short();
        regs.ime = true;
    }

    return (JumpResult){
        should_ret,
        next_pc,
    };
}

#define rl(x) (((x) << 1) | ((x) >> 7))

void cpu_rlc(Instruction instr, uint8_t op) {
    F_SET(F_CARRY, op & 1 << 7);

    uint8_t res = rl(op);
    F_SET(F_ZERO, !res);
    F_CLEAR(F_NEG);
    F_CLEAR(F_HALF);

    set_dest_8(instr.t1, res);
}

void cpu_rl(Instruction instr, uint8_t op) {
    uint8_t old_carry = F_ISSET(F_CARRY);
    F_SET(F_CARRY, op & 1 << 7);

    uint8_t res = (rl(op) & 0xFE) | old_carry;
    F_SET(F_ZERO, !res);
    F_CLEAR(F_NEG);
    F_CLEAR(F_HALF);

    set_dest_8(instr.t1, res);
}

#define rr(x) (((x) >> 1) | ((x) << 7))

void cpu_rrc(Instruction instr, uint8_t op) {
    F_SET(F_CARRY, op & 1);

    uint8_t res = rr(op);

    F_SET(F_ZERO, !res);
    F_CLEAR(F_NEG);
    F_CLEAR(F_HALF);

    set_dest_8(instr.t1, res);
}

void cpu_rr(Instruction instr, uint8_t op) {
    uint8_t old_carry = F_ISSET(F_CARRY);
    F_SET(F_CARRY, op & 1);

    uint8_t res = (rr(op) & 0x7F) | old_carry << 7;
    F_SET(F_ZERO, !res);
    F_CLEAR(F_NEG);
    F_CLEAR(F_HALF);

    set_dest_8(instr.t1, res);
}

JumpResult cpu_rst(Instruction instr) {
    cpu_push_short(regs.pc + instr.length);

    return (JumpResult){
        true,
        instr.t1 * 8,
    };
}

void cpu_sbc(Instruction instr, uint8_t op1, uint8_t op2) {
    F_ENABLE(F_NEG);

    regs.a = op1 - (op2 + F_ISSET(F_CARRY));

    F_SET(F_ZERO, !regs.a);
    F_SET(F_HALF, (op2 & 0x0f) + F_ISSET(F_CARRY) > (op1 & 0x0f));
    F_SET(F_CARRY, op2 + F_ISSET(F_CARRY) > op1);
}

void cpu_scf() {
    F_CLEAR(F_NEG);
    F_CLEAR(F_HALF);
    F_ENABLE(F_CARRY);
}

void cpu_sub(Instruction instr, uint8_t op1, uint8_t op2) {
    F_ENABLE(F_NEG);

    regs.a = op1 - op2;

    F_SET(F_ZERO, !regs.a);
    F_SET(F_HALF, (op2 & 0x0f) > (op1 & 0x0f));
    F_SET(F_CARRY, op2 > op1);
}

void cpu_xor(Instruction instr, uint8_t op1, uint8_t op2) {
    regs.a = op1 ^ op2;

    F_SET(F_ZERO, !regs.a);
    F_CLEAR(F_NEG);
    F_CLEAR(F_HALF);
    F_CLEAR(F_CARRY);
}

// 16-bit opcodes

void cpu_bit(Instruction instr, uint8_t op) {
    uint8_t bit_idx = instr.t2;
    assert(bit_idx < 8);

    F_SET(F_ZERO, !((op >> bit_idx) & 1));
    F_CLEAR(F_NEG);
    F_ENABLE(F_HALF);
}

void cpu_res(Instruction instr, uint8_t op) {
    uint8_t bit_idx = instr.t2;
    assert(bit_idx < 8);

    set_dest_8(instr.t1, op & ~(1 << bit_idx));
}

void cpu_set(Instruction instr, uint8_t op) {
    uint8_t bit_idx = instr.t2;
    assert(bit_idx < 8);

    set_dest_8(instr.t1, op | (1 << bit_idx));
}

void cpu_sla(Instruction instr, uint8_t op) {
    F_SET(F_CARRY, op & 1 << 7);

    uint8_t res = op << 1;
    set_dest_8(instr.t1, res);

    F_SET(F_ZERO, !res);
    F_CLEAR(F_NEG);
    F_CLEAR(F_HALF);
}

void cpu_sra(Instruction instr, uint8_t op) {
    F_SET(F_CARRY, op & 1);

    uint8_t res = op >> 1 | (op & 1 << 7);
    set_dest_8(instr.t1, res);
    F_SET(F_ZERO, !res);
    F_CLEAR(F_NEG);
    F_CLEAR(F_HALF);
}

void cpu_srl(Instruction instr, uint8_t op) {
    F_SET(F_CARRY, op & 1);

    uint8_t res = op >> 1;
    set_dest_8(instr.t1, res);
    F_SET(F_ZERO, !res);
    F_CLEAR(F_NEG);
    F_CLEAR(F_HALF);
}

void cpu_swap(Instruction instr, uint8_t op) {
    uint8_t res = op >> 4 | op << 4;
    set_dest_8(instr.t1, res);

    F_SET(F_ZERO, !res);
    F_CLEAR(F_NEG);
    F_CLEAR(F_HALF);
    F_CLEAR(F_CARRY);
}
