#include "vm_internal.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Operaciones de stack
 * ============================================================ */

void vm_stack_ensure(LunaState* L, size_t extra) {
    if (L->stack_top + extra <= L->stack_size) return;

    size_t new_size = L->stack_size * 2;
    while (new_size < L->stack_top + extra) new_size *= 2;

    LunaValue* new_stack = (LunaValue*)luna_alloc(L, L->stack,
                                                   L->stack_size * sizeof(LunaValue),
                                                   new_size * sizeof(LunaValue));
    if (!new_stack) {
        luna_error_set_simple(L, LUNA_ERR_MEMORY, "stack overflow");
        return;
    }

    /* Rellenar los nuevos slots con nil */
    for (size_t i = L->stack_size; i < new_size; i++) {
        new_stack[i] = luna_nil();
    }

    L->stack = new_stack;
    L->stack_size = new_size;
}

void vm_stack_push(LunaState* L, LunaValue v) {
    vm_stack_ensure(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = v;
}

LunaValue vm_stack_pop(LunaState* L) {
    if (L->stack_top == 0) {
        luna_error_set_simple(L, LUNA_ERR_RUNTIME, "stack underflow");
        return luna_nil();
    }
    LunaValue v = L->stack[--L->stack_top];
    L->stack[L->stack_top] = luna_nil();
    return v;
}