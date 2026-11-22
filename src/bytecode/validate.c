#include "proto.h"
#include "opcodes.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>    /* <-- añade esta línea */

typedef struct {
    int   failed;
    char  message[256];
    int   proto_depth;
} Validator;

static void vfail(Validator* V, const char* fmt, ...) {
    if (V->failed) return;
    V->failed = 1;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(V->message, sizeof(V->message), fmt, ap);
    va_end(ap);
}

/* Cuenta el tamaño en bytes de una instrucción dado su opcode. */
static int instruction_size(uint8_t op) {
    LunaOpMode mode = luna_opcode_mode((LunaOpcode)op);
    switch (mode) {
        case OP_ARG_NONE: return 1;
        case OP_ARG_A:    return 2;
        case OP_ARG_AB:   return 3;
        case OP_ARG_ABC:  return 4;
        case OP_ARG_ABx:  return 4;
        case OP_ARG_AsBx: return 4;
        case OP_ARG_sBx:  return 3;
    }
    return -1;
}

static void validate_code(Validator* V, const LunaProto* p) {
    size_t i = 0;
    size_t inst_idx = 0;
    while (i < p->code_len) {
        uint8_t op = p->code[i];
        if (op >= OP_COUNT) {
            vfail(V, "invalid opcode %d at byte %zu", op, i);
            return;
        }
        int sz = instruction_size(op);
        if (sz < 0) {
            vfail(V, "unknown addressing mode for opcode %d", op);
            return;
        }
        if (i + sz > p->code_len) {
            vfail(V, "truncated instruction at byte %zu", i);
            return;
        }
        i += sz;
        inst_idx++;
    }

    if (inst_idx != p->num_instructions) {
        vfail(V, "instruction count mismatch: code has %zu, lines has %zu",
              inst_idx, p->num_instructions);
    }
}

static void validate_const_refs(Validator* V, const LunaProto* p) {
    size_t i = 0;
    while (i < p->code_len) {
        uint8_t op = p->code[i];
        /* Quita la línea: LunaOpMode mode = luna_opcode_mode((LunaOpcode)op); */

        if (op == OP_LOAD_CONST || op == OP_GET_GLOBAL || op == OP_SET_GLOBAL || op == OP_CLOSURE) {
            uint16_t bx = (uint16_t)p->code[i + 2] | ((uint16_t)p->code[i + 3] << 8);
            if (bx >= p->num_constants) {
                vfail(V, "constant index %u out of range (max %zu) at byte %zu",
                      bx, p->num_constants, i);
                return;
            }
        }

        i += instruction_size(op);
    }
}

static void validate_jumps(Validator* V, const LunaProto* p) {
    size_t i = 0;
    size_t inst_idx = 0;
    while (i < p->code_len) {
        uint8_t op = p->code[i];
        LunaOpMode mode = luna_opcode_mode((LunaOpcode)op);

        if (op == OP_JUMP || op == OP_JUMP_IF_TRUE || op == OP_JUMP_IF_FALSE ||
            op == OP_SETUP_EXC || op == OP_FOR_INIT || op == OP_FOR_STEP) {
            uint16_t bx;
            int a_pos;
            if (mode == OP_ARG_sBx) {
                bx = (uint16_t)p->code[i + 1] | ((uint16_t)p->code[i + 2] << 8);
                a_pos = 0;
            } else {
                bx = (uint16_t)p->code[i + 2] | ((uint16_t)p->code[i + 3] << 8);
                a_pos = 1;
            }
            int32_t offset = (int32_t)bx - 32767;
            int32_t target = (int32_t)inst_idx + offset;

            if (target < 0 || target > (int32_t)p->num_instructions) {
                vfail(V, "jump target %d out of range [0, %zu] at instruction %zu",
                      target, p->num_instructions, inst_idx);
                return;
            }
            (void)a_pos;
        }

        i += instruction_size(op);
        inst_idx++;
    }
}

static void validate_proto_refs(Validator* V, const LunaProto* p) {
    size_t i = 0;
    while (i < p->code_len) {
        uint8_t op = p->code[i];
        if (op == OP_CLOSURE) {
            uint16_t bx = (uint16_t)p->code[i + 2] | ((uint16_t)p->code[i + 3] << 8);
            if (bx >= p->num_protos) {
                vfail(V, "proto index %u out of range (max %zu) at byte %zu",
                      bx, p->num_protos, i);
                return;
            }
        }
        i += instruction_size(op);
    }
}

static void validate_proto(Validator* V, const LunaProto* p) {
    if (V->failed) return;
    if (!p) {
        vfail(V, "null proto at depth %d", V->proto_depth);
        return;
    }

    if (p->num_params > 64) {
        vfail(V, "too many parameters (%d) in proto '%s'",
              p->num_params, p->name ? p->name : "<anon>");
        return;
    }

    validate_code(V, p);
    if (V->failed) return;

    validate_const_refs(V, p);
    if (V->failed) return;

    validate_jumps(V, p);
    if (V->failed) return;

    validate_proto_refs(V, p);
    if (V->failed) return;

    V->proto_depth++;
    for (size_t i = 0; i < p->num_protos; i++) {
        validate_proto(V, p->protos[i]);
        if (V->failed) return;
    }
    V->proto_depth--;
}

int luna_proto_validate(const LunaProto* root, char* error_out, size_t error_size) {
    Validator V = { .failed = 0, .message = {0}, .proto_depth = 0 };
    validate_proto(&V, root);

    if (V.failed && error_out) {
        snprintf(error_out, error_size, "%s", V.message);
    }
    return V.failed ? -1 : 0;
}