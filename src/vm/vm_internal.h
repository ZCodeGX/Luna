#ifndef LUNA_VM_INTERNAL_H
#define LUNA_VM_INTERNAL_H

#include "vm.h"
#include "../runtime/runtime_internal.h"
#include "../runtime/value.h"
#include "../runtime/lstring.h"
#include "../runtime/table.h"
#include "../runtime/list.h"
#include "../runtime/function.h"
#include "../runtime/userdata.h"
#include "../bytecode/proto.h"
#include "../bytecode/opcodes.h"
#include "../errors/errors_internal.h"
#include <stdint.h>

/* ============================================================
 * CallFrame: un frame de ejecución
 * ============================================================ */

#define LUNA_MAX_CALLS 200

typedef struct CallFrame {
    LunaValue*     base;           /* base del stack para este frame */
    LunaProto*     proto;          /* proto en ejecución (NULL para C) */
    LunaFunction*  func;           /* función (para natives) */
    size_t         pc;             /* program counter */
    uint8_t        num_results;    /* resultados esperados por el caller */
    int            is_c;           /* 1 si es llamada a función C */
    LunaUpvalue**  open_upvalues;  /* upvalues abiertos de este frame */
    int            num_open_upvalues;
    int            handler_pc;     /* -1 si no hay handler; sino pc destino */
} CallFrame;

/* ============================================================
 * Estado interno de la VM (extendido sobre LunaState)
 * ============================================================ */

typedef struct {
    CallFrame  frames[LUNA_MAX_CALLS];
    int        frame_top;          /* número de frames activos */

    /* Upvalues abiertos globales (lista enlazada por uv->next) */
    LunaUpvalue* open_upvalues;
} LunaVM;

/* ============================================================
 * Helpers de decodificación de instrucciones
 * ============================================================ */

static inline uint8_t decode_op(const uint8_t* code, size_t pc) {
    return code[pc];
}

static inline uint8_t decode_A(const uint8_t* code, size_t pc) {
    return code[pc + 1];
}

static inline uint8_t decode_B(const uint8_t* code, size_t pc) {
    return code[pc + 2];
}

static inline uint8_t decode_C(const uint8_t* code, size_t pc) {
    return code[pc + 3];
}

static inline uint16_t decode_Bx(const uint8_t* code, size_t pc) {
    return (uint16_t)code[pc + 2] | ((uint16_t)code[pc + 3] << 8);
}

static inline int16_t decode_sBx(const uint8_t* code, size_t pc) {
    return (int16_t)(decode_Bx(code, pc) - 32767);
}

/* Avanza el PC según el modo del opcode. Devuelve el nuevo PC. */
static inline size_t advance_pc(const uint8_t* code, size_t pc) {
    LunaOpMode mode = luna_opcode_mode((LunaOpcode)code[pc]);
    switch (mode) {
        case OP_ARG_NONE: return pc + 1;
        case OP_ARG_A:    return pc + 2;
        case OP_ARG_AB:   return pc + 3;
        case OP_ARG_ABC:  return pc + 4;
        case OP_ARG_ABx:  return pc + 4;
        case OP_ARG_AsBx: return pc + 4;
        case OP_ARG_sBx:  return pc + 3;
    }
    return pc + 1;
}

/* ============================================================
 * API interna de la VM
 * ============================================================ */

/* Stack */
void vm_stack_push(LunaState* L, LunaValue v);
LunaValue vm_stack_pop(LunaState* L);
void vm_stack_ensure(LunaState* L, size_t extra);

/* Aritmética y comparaciones (arithmetic.c / compare.c) */
int vm_arith(LunaState* L, LunaValue a, LunaValue b, LunaValue* out, LunaOpcode op);
int vm_neg(LunaState* L, LunaValue a, LunaValue* out);
int vm_compare(LunaState* L, LunaValue a, LunaValue b, LunaOpcode op, int* out);

/* Llamadas (call.c) */
int vm_call_function(LunaState* L, int func_index, int nargs, int nresults);
int vm_call_internal(LunaState* L, LunaValue func, int nargs, int nresults);

/* Upvalues (call.c) */
LunaUpvalue* vm_find_upvalue(LunaState* L, CallFrame* frame, LunaValue* slot);
void vm_close_upvalues(LunaState* L, LunaValue* from);

/* Errores */
int vm_throw(LunaState* L, LunaValue value);
int vm_error_type(LunaState* L, const char* op, LunaValue a, LunaValue b);

/* Acceso a la VM desde el estado */
static inline LunaVM* state_vm(LunaState* L) {
    return (LunaVM*)L->vm_data;
}

#endif /* LUNA_VM_INTERNAL_H */