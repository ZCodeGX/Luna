#include "compiler_internal.h"
#include <string.h>

/*
 * Resuelve una variable en el scope actual o en scopes padres.
 * Si es local del scope actual, devuelve slot >= 0.
 * Si es upvalue, añade la captura y devuelve el índice.
 * Si es global, devuelve -2.
 */
int resolve_variable(LunaCompiler* C, const char* name) {
    /* Buscar en scope actual */
    int slot = scope_find_local(&C->scope_stack[C->scope_depth], name);
    if (slot >= 0) return slot;
    
    /* Buscar en scopes padres */
    for (int i = C->scope_depth - 1; i >= 0; i--) {
        LunaScope* parent = &C->scope_stack[i];
        slot = scope_find_local(parent, name);
        if (slot >= 0) {
            /* Es local del padre inmediato o upvalue del padre */
            bool is_local = (i == C->scope_depth - 1);
            uint8_t index = (uint8_t)slot;
            
            /* Si no es local directo, necesitamos resolver upvalues recursivamente */
            if (!is_local) {
                /* Buscar o crear upvalue en el padre */
                int up_idx = -1;
                for (int j = 0; j < parent->num_upvalues; j++) {
                    if (parent->upvalues[j].is_local == false && 
                        parent->upvalues[j].index == index) {
                        up_idx = j;
                        break;
                    }
                }
                if (up_idx < 0) {
                    up_idx = scope_add_upvalue(parent, false, index);
                }
                index = (uint8_t)up_idx;
            }
            
            /* Añadir upvalue al scope actual */
            return scope_add_upvalue(&C->scope_stack[C->scope_depth], is_local, index);
        }
    }
    
    /* No encontrada: es global */
    return -2;
}