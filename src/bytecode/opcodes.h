#ifndef LUNA_OPCODES_H
#define LUNA_OPCODES_H

#include <stdint.h>

/*
 * Formato de instrucción:
 *   - 1 byte de opcode
 *   - Operandos según el opcode (0, 1, 2 o 3 bytes adicionales)
 *
 * Convenciones:
 *   A = 8 bits (slot de stack, constante pequeña, etc.)
 *   Bx = 16 bits (unsigned)
 *   sBx = 16 bits (signed, con sesgo de 32767 para saltos)
 */
typedef enum {
    /* ---------- Constantes y literales ---------- */
    OP_NOP = 0,          /* -                          */
    OP_LOAD_NIL,         /* A                          => stack[A] = nil */
    OP_LOAD_TRUE,        /* A                          => stack[A] = true */
    OP_LOAD_FALSE,       /* A                          => stack[A] = false */
    OP_LOAD_CONST,       /* A Bx                       => stack[A] = constants[Bx] */
    OP_LOAD_INT,         /* A sBx (sesgado)            => stack[A] = (int64_t)(sBx - 32767) */

    /* ---------- Variables locales y upvalues ---------- */
    OP_MOVE,             /* A B                        => stack[A] = stack[B] */
    OP_GET_UPVALUE,      /* A B                        => stack[A] = upvalue[B] */
    OP_SET_UPVALUE,      /* A B                        => upvalue[B] = stack[A] */

    /* ---------- Globales y campos ---------- */
    OP_GET_GLOBAL,       /* A Bx                       => stack[A] = globals[constants[Bx]] */
    OP_SET_GLOBAL,       /* A Bx                       => globals[constants[Bx]] = stack[A] */
    OP_GET_FIELD,        /* A B C                      => stack[A] = stack[B][constants[C]] */
    OP_SET_FIELD,        /* A B C                      => stack[A][constants[B]] = stack[C] */
    OP_GET_INDEX,        /* A B C                      => stack[A] = stack[B][stack[C]] */
    OP_SET_INDEX,        /* A B C                      => stack[A][stack[B]] = stack[C] */

    /* ---------- Tablas y listas ---------- */
    OP_NEW_TABLE,        /* A                          => stack[A] = {} */
    OP_NEW_LIST,         /* A                          => stack[A] = [] */
    OP_LIST_APPEND,      /* A B                        => list_append(stack[A], stack[B]) */
    OP_TABLE_SET,        /* A B C                      => table_set(stack[A], stack[B], stack[C]) */

    /* ---------- Aritmética y comparaciones ---------- */
    OP_ADD,              /* A B C                      => stack[A] = stack[B] + stack[C] */
    OP_SUB,              /* A B C                      => stack[A] = stack[B] - stack[C] */
    OP_MUL,              /* A B C                      => stack[A] = stack[B] * stack[C] */
    OP_DIV,              /* A B C                      => stack[A] = stack[B] / stack[C] */
    OP_MOD,              /* A B C                      => stack[A] = stack[B] % stack[C] */
    OP_POW,              /* A B C                      => stack[A] = stack[B] ** stack[C] */
    OP_NEG,              /* A B                        => stack[A] = -stack[B] */

    OP_EQ,               /* A B C                      => stack[A] = (stack[B] == stack[C]) */
    OP_NEQ,              /* A B C                      => stack[A] = (stack[B] != stack[C]) */
    OP_LT,               /* A B C                      => stack[A] = (stack[B] < stack[C]) */
    OP_LE,               /* A B C                      => stack[A] = (stack[B] <= stack[C]) */
    OP_GT,               /* A B C                      => stack[A] = (stack[B] > stack[C]) */
    OP_GE,               /* A B C                      => stack[A] = (stack[B] >= stack[C]) */

    /* ---------- Lógica ---------- */
    OP_NOT,              /* A B                        => stack[A] = !stack[B] */
    OP_AND,              /* A B C                      => stack[A] = stack[B] and stack[C] */
    OP_OR,               /* A B C                      => stack[A] = stack[B] or stack[C] */

    /* ---------- Concatenación y longitud ---------- */
    OP_CONCAT,           /* A B C                      => stack[A] = concat(stack[B..C]) */
    OP_LEN,              /* A B                        => stack[A] = len(stack[B]) */

    /* ---------- Control de flujo ---------- */
    OP_JUMP,             /* sBx                        => pc += sBx - 32767 */
    OP_JUMP_IF_TRUE,     /* A sBx                      => if stack[A] then pc += sBx - 32767 */
    OP_JUMP_IF_FALSE,    /* A sBx                      => if !stack[A] then pc += sBx - 32767 */

    /* ---------- Llamar y retornar ---------- */
    OP_CALL,             /* A B C                      => call stack[A](args=stack[A+1..A+B], results=C) */
    OP_RETURN,           /* A B                        => return stack[A..A+B-1] (B=0 => return nothing) */

    /* ---------- Closures ---------- */
    OP_CLOSURE,          /* A Bx                       => stack[A] = closure(protos[Bx]) */

    /* ---------- Iteración (for) ---------- */
    OP_FOR_INIT,         /* A sBx                      => inicializa for-loop en slot A; salta sBx si vacío */
    OP_FOR_STEP,         /* A sBx                      => avanza iterador en slot A; salta sBx si termina */

    /* ---------- Excepciones ---------- */
    OP_THROW,            /* A                          => throw stack[A] */
    OP_SETUP_EXC,        /* sBx                        => push handler con target pc += sBx - 32767 */
    OP_POP_EXC,          /*                            => pop handler */

    OP_COUNT             /* número de opcodes */
} LunaOpcode;

