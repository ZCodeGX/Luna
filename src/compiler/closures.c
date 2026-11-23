#include "compiler_internal.h"
#include <string.h>

/*
 * Resuelve una variable en el scope actual o en scopes padres.
 * Si es local del scope actual, devuelve slot >= 0.
 * Si es upvalue, añade la captura y devuelve -3 - índice.
 * Si es global, devuelve -2.
 */
static int find_local_owner(LunaCompiler* C, const char* name, int* owner, int* slot) {
    for (int s = C->scope_depth - 1; s >= 0; s--) {
        int found = scope_find_local(&C->scope_stack[s], name);
        if (found >= 0) {
            *owner = s;
            *slot = found;
            return 1;
        }
    }
    return 0;
}

int variable_is_const(LunaCompiler* C, const char* name) {
    int owner = -1;
    int slot = -1;
    if (!find_local_owner(C, name, &owner, &slot)) return 0;
    return C->scope_stack[owner].locals[slot].is_const;
}

int resolve_variable(LunaCompiler* C, const char* name) {
    int current = C->scope_depth - 1;
    if (current < 0) return -2;

    int owner = -1;
    int slot = -1;
    if (!find_local_owner(C, name, &owner, &slot)) return -2;
    if (owner == current) return slot;

    int up = -1;
    for (int s = owner + 1; s <= current; s++) {
        int direct = s == owner + 1;
        up = scope_add_upvalue(&C->scope_stack[s], direct,
                               (uint8_t)(direct ? slot : up));
        if (up < 0) return -2;
    }

    return -3 - up;
}