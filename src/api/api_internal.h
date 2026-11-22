#ifndef LUNA_API_INTERNAL_H
#define LUNA_API_INTERNAL_H

#include "../../include/luna/luna.h"
#include "../runtime/runtime_internal.h"
#include "../runtime/value.h"
#include "../runtime/lstring.h"
#include "../runtime/table.h"
#include "../runtime/list.h"
#include "../runtime/function.h"
#include "../runtime/userdata.h"
#include "../errors/errors_internal.h"
#include "../vm/vm.h"
#include "../vm/vm_internal.h"
#include "../memory/gc.h"
#include "../memory/alloc.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================
 * LunaRuntime (configuración compartida entre estados)
 * ============================================================ */

struct LunaRuntime {
    LunaAllocator allocator;
    size_t        memory_limit;
    uint64_t      instruction_limit;
};

void luna_open_libs(LunaState* L);

/* ============================================================
 * Helpers internos de la API
 * ============================================================ */

/* Convierte un índice relativo (negativo) a absoluto (1-based).
 * Devuelve 0 si el índice es inválido. */
static inline int api_abs_index(LunaState* L, int idx) {
    int top = (int)L->stack_top;
    if (idx > 0) return idx;
    if (idx < 0) return top + idx + 1;
    return 0;  /* índice 0 es inválido */
}

/* Convierte un índice de API (1-based) a slot interno (0-based).
 * Devuelve (size_t)-1 si el índice es inválido. */
static inline size_t api_to_slot(LunaState* L, int idx) {
    int abs_idx = api_abs_index(L, idx);
    if (abs_idx < 1 || abs_idx > (int)L->stack_size) return (size_t)-1;
    return (size_t)(abs_idx - 1);
}

/* Obtiene el valor en un índice de la API. Devuelve nil si inválido. */
static inline LunaValue api_get(LunaState* L, int idx) {
    size_t slot = api_to_slot(L, idx);
    if (slot == (size_t)-1 || slot >= L->stack_top) return luna_nil();
    return L->stack[slot];
}

/* Establece el valor en un índice de la API. */
static inline void api_set(LunaState* L, int idx, LunaValue v) {
    size_t slot = api_to_slot(L, idx);
    if (slot == (size_t)-1 || slot >= L->stack_top) return;
    L->stack[slot] = v;
}

/* Reporta un error de tipo en la API. */
static inline void api_type_error(LunaState* L, int idx, const char* expected) {
    LunaValue v = api_get(L, idx);
    luna_error_set(L, LUNA_ERR_TYPE, NULL, 0, 0,
                   "bad argument #%d (%s expected, got %s)",
                   idx, expected, luna_type_name(v.type));
}

/* Asegura que el stack tenga al menos `n` slots libres por encima del top. */
static inline void api_ensure_stack(LunaState* L, size_t n) {
    if (L->stack_top + n > L->stack_size) {
        size_t new_size = L->stack_size * 2;
        while (new_size < L->stack_top + n) new_size *= 2;
        LunaValue* new_stack = (LunaValue*)luna_alloc(L, L->stack,
                                                       L->stack_size * sizeof(LunaValue),
                                                       new_size * sizeof(LunaValue));
        if (!new_stack) {
            luna_error_set_simple(L, LUNA_ERR_MEMORY, "stack overflow");
            return;
        }
        for (size_t i = L->stack_size; i < new_size; i++) {
            new_stack[i] = luna_nil();
        }
        L->stack = new_stack;
        L->stack_size = new_size;
    }
}

#endif /* LUNA_API_INTERNAL_H */