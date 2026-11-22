#include "scope.h"
#include <stdlib.h>
#include <string.h>

void scope_init(LunaScope* S, int depth) {
    memset(S, 0, sizeof(*S));
    S->depth = depth;
    S->scope_start = 0;
}

int scope_add_local(LunaScope* S, const char* name, bool is_const) {
    if (S->num_locals >= LUNA_MAX_LOCALS) return -1;
    
    int slot = S->num_locals;
    size_t len = strlen(name);
    S->locals[slot].name = (char*)malloc(len + 1);
    if (S->locals[slot].name) {
        memcpy(S->locals[slot].name, name, len + 1);
    }
    S->locals[slot].slot = (uint8_t)slot;
    S->locals[slot].is_const = is_const;
    S->num_locals++;
    
    return slot;
}

int scope_find_local(LunaScope* S, const char* name) {
    for (int i = S->num_locals - 1; i >= 0; i--) {
        if (strcmp(S->locals[i].name, name) == 0) {
            return S->locals[i].slot;
        }
    }
    return -1;
}

int scope_add_upvalue(LunaScope* S, bool is_local, uint8_t index) {
    for (int i = 0; i < S->num_upvalues; i++) {
        if (S->upvalues[i].is_local == is_local && S->upvalues[i].index == index) {
            return i;
        }
    }
    
    if (S->num_upvalues >= LUNA_MAX_UPVALUES) return -1;
    
    int idx = S->num_upvalues;
    S->upvalues[idx].is_local = is_local;
    S->upvalues[idx].index = index;
    S->num_upvalues++;
    
    return idx;
}

void scope_free(LunaScope* S) {
    for (int i = 0; i < S->num_locals; i++) {
        free(S->locals[i].name);
    }
}