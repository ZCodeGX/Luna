#include "stdlib_internal.h"

#include <stdlib.h>

static LunaList* list_arg(LunaState* L, int index) {
    LunaValue v = lib_arg(L, index);
    if (v.type != LUNA_TLIST) {
        lib_error(L, "expected list, got %s", luna_type_name(v.type));
        return NULL;
    }
    return luna_val_list(v);
}

static LunaTable* table_arg(LunaState* L, int index) {
    LunaValue v = lib_arg(L, index);
    if (v.type != LUNA_TTABLE) {
        lib_error(L, "expected table, got %s", luna_type_name(v.type));
        return NULL;
    }
    return luna_val_table(v);
}

static int l_new(LunaState* L) {
    LunaList* list = luna_list_new(L);
    if (!list) return lib_error(L, "out of memory");
    int n = lib_nargs(L);
    for (int i = 0; i < n; i++) luna_list_push_value(L, list, lib_arg(L, i));
    lib_push_value(L, luna_list_val(list));
    return 1;
}

static int l_push(LunaState* L) {
    LunaList* list = list_arg(L, 0);
    if (!list) return -1;
    luna_list_push_value(L, list, lib_arg(L, 1));
    luna_pushnil(L);
    return 1;
}

static int l_pop(LunaState* L) {
    LunaList* list = list_arg(L, 0);
    if (!list) return -1;
    if (list->length == 0) {
        luna_pushnil(L);
        return 1;
    }
    LunaValue v = list->items[list->length - 1];
    list->length--;
    lib_push_value(L, v);
    return 1;
}

static int l_get(LunaState* L) {
    LunaList* list = list_arg(L, 0);
    LunaValue iv = lib_arg(L, 1);
    if (!list) return -1;
    if (iv.type != LUNA_TINTEGER) return lib_error(L, "expected integer index");
    int64_t i = luna_val_int(iv);
    if (i < 0) i += (int64_t)list->length;
    LunaValue out;
    if (i < 0 || !luna_list_get(list, (size_t)i, &out)) {
        luna_pushnil(L);
        return 1;
    }
    lib_push_value(L, out);
    return 1;
}

static int l_set(LunaState* L) {
    LunaList* list = list_arg(L, 0);
    LunaValue iv = lib_arg(L, 1);
    if (!list) return -1;
    if (iv.type != LUNA_TINTEGER) return lib_error(L, "expected integer index");
    int64_t i = luna_val_int(iv);
    if (i < 0) i += (int64_t)list->length;
    if (i < 0 || (size_t)i >= list->length) {
        luna_pushboolean(L, 0);
        return 1;
    }
    luna_list_set(L, list, (size_t)i, lib_arg(L, 2));
    luna_pushboolean(L, 1);
    return 1;
}

static int l_clear(LunaState* L) {
    LunaList* list = list_arg(L, 0);
    if (!list) return -1;
    list->length = 0;
    luna_pushnil(L);
    return 1;
}

static int l_reverse(LunaState* L) {
    LunaList* list = list_arg(L, 0);
    if (!list) return -1;
    for (size_t i = 0; i < list->length / 2; i++) {
        size_t j = list->length - 1 - i;
        LunaValue tmp = list->items[i];
        list->items[i] = list->items[j];
        list->items[j] = tmp;
    }
    luna_pushnil(L);
    return 1;
}

static int l_contains(LunaState* L) {
    LunaList* list = list_arg(L, 0);
    if (!list) return -1;
    LunaValue needle = lib_arg(L, 1);
    for (size_t i = 0; i < list->length; i++) {
        if (luna_value_equal(list->items[i], needle)) {
            luna_pushboolean(L, 1);
            return 1;
        }
    }
    luna_pushboolean(L, 0);
    return 1;
}

static int t_new(LunaState* L) {
    LunaTable* table = luna_table_new(L);
    if (!table) return lib_error(L, "out of memory");
    lib_push_value(L, luna_table_val(table));
    return 1;
}

static int table_key(LunaState* L, LunaValue v, LunaValue* out) {
    if (v.type == LUNA_TSTRING || v.type == LUNA_TINTEGER) {
        *out = v;
        return 0;
    }
    return lib_error(L, "table key must be string or integer");
}

static int t_get(LunaState* L) {
    LunaTable* table = table_arg(L, 0);
    LunaValue key;
    if (!table || table_key(L, lib_arg(L, 1), &key) != 0) return -1;
    LunaValue out;
    if (!luna_table_get(table, key, &out)) luna_pushnil(L);
    else lib_push_value(L, out);
    return 1;
}

static int t_set(LunaState* L) {
    LunaTable* table = table_arg(L, 0);
    LunaValue key;
    if (!table || table_key(L, lib_arg(L, 1), &key) != 0) return -1;
    luna_table_set(L, table, key, lib_arg(L, 2));
    luna_pushnil(L);
    return 1;
}

static int t_has(LunaState* L) {
    LunaTable* table = table_arg(L, 0);
    LunaValue key;
    if (!table || table_key(L, lib_arg(L, 1), &key) != 0) return -1;
    LunaValue out;
    luna_pushboolean(L, luna_table_get(table, key, &out));
    return 1;
}

static int t_remove(LunaState* L) {
    LunaTable* table = table_arg(L, 0);
    LunaValue key;
    if (!table || table_key(L, lib_arg(L, 1), &key) != 0) return -1;
    luna_table_remove(table, key);
    luna_pushnil(L);
    return 1;
}

static int t_keys(LunaState* L) {
    LunaTable* table = table_arg(L, 0);
    if (!table) return -1;
    LunaList* keys = luna_list_new(L);
    if (!keys) return lib_error(L, "out of memory");
    for (uint32_t i = 0; i < table->size; i++) {
        LunaTableNode* n = &table->nodes[i];
        if (n->key.type != LUNA_TNIL) luna_list_push_value(L, keys, n->key);
    }
    lib_push_value(L, luna_list_val(keys));
    return 1;
}

static void register_list(LunaState* L, LunaTable* exports) {
    lib_export_function(L, exports, "new", l_new);
    lib_export_function(L, exports, "push", l_push);
    lib_export_function(L, exports, "pop", l_pop);
    lib_export_function(L, exports, "get", l_get);
    lib_export_function(L, exports, "set", l_set);
    lib_export_function(L, exports, "len", lib_builtin_len);
    lib_export_function(L, exports, "clear", l_clear);
    lib_export_function(L, exports, "reverse", l_reverse);
    lib_export_function(L, exports, "contains", l_contains);
}

static void register_table(LunaState* L, LunaTable* exports) {
    lib_export_function(L, exports, "new", t_new);
    lib_export_function(L, exports, "get", t_get);
    lib_export_function(L, exports, "set", t_set);
    lib_export_function(L, exports, "has", t_has);
    lib_export_function(L, exports, "remove", t_remove);
    lib_export_function(L, exports, "keys", t_keys);
    lib_export_function(L, exports, "len", lib_builtin_len);
}

static const LunaModule g_list = { "list_native", register_list, NULL };
static const LunaModule g_table = { "table_native", register_table, NULL };

void luna_open_collection_lib(LunaState* L) {
    luna_register_module(L, &g_list);
    luna_register_module(L, &g_table);
}
