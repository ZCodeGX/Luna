#include "stdlib_internal.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static LunaString* str_arg(LunaState* L, int index) {
    LunaValue v = lib_arg(L, index);
    if (v.type != LUNA_TSTRING) {
        lib_error(L, "expected string, got %s", luna_type_name(v.type));
        return NULL;
    }
    return luna_val_string(v);
}

static int push_bytes(LunaState* L, const char* data, size_t len) {
    luna_pushlstring(L, data, len);
    return 1;
}

static char* value_text(LunaValue v, size_t* len_out) {
    char buf[64];
    switch (v.type) {
        case LUNA_TNIL:
            if (len_out) *len_out = 3;
            return lib_strdup("nil");
        case LUNA_TBOOLEAN: {
            const char* text = luna_val_bool(v) ? "true" : "false";
            if (len_out) *len_out = strlen(text);
            return lib_strdup(text);
        }
        case LUNA_TINTEGER: {
            int n = snprintf(buf, sizeof(buf), "%lld", (long long)luna_val_int(v));
            if (n < 0) n = 0;
            if (len_out) *len_out = (size_t)n;
            return lib_strdup(buf);
        }
        case LUNA_TFLOAT: {
            int n = snprintf(buf, sizeof(buf), "%.14g", luna_val_float(v));
            if (n < 0) n = 0;
            if (len_out) *len_out = (size_t)n;
            return lib_strdup(buf);
        }
        case LUNA_TSTRING: {
            LunaString* s = luna_val_string(v);
            char* copy = (char*)malloc(s->len + 1);
            if (copy) {
                memcpy(copy, s->data, s->len);
                copy[s->len] = '\0';
            }
            if (len_out) *len_out = s->len;
            return copy;
        }
        default: {
            const char* name = luna_type_name(v.type);
            if (len_out) *len_out = strlen(name);
            return lib_strdup(name);
        }
    }
}

static int s_upper(LunaState* L) {
    LunaString* s = str_arg(L, 0);
    if (!s) return -1;
    char* out = (char*)malloc(s->len ? s->len : 1);
    if (!out) return lib_error(L, "out of memory");
    for (size_t i = 0; i < s->len; i++) out[i] = (char)toupper((unsigned char)s->data[i]);
    int r = push_bytes(L, out, s->len);
    free(out);
    return r;
}

static int s_lower(LunaState* L) {
    LunaString* s = str_arg(L, 0);
    if (!s) return -1;
    char* out = (char*)malloc(s->len ? s->len : 1);
    if (!out) return lib_error(L, "out of memory");
    for (size_t i = 0; i < s->len; i++) out[i] = (char)tolower((unsigned char)s->data[i]);
    int r = push_bytes(L, out, s->len);
    free(out);
    return r;
}

static int s_concat(LunaState* L) {
    size_t total = 0;
    char** parts = (char**)calloc((size_t)(lib_nargs(L) > 0 ? lib_nargs(L) : 1), sizeof(char*));
    size_t* lens = (size_t*)calloc((size_t)(lib_nargs(L) > 0 ? lib_nargs(L) : 1), sizeof(size_t));
    if (!parts || !lens) {
        free(parts);
        free(lens);
        return lib_error(L, "out of memory");
    }

    int n = lib_nargs(L);
    for (int i = 0; i < n; i++) {
        parts[i] = value_text(lib_arg(L, i), &lens[i]);
        if (!parts[i]) {
            for (int j = 0; j < i; j++) free(parts[j]);
            free(parts);
            free(lens);
            return lib_error(L, "out of memory");
        }
        total += lens[i];
    }

    char* out = (char*)malloc(total ? total : 1);
    if (!out) {
        for (int i = 0; i < n; i++) free(parts[i]);
        free(parts);
        free(lens);
        return lib_error(L, "out of memory");
    }

    size_t at = 0;
    for (int i = 0; i < n; i++) {
        memcpy(out + at, parts[i], lens[i]);
        at += lens[i];
        free(parts[i]);
    }
    free(parts);
    free(lens);
    int r = push_bytes(L, out, total);
    free(out);
    return r;
}

static int s_find(LunaState* L) {
    LunaString* s = str_arg(L, 0);
    LunaString* needle = str_arg(L, 1);
    if (!s || !needle) return -1;
    if (needle->len == 0) {
        luna_pushinteger(L, 0);
        return 1;
    }
    for (size_t i = 0; i + needle->len <= s->len; i++) {
        if (memcmp(s->data + i, needle->data, needle->len) == 0) {
            luna_pushinteger(L, (int64_t)i);
            return 1;
        }
    }
    luna_pushinteger(L, -1);
    return 1;
}

