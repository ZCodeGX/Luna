#include "api_internal.h"

int luna_gettop(LunaState* L) {
    return (int)L->stack_top;
}

void luna_settop(LunaState* L, int idx) {
    if (idx >= 0) {
        size_t new_top = (size_t)idx;
        if (new_top > L->stack_size) {
            api_ensure_stack(L, new_top - L->stack_top);
            if (L->status != LUNA_OK) return;
        }
        /* Rellenar con nil si crece */
        for (size_t i = L->stack_top; i < new_top; i++) {
            L->stack[i] = luna_nil();
        }
        /* Limpiar si decrece */
        for (size_t i = new_top; i < L->stack_top; i++) {
            L->stack[i] = luna_nil();
        }
        L->stack_top = new_top;
    } else {
        /* idx negativo: decrementar */
        int dec = -idx;
        if ((int)L->stack_top >= dec) {
            size_t new_top = L->stack_top - (size_t)dec;
            for (size_t i = new_top; i < L->stack_top; i++) {
                L->stack[i] = luna_nil();
            }
            L->stack_top = new_top;
        } else {
            L->stack_top = 0;
        }
    }
}

void luna_pushvalue(LunaState* L, int idx) {
    LunaValue v = api_get(L, idx);
    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = v;
}

void luna_pop(LunaState* L, int n) {
    if (n <= 0) return;
    if ((int)L->stack_top < n) {
        L->stack_top = 0;
        return;
    }
    for (int i = 0; i < n; i++) {
        L->stack[--L->stack_top] = luna_nil();
    }
}

void luna_remove(LunaState* L, int idx) {
    size_t slot = api_to_slot(L, idx);
    if (slot == (size_t)-1 || slot >= L->stack_top) return;
    /* Mover todos hacia abajo */
    for (size_t i = slot; i + 1 < L->stack_top; i++) {
        L->stack[i] = L->stack[i + 1];
    }
    L->stack[--L->stack_top] = luna_nil();
}

void luna_insert(LunaState* L, int idx) {
    size_t slot = api_to_slot(L, idx);
    if (slot == (size_t)-1 || slot >= L->stack_top) return;
    if (slot == L->stack_top - 1) return;  /* ya está en su sitio */
    /* El valor a mover es el top */
    LunaValue top = L->stack[L->stack_top - 1];
    /* Mover hacia arriba desde slot */
    for (size_t i = L->stack_top - 1; i > slot; i--) {
        L->stack[i] = L->stack[i - 1];
    }
    L->stack[slot] = top;
}

void luna_replace(LunaState* L, int idx) {
    size_t slot = api_to_slot(L, idx);
    if (slot == (size_t)-1 || slot >= L->stack_top || L->stack_top == 0) return;
    LunaValue top = L->stack[L->stack_top - 1];
    L->stack[--L->stack_top] = luna_nil();
    L->stack[slot] = top;
}

void luna_xmove(LunaState* from, LunaState* to, int n) {
    if (n <= 0 || from == to) return;
    if ((int)from->stack_top < n) return;

    api_ensure_stack(to, (size_t)n);
    if (to->status != LUNA_OK) return;

    for (int i = 0; i < n; i++) {
        to->stack[to->stack_top++] = from->stack[from->stack_top - n + i];
    }
    from->stack_top -= (size_t)n;
    for (size_t i = from->stack_top; i < from->stack_top + (size_t)n; i++) {
        if (i < from->stack_size) from->stack[i] = luna_nil();
    }
}