/* Nombres legibles (para dump y debug) */
const char* luna_opcode_name(LunaOpcode op);

/* Modo de direccionamiento por opcode (para el desensamblador) */
typedef enum {
    OP_ARG_NONE,         /* sin operandos */
    OP_ARG_A,            /* 8 bits */
    OP_ARG_AB,           /* 8 + 8 bits */
    OP_ARG_ABC,          /* 8 + 8 + 8 bits */
    OP_ARG_ABx,          /* 8 + 16 bits unsigned */
    OP_ARG_AsBx,         /* 8 + 16 bits signed (sesgado) */
    OP_ARG_sBx           /* 16 bits signed (salto puro) */
} LunaOpMode;

static inline LunaOpMode luna_opcode_mode(LunaOpcode op) {
    switch (op) {
        case OP_NOP:
        case OP_POP_EXC:
            return OP_ARG_NONE;

        case OP_LOAD_NIL:
        case OP_LOAD_TRUE:
        case OP_LOAD_FALSE:
        case OP_NEW_TABLE:
        case OP_NEW_LIST:
        case OP_THROW:
            return OP_ARG_A;

        case OP_MOVE:
        case OP_GET_UPVALUE:
        case OP_SET_UPVALUE:
        case OP_NEG:
        case OP_NOT:
        case OP_LEN:
        case OP_LIST_APPEND:
            return OP_ARG_AB;

        case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV:
        case OP_MOD: case OP_POW:
        case OP_EQ: case OP_NEQ: case OP_LT: case OP_LE:
        case OP_GT: case OP_GE:
        case OP_AND: case OP_OR:
        case OP_GET_FIELD: case OP_SET_FIELD:
        case OP_GET_INDEX: case OP_SET_INDEX:
        case OP_TABLE_SET:
        case OP_CONCAT:
        case OP_CALL:
        case OP_RETURN:
            return OP_ARG_ABC;

        case OP_LOAD_CONST:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
        case OP_CLOSURE:
            return OP_ARG_ABx;

        case OP_LOAD_INT:
        case OP_JUMP_IF_TRUE:
        case OP_JUMP_IF_FALSE:
        case OP_FOR_INIT:
        case OP_FOR_STEP:
        case OP_SETUP_EXC:
            return OP_ARG_AsBx;

        case OP_JUMP:
            return OP_ARG_sBx;

        default:
            return OP_ARG_NONE;
    }
}

#endif /* LUNA_OPCODES_H */