static int s_contains(LunaState* L) {
    if (s_find(L) != 1) return -1;
    int64_t pos = luna_tointeger(L, -1, NULL);
    luna_pop(L, 1);
    luna_pushboolean(L, pos >= 0);
    return 1;
}

static int s_starts(LunaState* L) {
    LunaString* s = str_arg(L, 0);
    LunaString* p = str_arg(L, 1);
    if (!s || !p) return -1;
    luna_pushboolean(L, p->len <= s->len && memcmp(s->data, p->data, p->len) == 0);
    return 1;
}

static int s_ends(LunaState* L) {
    LunaString* s = str_arg(L, 0);
    LunaString* p = str_arg(L, 1);
    if (!s || !p) return -1;
    luna_pushboolean(L, p->len <= s->len &&
                         memcmp(s->data + s->len - p->len, p->data, p->len) == 0);
    return 1;
}

static int s_trim(LunaState* L) {
    LunaString* s = str_arg(L, 0);
    if (!s) return -1;
    LunaString* chars = NULL;
    if (lib_nargs(L) > 1) {
        chars = str_arg(L, 1);
        if (!chars) return -1;
    }

    size_t start = 0, end = s->len;
    while (start < end) {
        char c = s->data[start];
        int match = chars ? memchr(chars->data, c, chars->len) != NULL : isspace((unsigned char)c);
        if (!match) break;
        start++;
    }
    while (end > start) {
        char c = s->data[end - 1];
        int match = chars ? memchr(chars->data, c, chars->len) != NULL : isspace((unsigned char)c);
        if (!match) break;
        end--;
    }
    return push_bytes(L, s->data + start, end - start);
}

static int s_split(LunaState* L) {
    LunaString* s = str_arg(L, 0);
    LunaString* sep = str_arg(L, 1);
    if (!s || !sep) return -1;
    if (sep->len == 0) return lib_error(L, "separator cannot be empty");

    LunaList* out = luna_list_new(L);
    if (!out) return lib_error(L, "out of memory");

    size_t start = 0;
    for (size_t i = 0; i + sep->len <= s->len;) {
        if (memcmp(s->data + i, sep->data, sep->len) != 0) {
            i++;
            continue;
        }
        luna_list_push_value(L, out, lib_value_from_string(L, s->data + start, i - start));
        i += sep->len;
        start = i;
    }
    luna_list_push_value(L, out, lib_value_from_string(L, s->data + start, s->len - start));
    lib_push_value(L, luna_list_val(out));
    return 1;
}

static int s_join(LunaState* L) {
    LunaString* sep = str_arg(L, 0);
    LunaValue listv = lib_arg(L, 1);
    if (!sep) return -1;
    if (listv.type != LUNA_TLIST) return lib_error(L, "expected list, got %s",
                                                   luna_type_name(listv.type));
    LunaList* list = luna_val_list(listv);

    size_t total = 0;
    if (list->length) total = (list->length - 1) * sep->len;
    char** parts = (char**)calloc(list->length ? list->length : 1, sizeof(char*));
    size_t* lens = (size_t*)calloc(list->length ? list->length : 1, sizeof(size_t));
    if (!parts || !lens) {
        free(parts);
        free(lens);
        return lib_error(L, "out of memory");
    }

    for (size_t i = 0; i < list->length; i++) {
        parts[i] = value_text(list->items[i], &lens[i]);
        if (!parts[i]) {
            for (size_t j = 0; j < i; j++) free(parts[j]);
            free(parts);
            free(lens);
            return lib_error(L, "out of memory");
        }
        total += lens[i];
    }

    char* out = (char*)malloc(total ? total : 1);
    if (!out) {
        for (size_t i = 0; i < list->length; i++) free(parts[i]);
        free(parts);
        free(lens);
        return lib_error(L, "out of memory");
    }

    size_t at = 0;
    for (size_t i = 0; i < list->length; i++) {
        if (i) {
            memcpy(out + at, sep->data, sep->len);
            at += sep->len;
        }
        memcpy(out + at, parts[i], lens[i]);
        at += lens[i];
        free(parts[i]);
    }
    free(parts);
    free(lens);
    int r = push_bytes(L, out, total);
    free(out);
    return r;
}

