#include "api_internal.h"

#define LUNA_REF_NONE  0
#define LUNA_REF_FREE  (-1)

/*
 * Tabla de referencias: una tabla interna donde guardamos valores
 * para que el GC no los libere. Las claves son enteros (ref ids).
 *
 * L->ref_table: tabla que mapea ref_id → value
 * L->ref_free:  próximo ref_id libre (0 = sin libres, usar ref_count)
 */

static void ensure_ref_table(LunaState* L) {
    if (!L->ref_table) {
        L->ref_table = luna_table_new(L);
        L->ref_free = 1;
    }
}

int luna_ref(LunaState* L, int idx) {
    LunaValue v = api_get(L, idx);
    if (v.type == LUNA_TNIL) return LUNA_REF_NONE;

    ensure_ref_table(L);
    if (!L->ref_table) return LUNA_REF_NONE;

    int ref = L->ref_free++;
    LunaValue key = luna_int((int64_t)ref);
    luna_table_set(L, L->ref_table, key, v);

    return ref;
}

void luna_unref(LunaState* L, int ref) {
    if (ref <= LUNA_REF_NONE) return;
    if (!L->ref_table) return;

    LunaValue key = luna_int((int64_t)ref);
    luna_table_remove(L->ref_table, key);
}

void luna_getref(LunaState* L, int ref) {
    if (ref <= LUNA_REF_NONE) {
        luna_pushnil(L);
        return;
    }
    if (!L->ref_table) {
        luna_pushnil(L);
        return;
    }

    LunaValue key = luna_int((int64_t)ref);
    LunaValue v;
    if (!luna_table_get(L->ref_table, key, &v)) {
        v = luna_nil();
    }

    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = v;
}