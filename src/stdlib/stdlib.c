#include "stdlib_internal.h"
#include "../platform/platform.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LUNA_STDLIB_DIR
#define LUNA_STDLIB_DIR "Lib"
#endif

int lib_nargs(LunaState* L) {
    LunaVM* vm = state_vm(L);
    if (!vm || vm->frame_top == 0) return 0;
    return vm->frames[vm->frame_top - 1].num_args;
}

LunaValue lib_arg(LunaState* L, int i) {
    LunaVM* vm = state_vm(L);
    if (!vm || vm->frame_top == 0) return luna_nil();
    CallFrame* frame = &vm->frames[vm->frame_top - 1];
    if (i < 0 || i >= frame->num_args) return luna_nil();
    return frame->base[1 + (size_t)i];
}

int lib_error(LunaState* L, const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    luna_pushstring(L, buf);
    return -1;
}

char* lib_strdup(const char* s) {
    size_t len = strlen(s);
    char* copy = (char*)malloc(len + 1);
    if (copy) memcpy(copy, s, len + 1);
    return copy;
}

void lib_push_value(LunaState* L, LunaValue value) {
    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = value;
}

LunaValue lib_value_from_string(LunaState* L, const char* s, size_t len) {
    LunaString* str = luna_string_new(L, s, len);
    return str ? luna_string_val(str) : luna_nil();
}

static void export_value(LunaState* L, LunaTable* exports, const char* name,
                         LunaValue value) {
    if (!exports) return;
    LunaString* key = luna_string_newz(L, name);
    if (!key) return;
    luna_table_set(L, exports, luna_string_val(key), value);
}

void lib_export_function(LunaState* L, LunaTable* exports, const char* name,
                         int (*fn)(LunaState* L)) {
    if (!fn) return;
    LunaFunction* f = luna_function_new_c(L, fn);
    if (!f) return;
    export_value(L, exports, name, luna_function_val(f));
}

void lib_export_float(LunaState* L, LunaTable* exports, const char* name, double value) {
    export_value(L, exports, name, luna_float(value));
}

void lib_export_string(LunaState* L, LunaTable* exports, const char* name,
                       const char* value) {
    if (!value) return;
    export_value(L, exports, name, lib_value_from_string(L, value, strlen(value)));
}

