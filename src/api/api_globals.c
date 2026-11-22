#include "api_internal.h"

void luna_getglobal(LunaState* L, const char* name) {
    LunaString* key = luna_string_newz(L, name);
    if (!key) {
        luna_pushnil(L);
        return;
    }

    LunaValue v;
    if (!luna_table_get(L->globals, luna_string_val(key), &v)) {
        v = luna_nil();
    }

    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = v;
}

void luna_setglobal(LunaState* L, const char* name) {
    if (L->stack_top == 0) return;
    LunaValue v = L->stack[--L->stack_top];
    L->stack[L->stack_top] = luna_nil();

    LunaString* key = luna_string_newz(L, name);
    if (!key) return;

    luna_table_set(L, L->globals, luna_string_val(key), v);
}

int luna_getupvalue(LunaState* L, int funcindex, int n) {
    LunaValue fv = api_get(L, funcindex);
    if (fv.type != LUNA_TFUNCTION) {
        luna_pushnil(L);
        return LUNA_TNIL;
    }

    LunaFunction* f = luna_val_function(fv);
    if (n < 1 || (size_t)n > f->num_upvalues || !f->upvalues) {
        luna_pushnil(L);
        return LUNA_TNIL;
    }

    LunaUpvalue* uv = f->upvalues[n - 1];
    if (!uv) {
        luna_pushnil(L);
        return LUNA_TNIL;
    }

    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return LUNA_TNIL;
    L->stack[L->stack_top++] = *uv->location;
    return L->stack[L->stack_top - 1].type;
}