static int s_replace(LunaState* L) {
    LunaString* s = str_arg(L, 0);
    LunaString* from = str_arg(L, 1);
    LunaString* to = str_arg(L, 2);
    if (!s || !from || !to) return -1;
    if (from->len == 0) return lib_error(L, "pattern cannot be empty");

    size_t cap = s->len + 1, len = 0;
    char* out = (char*)malloc(cap);
    if (!out) return lib_error(L, "out of memory");

    for (size_t i = 0; i < s->len;) {
        size_t need = len + (s->len - i) + to->len + 1;
        if (need > cap) {
            while (cap < need) cap *= 2;
            char* tmp = (char*)realloc(out, cap);
            if (!tmp) {
                free(out);
                return lib_error(L, "out of memory");
            }
            out = tmp;
        }
        if (i + from->len <= s->len && memcmp(s->data + i, from->data, from->len) == 0) {
            memcpy(out + len, to->data, to->len);
            len += to->len;
            i += from->len;
        } else {
            out[len++] = s->data[i++];
        }
    }
    int r = push_bytes(L, out, len);
    free(out);
    return r;
}

static int s_repeat(LunaState* L) {
    LunaString* s = str_arg(L, 0);
    LunaValue nv = lib_arg(L, 1);
    if (!s) return -1;
    if (nv.type != LUNA_TINTEGER || luna_val_int(nv) < 0)
        return lib_error(L, "expected non-negative integer, got %s", luna_type_name(nv.type));
    size_t n = (size_t)luna_val_int(nv);
    if (n && s->len > (size_t)-1 / n) return lib_error(L, "result too large");

    size_t total = s->len * n;
    char* out = (char*)malloc(total ? total : 1);
    if (!out) return lib_error(L, "out of memory");
    for (size_t i = 0; i < n; i++) memcpy(out + i * s->len, s->data, s->len);
    int r = push_bytes(L, out, total);
    free(out);
    return r;
}

static int s_reverse(LunaState* L) {
    LunaString* s = str_arg(L, 0);
    if (!s) return -1;
    char* out = (char*)malloc(s->len ? s->len : 1);
    if (!out) return lib_error(L, "out of memory");
    for (size_t i = 0; i < s->len; i++) out[i] = s->data[s->len - 1 - i];
    int r = push_bytes(L, out, s->len);
    free(out);
    return r;
}

static int s_sub(LunaState* L) {
    LunaString* s = str_arg(L, 0);
    LunaValue av = lib_arg(L, 1);
    if (!s) return -1;
    if (av.type != LUNA_TINTEGER) return lib_error(L, "expected integer index");
    int64_t start = luna_val_int(av);
    if (start < 0) start += (int64_t)s->len;
    if (start < 0) start = 0;
    if ((size_t)start > s->len) start = (int64_t)s->len;

    size_t end = s->len;
    if (lib_nargs(L) > 2) {
        LunaValue bv = lib_arg(L, 2);
        if (bv.type != LUNA_TINTEGER) return lib_error(L, "expected integer index");
        int64_t e = luna_val_int(bv);
        if (e < 0) e += (int64_t)s->len;
        if (e < 0) e = 0;
        if ((size_t)e < (size_t)start) e = start;
        if ((size_t)e > s->len) e = (int64_t)s->len;
        end = (size_t)e;
    }
    return push_bytes(L, s->data + start, end - (size_t)start);
}

static void register_functions(LunaState* L, LunaTable* exports) {
    lib_export_function(L, exports, "len", lib_builtin_len);
    lib_export_function(L, exports, "upper", s_upper);
    lib_export_function(L, exports, "lower", s_lower);
    lib_export_function(L, exports, "concat", s_concat);
    lib_export_function(L, exports, "find", s_find);
    lib_export_function(L, exports, "contains", s_contains);
    lib_export_function(L, exports, "starts_with", s_starts);
    lib_export_function(L, exports, "ends_with", s_ends);
    lib_export_function(L, exports, "trim", s_trim);
    lib_export_function(L, exports, "split", s_split);
    lib_export_function(L, exports, "join", s_join);
    lib_export_function(L, exports, "replace", s_replace);
    lib_export_function(L, exports, "repeat", s_repeat);
    lib_export_function(L, exports, "reverse", s_reverse);
    lib_export_function(L, exports, "sub", s_sub);
}

static const LunaModule g_module = { "string_native", register_functions, NULL };

void luna_open_string_lib(LunaState* L) {
    luna_register_module(L, &g_module);
}
