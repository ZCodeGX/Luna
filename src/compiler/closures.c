#include "compiler_internal.h"
#include <string.h>

/*
 * Resuelve una variable en el scope actual o en scopes padres.
 * Si es local del scope actual, devuelve slot >= 0.
 * Si es upvalue, añade la captura y devuelve -3 - índice.
 * Si es global, devuelve -2.
 */
int variable_is_const(LunaCompiler* C, const char* name) {
    int current = C->scope_depth - 1;
    if (current < 0) return 0;

    LunaScope* scope = &C->scope_stack[current];
    int slot = scope_find_local(scope, name);
    if (slot >= 0) return scope->locals[slot].is_const;

    if (current > 0) {
        LunaScope* parent = &C->scope_stack[current - 1];
        slot = scope_find_local(parent, name);
        if (slot >= 0) return parent->locals[slot].is_const;
    }

    return 0;
}

int resolve_variable(LunaCompiler* C, const char* name) {
    int current = C->scope_depth - 1;
    if (current < 0) return -2;

    int slot = scope_find_local(&C->scope_stack[current], name);
    if (slot >= 0) return slot;

    if (current > 0) {
        LunaScope* parent = &C->scope_stack[current - 1];
        slot = scope_find_local(parent, name);
        if (slot >= 0) {
            int up = scope_add_upvalue(&C->scope_stack[current], true, (uint8_t)slot);
            if (up >= 0) return -3 - up;
        }
    }

    return -2;
}