static char* value_to_text(LunaState* L, LunaValue v, size_t* len_out) {
    char buf[64];
    switch (v.type) {
        case LUNA_TNIL:
            return lib_strdup("nil");
        case LUNA_TBOOLEAN:
            return lib_strdup(luna_val_bool(v) ? "true" : "false");
        case LUNA_TINTEGER:
            snprintf(buf, sizeof(buf), "%lld", (long long)luna_val_int(v));
            return lib_strdup(buf);
        case LUNA_TFLOAT:
            snprintf(buf, sizeof(buf), "%.14g", luna_val_float(v));
            return lib_strdup(buf);
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

int lib_builtin_len(LunaState* L) {
    LunaValue v = lib_arg(L, 0);
    switch (v.type) {
        case LUNA_TSTRING:
            luna_pushinteger(L, (int64_t)luna_string_len(luna_val_string(v)));
            return 1;
        case LUNA_TLIST:
            luna_pushinteger(L, (int64_t)luna_list_len(luna_val_list(v)));
            return 1;
        case LUNA_TTABLE:
            luna_pushinteger(L, (int64_t)luna_table_count(luna_val_table(v)));
            return 1;
        default:
            return lib_error(L, "cannot get length of a %s", luna_type_name(v.type));
    }
}

static int builtin_type(LunaState* L) {
    LunaValue v = lib_arg(L, 0);
    const char* name = luna_type_name(v.type);
    luna_pushlstring(L, name, strlen(name));
    return 1;
}

static int builtin_str(LunaState* L) {
    size_t len = 0;
    char* text = value_to_text(L, lib_arg(L, 0), &len);
    if (!text) return lib_error(L, "out of memory");
    if (!len) len = strlen(text);
    luna_pushlstring(L, text, len);
    free(text);
    return 1;
}

static int builtin_int(LunaState* L) {
    LunaValue v = lib_arg(L, 0);
    switch (v.type) {
        case LUNA_TINTEGER:
            luna_pushinteger(L, luna_val_int(v));
            return 1;
        case LUNA_TFLOAT:
            luna_pushinteger(L, (int64_t)luna_val_float(v));
            return 1;
        case LUNA_TBOOLEAN:
            luna_pushinteger(L, luna_val_bool(v) ? 1 : 0);
            return 1;
        case LUNA_TSTRING: {
            LunaString* s = luna_val_string(v);
            char* tmp = (char*)malloc(s->len + 1);
            if (!tmp) return lib_error(L, "out of memory");
            memcpy(tmp, s->data, s->len);
            tmp[s->len] = '\0';
            errno = 0;
            char* end = NULL;
            long long n = strtoll(tmp, &end, 0);
            if (errno != 0 || end == tmp || *end != '\0') {
                int r = lib_error(L, "cannot convert '%s' to integer", tmp);
                free(tmp);
                return r;
            }
            free(tmp);
            luna_pushinteger(L, (int64_t)n);
            return 1;
        }
        default:
            return lib_error(L, "cannot convert %s to integer", luna_type_name(v.type));
    }
}

static int builtin_float(LunaState* L) {
    LunaValue v = lib_arg(L, 0);
    switch (v.type) {
        case LUNA_TINTEGER:
            luna_pushfloat(L, (double)luna_val_int(v));
            return 1;
        case LUNA_TFLOAT:
            luna_pushfloat(L, luna_val_float(v));
            return 1;
        case LUNA_TBOOLEAN:
            luna_pushfloat(L, luna_val_bool(v) ? 1.0 : 0.0);
            return 1;
        case LUNA_TSTRING: {
            LunaString* s = luna_val_string(v);
            char* tmp = (char*)malloc(s->len + 1);
            if (!tmp) return lib_error(L, "out of memory");
            memcpy(tmp, s->data, s->len);
            tmp[s->len] = '\0';
            errno = 0;
            char* end = NULL;
            double d = strtod(tmp, &end);
            if (errno != 0 || end == tmp || *end != '\0') {
                int r = lib_error(L, "cannot convert '%s' to float", tmp);
                free(tmp);
                return r;
            }
            free(tmp);
            luna_pushfloat(L, d);
            return 1;
        }
        default:
            return lib_error(L, "cannot convert %s to float", luna_type_name(v.type));
    }
}

static int builtin_input(LunaState* L) {
    if (lib_nargs(L) > 0) {
        LunaValue prompt = lib_arg(L, 0);
        if (prompt.type == LUNA_TSTRING) {
            LunaString* s = luna_val_string(prompt);
            fwrite(s->data, 1, s->len, stdout);
        }
        fflush(stdout);
    }

    size_t cap = 256, len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) return lib_error(L, "out of memory");

    int c;
    while ((c = getchar()) != EOF && c != '\n') {
        if (len + 1 >= cap) {
            size_t new_cap = cap * 2;
            char* tmp = (char*)realloc(buf, new_cap);
            if (!tmp) {
                free(buf);
                return lib_error(L, "out of memory");
            }
            buf = tmp;
            cap = new_cap;
        }
        buf[len++] = (char)c;
    }
    if (len > 0 && buf[len - 1] == '\r') len--;
    luna_pushlstring(L, buf, len);
    free(buf);
    return 1;
}

static void add_stdlib_path(LunaState* L, const char* dir) {
    if (!dir || !dir[0]) return;
    if (!luna_platform_is_directory(dir)) return;
    luna_add_module_path(L, dir);
}

static void add_stdlib_path_list(LunaState* L, const char* list) {
    if (!list || !list[0]) return;

    char* copy = (char*)malloc(strlen(list) + 1);
    if (!copy) return;
    strcpy(copy, list);

#ifdef _WIN32
    const char* seps = ";";
#else
    const char* seps = ":";
#endif

    for (char* item = strtok(copy, seps); item; item = strtok(NULL, seps)) {
        add_stdlib_path(L, item);
        char* nested = luna_platform_path_join(item, "Lib");
        if (nested) {
            add_stdlib_path(L, nested);
            free(nested);
        }
    }

    free(copy);
}

static void discover_stdlib_paths(LunaState* L) {
    const char* home = luna_platform_getenv("LUNA_HOME");
    if (home && home[0]) {
        char* lib = luna_platform_path_join(home, "Lib");
        if (lib) {
            add_stdlib_path(L, lib);
            free(lib);
        }
        char* share = luna_platform_path_join(home, "share/luna/Lib");
        if (share) {
            add_stdlib_path(L, share);
            free(share);
        }
    }

    add_stdlib_path_list(L, luna_platform_getenv("LUNA_PATH"));

    char* exe = luna_platform_executable_path();
    if (exe) {
        char* bin_dir = luna_platform_path_dirname(exe);
        if (bin_dir) {
            char* sibling = luna_platform_path_join(bin_dir, "Lib");
            if (sibling) {
                add_stdlib_path(L, sibling);
                free(sibling);
            }

            char* parent = luna_platform_path_dirname(bin_dir);
            if (parent) {
                char* from_parent = luna_platform_path_join(parent, "Lib");
                if (from_parent) {
                    add_stdlib_path(L, from_parent);
                    free(from_parent);
                }
                char* from_share = luna_platform_path_join(parent, "share/luna/Lib");
                if (from_share) {
                    add_stdlib_path(L, from_share);
                    free(from_share);
                }
                free(parent);
            }
            free(bin_dir);
        }
        free(exe);
    }

    add_stdlib_path(L, LUNA_STDLIB_DIR);
}

void luna_open_stdlib(LunaState* L) {
    discover_stdlib_paths(L);

    luna_register_function(L, "len", lib_builtin_len);
    luna_register_function(L, "type", builtin_type);
    luna_register_function(L, "str", builtin_str);
    luna_register_function(L, "int", builtin_int);
    luna_register_function(L, "float", builtin_float);
    luna_register_function(L, "input", builtin_input);

    luna_open_math_lib(L);
    luna_open_string_lib(L);
    luna_open_collection_lib(L);
    luna_open_system_lib(L);
    luna_open_ffi_lib(L);
    luna_open_http_lib(L);
}
