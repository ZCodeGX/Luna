#include "api_internal.h"

void luna_newtable(LunaState* L) {
    LunaTable* t = luna_table_new(L);
    if (!t) {
        luna_error_set_simple(L, LUNA_ERR_MEMORY, "out of memory");
        return;
    }
    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = luna_table_val(t);
}

void luna_newlist(LunaState* L) {
    LunaList* list = luna_list_new(L);
    if (!list) {
        luna_error_set_simple(L, LUNA_ERR_MEMORY, "out of memory");
        return;
    }
    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = luna_list_val(list);
}

void luna_gettable(LunaState* L, int idx) {
    /* pop key, push value */
    if (L->stack_top == 0) return;
    LunaValue key = L->stack[--L->stack_top];
    L->stack[L->stack_top] = luna_nil();

    LunaValue obj = api_get(L, idx);
    LunaValue result = luna_nil();

    if (obj.type == LUNA_TTABLE) {
        LunaTable* t = luna_val_table(obj);
        if (!luna_table_get(t, key, &result)) {
            result = luna_nil();
        }
    } else if (obj.type == LUNA_TLIST) {
        LunaList* list = luna_val_list(obj);
        if (key.type == LUNA_TINTEGER) {
            int64_t i = key.as.integer;
            if (i >= 0 && (size_t)i < list->length) {
                result = list->items[i];
            }
        }
    }

    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = result;
}

void luna_settable(LunaState* L, int idx) {
    /* pop key y value */
    if (L->stack_top < 2) return;
    LunaValue value = L->stack[--L->stack_top];
    LunaValue key   = L->stack[--L->stack_top];
    L->stack[L->stack_top] = luna_nil();
    L->stack[L->stack_top + 1] = luna_nil();

    LunaValue obj = api_get(L, idx);

    if (obj.type == LUNA_TTABLE) {
        luna_table_set(L, luna_val_table(obj), key, value);
    } else if (obj.type == LUNA_TLIST) {
        LunaList* list = luna_val_list(obj);
        if (key.type == LUNA_TINTEGER) {
            int64_t i = key.as.integer;
            if (i >= 0 && (size_t)i < list->length) {
                list->items[i] = value;
            }
        }
    }
}

void luna_getfield(LunaState* L, int idx, const char* k) {
    LunaValue obj = api_get(L, idx);
    LunaValue result = luna_nil();

    if (obj.type == LUNA_TTABLE) {
        LunaString* key = luna_string_newz(L, k);
        if (key) {
            LunaTable* t = luna_val_table(obj);
            if (!luna_table_get(t, luna_string_val(key), &result)) {
                result = luna_nil();
            }
        }
    }

    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = result;
}

void luna_setfield(LunaState* L, int idx, const char* k) {
    if (L->stack_top == 0) return;
    LunaValue value = L->stack[--L->stack_top];
    L->stack[L->stack_top] = luna_nil();

    LunaValue obj = api_get(L, idx);
    if (obj.type == LUNA_TTABLE) {
        LunaString* key = luna_string_newz(L, k);
        if (key) {
            luna_table_set(L, luna_val_table(obj), luna_string_val(key), value);
        }
    }
}

void luna_getindex(LunaState* L, int idx, int64_t i) {
    LunaValue obj = api_get(L, idx);
    LunaValue result = luna_nil();

    if (obj.type == LUNA_TLIST) {
        LunaList* list = luna_val_list(obj);
        if (i >= 0 && (size_t)i < list->length) {
            result = list->items[i];
        }
    } else if (obj.type == LUNA_TTABLE) {
        LunaValue key = luna_int(i);
        if (!luna_table_get(luna_val_table(obj), key, &result)) {
            result = luna_nil();
        }
    }

    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = result;
}

void luna_setindex(LunaState* L, int idx, int64_t i) {
    if (L->stack_top == 0) return;
    LunaValue value = L->stack[--L->stack_top];
    L->stack[L->stack_top] = luna_nil();

    LunaValue obj = api_get(L, idx);
    if (obj.type == LUNA_TLIST) {
        LunaList* list = luna_val_list(obj);
        if (i >= 0 && (size_t)i < list->length) {
            list->items[i] = value;
        }
    } else if (obj.type == LUNA_TTABLE) {
        luna_table_set(L, luna_val_table(obj), luna_int(i), value);
    }
}

int64_t luna_len(LunaState* L, int idx) {
    LunaValue v = api_get(L, idx);
    switch (v.type) {
        case LUNA_TSTRING: return (int64_t)luna_val_string(v)->len;
        case LUNA_TLIST:   return (int64_t)luna_val_list(v)->length;
        case LUNA_TTABLE:  return (int64_t)luna_table_count(luna_val_table(v));
        default: return 0;
    }
}

void luna_list_push(LunaState* L, int idx) {
    if (L->stack_top == 0) return;
    LunaValue value = L->stack[--L->stack_top];
    L->stack[L->stack_top] = luna_nil();

    LunaValue obj = api_get(L, idx);
    if (obj.type == LUNA_TLIST) {
        luna_list_push_value(L, luna_val_list(obj), value);
    }
}