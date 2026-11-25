#include "stdlib_internal.h"
#include "../vm/vm_internal.h"
#include "../platform/platform.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FFI_MAX_ARGS 8

typedef enum {
    FFI_TVOID = 0,
    FFI_TINT,
    FFI_TFLOAT,
    FFI_TSTR,
    FFI_TPTR
} FfiType;

typedef struct {
    LunaDynLib lib;
    LunaDynSym sym;
    char* binding;
    char* symbol;
    char* library;
    FfiType ret;
    int nargs;
    FfiType args[FFI_MAX_ARGS];
} FfiHandle;

typedef struct {
    LunaDynLib lib;
    char* path;
} FfiLibrary;

static int ffi_call(LunaState* L);

static char* ffi_strdup(const char* s) {
    size_t n = strlen(s) + 1;
    char* out = (char*)malloc(n);
    if (out) memcpy(out, s, n);
    return out;
}

static void ffi_handle_finalizer(LunaState* L, void* ptr) {
    (void)L;
    FfiHandle* h = (FfiHandle*)ptr;
    if (!h) return;
    if (h->lib) luna_platform_dlclose(h->lib);
    free(h->binding);
    free(h->symbol);
    free(h->library);
    memset(h, 0, sizeof(*h));
}

static void ffi_library_finalizer(LunaState* L, void* ptr) {
    (void)L;
    FfiLibrary* lib = (FfiLibrary*)ptr;
    if (!lib) return;
    if (lib->lib) luna_platform_dlclose(lib->lib);
    free(lib->path);
    lib->lib = NULL;
    lib->path = NULL;
}

static int ffi_parse_type(const char* s, size_t len, FfiType* out) {
    if (len == 4 && memcmp(s, "void", 4) == 0) { *out = FFI_TVOID; return 0; }
    if (len == 3 && memcmp(s, "int", 3) == 0) { *out = FFI_TINT; return 0; }
    if (len == 3 && memcmp(s, "i64", 3) == 0) { *out = FFI_TINT; return 0; }
    if (len == 3 && memcmp(s, "str", 3) == 0) { *out = FFI_TSTR; return 0; }
    if (len == 3 && memcmp(s, "ptr", 3) == 0) { *out = FFI_TPTR; return 0; }
    if (len == 3 && memcmp(s, "f64", 3) == 0) { *out = FFI_TFLOAT; return 0; }
    if (len == 5 && memcmp(s, "float", 5) == 0) { *out = FFI_TFLOAT; return 0; }
    if (len == 6 && memcmp(s, "double", 6) == 0) { *out = FFI_TFLOAT; return 0; }
    if (len == 6 && memcmp(s, "string", 6) == 0) { *out = FFI_TSTR; return 0; }
    if (len == 7 && memcmp(s, "pointer", 7) == 0) { *out = FFI_TPTR; return 0; }
    if (len == 4 && memcmp(s, "cstr", 4) == 0) { *out = FFI_TSTR; return 0; }
    return -1;
}

static int ffi_parse_signature(const char* sig, FfiHandle* h) {
    if (!sig) return -1;
    const char* open = strchr(sig, '(');
    const char* close = strrchr(sig, ')');
    if (!open || !close || close < open || close[1] != '\0') return -1;

    const char* p = sig;
    while (p < open && (*p == ' ' || *p == '\t')) p++;
    if (ffi_parse_type(p, (size_t)(open - p), &h->ret) != 0) return -1;

    p = open + 1;
    while (p < close) {
        while (p < close && (*p == ' ' || *p == '\t' || *p == ',')) p++;
        if (p >= close) break;
        const char* start = p;
        while (p < close && *p != ',' && *p != ' ' && *p != '\t') p++;
        if (h->nargs >= FFI_MAX_ARGS) return -1;
        if (ffi_parse_type(start, (size_t)(p - start), &h->args[h->nargs]) != 0) return -1;
        if (h->args[h->nargs] == FFI_TVOID) return -1;
        h->nargs++;
    }
    return 0;
}

static int ffi_is_self(const char* path) {
    if (!path || !path[0]) return 1;
    if (strcmp(path, "c") == 0 || strcmp(path, "libc") == 0 ||
        strcmp(path, "self") == 0 || strcmp(path, "current") == 0) return 1;
    return 0;
}

static LunaDynLib ffi_load_library(LunaState* L, const char* path) {
    char error[256] = {0};
    LunaDynLib handle = NULL;
    const char* candidates[4] = {NULL, NULL, NULL, NULL};
    int count = 0;

    if (ffi_is_self(path)) {
        candidates[count++] = NULL;
    } else if (strcmp(path, "m") == 0 || strcmp(path, "libm") == 0) {
        candidates[count++] = "libm.so.6";
        candidates[count++] = "libm.so";
        candidates[count++] = NULL;
    } else {
        candidates[count++] = path;
        char with_ext[512];
        snprintf(with_ext, sizeof(with_ext), "%s%s", path, luna_platform_dl_extension());
        candidates[count++] = with_ext;
    }

    for (int i = 0; i < count; i++) {
        error[0] = '\0';
        handle = luna_platform_dlopen(candidates[i], error, sizeof(error));
        if (handle) return handle;
    }

    lib_error(L, "ffi: cannot load '%s': %s", path ? path : "current process",
              error[0] ? error : "unknown error");
    return NULL;
}

static const char* ffi_arg_string(LunaState* L, int i, size_t* len) {
    LunaValue v = lib_arg(L, i);
    if (v.type != LUNA_TSTRING) {
        lib_error(L, "ffi: argument %d must be a string", i + 1);
        return NULL;
    }
    LunaString* s = luna_val_string(v);
    if (len) *len = s->len;
    return s->data;
}

static FfiHandle* ffi_frame_handle(LunaState* L) {
    LunaVM* vm = state_vm(L);
    if (!vm || vm->frame_top == 0) return NULL;
    CallFrame* frame = &vm->frames[vm->frame_top - 1];
    if (!frame->func || frame->func->num_upvalues == 0) return NULL;
    LunaUpvalue* uv = frame->func->upvalues[0];
    if (!uv) return NULL;
    LunaValue v = uv->location ? *uv->location : uv->closed;
    if (v.type != LUNA_TUSERDATA) return NULL;
    return (FfiHandle*)luna_userdata_data(luna_val_userdata(v));
}

static int ffi_declare_impl(LunaState* L, int name_idx, int symbol_idx,
                            int library_idx, int signature_idx) {
    size_t name_len = 0, symbol_len = 0, library_len = 0, signature_len = 0;
    const char* name = ffi_arg_string(L, name_idx, &name_len);
    if (!name) return -1;
    const char* symbol = symbol_idx == name_idx
        ? name
        : ffi_arg_string(L, symbol_idx, &symbol_len);
    if (!symbol) return -1;
    if (symbol_idx == name_idx) symbol_len = name_len;
    const char* library = ffi_arg_string(L, library_idx, &library_len);
    if (!library) return -1;
    const char* signature = ffi_arg_string(L, signature_idx, &signature_len);
    if (!signature) return -1;

    char* name_buf = (char*)malloc(name_len + 1);
    char* symbol_buf = (char*)malloc(symbol_len + 1);
    char* library_buf = (char*)malloc(library_len + 1);
    if (!name_buf || !symbol_buf || !library_buf) {
        free(name_buf);
        free(symbol_buf);
        free(library_buf);
        return lib_error(L, "ffi: out of memory");
    }
    memcpy(name_buf, name, name_len);
    name_buf[name_len] = '\0';
    memcpy(symbol_buf, symbol, symbol_len);
    symbol_buf[symbol_len] = '\0';
    memcpy(library_buf, library, library_len);
    library_buf[library_len] = '\0';

    FfiHandle probe;
    memset(&probe, 0, sizeof(probe));
    if (ffi_parse_signature(signature, &probe) != 0) {
        free(name_buf);
        free(symbol_buf);
        free(library_buf);
        return lib_error(L, "ffi: invalid signature '%s'", signature);
    }

    LunaDynLib lib = ffi_load_library(L, library_buf);
    if (!lib) {
        free(name_buf);
        free(symbol_buf);
        free(library_buf);
        return -1;
    }

    char error[256] = {0};
    LunaDynSym sym = luna_platform_dlsym(lib, symbol_buf, error, sizeof(error));
    if (!sym) {
        luna_platform_dlclose(lib);
        free(name_buf);
        free(symbol_buf);
        free(library_buf);
        return lib_error(L, "ffi: %s", error[0] ? error : "symbol not found");
    }

    FfiHandle* h = (FfiHandle*)luna_newuserdata_finalized(L, sizeof(FfiHandle),
                                                          ffi_handle_finalizer);
    if (!h) {
        luna_platform_dlclose(lib);
        free(name_buf);
        free(symbol_buf);
        free(library_buf);
        return -1;
    }
    memset(h, 0, sizeof(*h));
    h->lib = lib;
    h->sym = sym;
    h->binding = name_buf;
    h->symbol = symbol_buf;
    h->library = library_buf;
    h->ret = probe.ret;
    h->nargs = probe.nargs;
    memcpy(h->args, probe.args, sizeof(h->args));

    luna_pushcclosure(L, ffi_call, 1);
    if (L->status != LUNA_OK) return -1;
    return 1;
}

static int ffi_value_to_int(LunaValue v, FfiType type, intptr_t* out) {
    LunaString* s;
    LunaUserdata* u;
    int64_t i;

    switch (type) {
        case FFI_TINT:
            if (!luna_value_to_integer(v, &i)) return -1;
            *out = (intptr_t)i;
            return 0;
        case FFI_TSTR:
            if (v.type != LUNA_TSTRING) return -1;
            s = luna_val_string(v);
            *out = (intptr_t)s->data;
            return 0;
        case FFI_TPTR:
            if (v.type != LUNA_TUSERDATA) return -1;
            u = luna_val_userdata(v);
            *out = (intptr_t)luna_userdata_data(u);
            return 0;
        default:
            return -1;
    }
}

static int ffi_value_to_float(LunaValue v, double* out) {
    return luna_value_to_number(v, out) ? 0 : -1;
}

#define FFI_RET_VOID(expr) do { (expr); return 0; } while (0)
#define FFI_RET_INT(expr) do { \
    intptr_t r_ = (intptr_t)(expr); \
    if (h->ret == FFI_TSTR) { \
        const char* s_ = (const char*)r_; \
        if (s_) luna_pushstring(L, s_); else luna_pushnil(L); \
    } else if (h->ret == FFI_TPTR) { \
        luna_pushlightuserdata(L, (void*)r_); \
    } else { \
        luna_pushinteger(L, (int64_t)r_); \
    } \
    return 1; \
} while (0)
#define FFI_RET_FLOAT(expr) do { \
    double r_ = (double)(expr); \
    luna_pushfloat(L, r_); \
    return 1; \
} while (0)

static void ffi_call_v_0(void* f, intptr_t* iv, double* dv) { (void)iv; (void)dv; ((void(*)())f)(); }
static intptr_t ffi_call_i_0(void* f, intptr_t* iv, double* dv) { (void)dv; return ((intptr_t(*)())f)(); }
static double ffi_call_f_0(void* f, intptr_t* iv, double* dv) { (void)dv; return ((double(*)())f)(); }
static void ffi_call_vf_0(void* f, intptr_t* iv, double* dv) { (void)iv; ((void(*)())f)(); }
static intptr_t ffi_call_if_0(void* f, intptr_t* iv, double* dv) { (void)iv; return ((intptr_t(*)())f)(); }
static double ffi_call_ff_0(void* f, intptr_t* iv, double* dv) { (void)iv; return ((double(*)())f)(); }

static void ffi_call_v_1(void* f, intptr_t* iv, double* dv) {  ((void(*)(intptr_t))f)(iv[0]); }
static intptr_t ffi_call_i_1(void* f, intptr_t* iv, double* dv) {  return ((intptr_t(*)(intptr_t))f)(iv[0]); }
static double ffi_call_f_1(void* f, intptr_t* iv, double* dv) {  return ((double(*)(intptr_t))f)(iv[0]); }
static void ffi_call_vf_1(void* f, intptr_t* iv, double* dv) {  ((void(*)(double))f)(dv[0]); }
static intptr_t ffi_call_if_1(void* f, intptr_t* iv, double* dv) {  return ((intptr_t(*)(double))f)(dv[0]); }
static double ffi_call_ff_1(void* f, intptr_t* iv, double* dv) {  return ((double(*)(double))f)(dv[0]); }

static void ffi_call_v_2(void* f, intptr_t* iv, double* dv) {  ((void(*)(intptr_t, intptr_t))f)(iv[0], iv[1]); }
static intptr_t ffi_call_i_2(void* f, intptr_t* iv, double* dv) {  return ((intptr_t(*)(intptr_t, intptr_t))f)(iv[0], iv[1]); }
static double ffi_call_f_2(void* f, intptr_t* iv, double* dv) {  return ((double(*)(intptr_t, intptr_t))f)(iv[0], iv[1]); }
static void ffi_call_vf_2(void* f, intptr_t* iv, double* dv) {  ((void(*)(double, double))f)(dv[0], dv[1]); }
static intptr_t ffi_call_if_2(void* f, intptr_t* iv, double* dv) {  return ((intptr_t(*)(double, double))f)(dv[0], dv[1]); }
static double ffi_call_ff_2(void* f, intptr_t* iv, double* dv) {  return ((double(*)(double, double))f)(dv[0], dv[1]); }

static void ffi_call_v_3(void* f, intptr_t* iv, double* dv) {  ((void(*)(intptr_t, intptr_t, intptr_t))f)(iv[0], iv[1], iv[2]); }
static intptr_t ffi_call_i_3(void* f, intptr_t* iv, double* dv) {  return ((intptr_t(*)(intptr_t, intptr_t, intptr_t))f)(iv[0], iv[1], iv[2]); }
static double ffi_call_f_3(void* f, intptr_t* iv, double* dv) {  return ((double(*)(intptr_t, intptr_t, intptr_t))f)(iv[0], iv[1], iv[2]); }
static void ffi_call_vf_3(void* f, intptr_t* iv, double* dv) {  ((void(*)(double, double, double))f)(dv[0], dv[1], dv[2]); }
static intptr_t ffi_call_if_3(void* f, intptr_t* iv, double* dv) {  return ((intptr_t(*)(double, double, double))f)(dv[0], dv[1], dv[2]); }
static double ffi_call_ff_3(void* f, intptr_t* iv, double* dv) {  return ((double(*)(double, double, double))f)(dv[0], dv[1], dv[2]); }

static void ffi_call_v_4(void* f, intptr_t* iv, double* dv) {  ((void(*)(intptr_t, intptr_t, intptr_t, intptr_t))f)(iv[0], iv[1], iv[2], iv[3]); }
static intptr_t ffi_call_i_4(void* f, intptr_t* iv, double* dv) {  return ((intptr_t(*)(intptr_t, intptr_t, intptr_t, intptr_t))f)(iv[0], iv[1], iv[2], iv[3]); }
static double ffi_call_f_4(void* f, intptr_t* iv, double* dv) {  return ((double(*)(intptr_t, intptr_t, intptr_t, intptr_t))f)(iv[0], iv[1], iv[2], iv[3]); }
static void ffi_call_vf_4(void* f, intptr_t* iv, double* dv) {  ((void(*)(double, double, double, double))f)(dv[0], dv[1], dv[2], dv[3]); }
static intptr_t ffi_call_if_4(void* f, intptr_t* iv, double* dv) {  return ((intptr_t(*)(double, double, double, double))f)(dv[0], dv[1], dv[2], dv[3]); }
static double ffi_call_ff_4(void* f, intptr_t* iv, double* dv) {  return ((double(*)(double, double, double, double))f)(dv[0], dv[1], dv[2], dv[3]); }

static int ffi_call(LunaState* L) {
    FfiHandle* h = ffi_frame_handle(L);
    if (!h) return lib_error(L, "ffi: invalid native handle");

    intptr_t iv[FFI_MAX_ARGS] = {0};
    double dv[FFI_MAX_ARGS] = {0};
    int all_float = h->nargs > 0;

    for (int i = 0; i < h->nargs; i++) {
        LunaValue v = lib_arg(L, i);
        if (h->args[i] == FFI_TFLOAT) {
            if (ffi_value_to_float(v, &dv[i]) != 0)
                return lib_error(L, "ffi: argument %d must be a number", i + 1);
        } else {
            all_float = 0;
            if (ffi_value_to_int(v, h->args[i], &iv[i]) != 0)
                return lib_error(L, "ffi: invalid value for argument %d", i + 1);
        }
    }

    if (h->nargs > 0 && !all_float) {
        int has_float = 0;
        for (int i = 0; i < h->nargs; i++) {
            if (h->args[i] == FFI_TFLOAT) has_float = 1;
        }
        if (has_float)
            return lib_error(L, "ffi: mixed float/integer arguments are not supported");
    }

    void* f = (void*)h->sym;
    if (h->nargs == 0) {
        if (h->ret == FFI_TVOID) FFI_RET_VOID(ffi_call_v_0(f, iv, dv));
        if (h->ret == FFI_TFLOAT) FFI_RET_FLOAT(ffi_call_f_0(f, iv, dv));
        FFI_RET_INT(ffi_call_i_0(f, iv, dv));
    }
    if (h->nargs == 1) {
        if (all_float) {
            if (h->ret == FFI_TVOID) FFI_RET_VOID(ffi_call_vf_1(f, iv, dv));
            if (h->ret == FFI_TFLOAT) FFI_RET_FLOAT(ffi_call_ff_1(f, iv, dv));
            FFI_RET_INT(ffi_call_if_1(f, iv, dv));
        }
        if (h->ret == FFI_TVOID) FFI_RET_VOID(ffi_call_v_1(f, iv, dv));
        if (h->ret == FFI_TFLOAT) FFI_RET_FLOAT(ffi_call_f_1(f, iv, dv));
        FFI_RET_INT(ffi_call_i_1(f, iv, dv));
    }
    if (h->nargs == 2) {
        if (all_float) {
            if (h->ret == FFI_TVOID) FFI_RET_VOID(ffi_call_vf_2(f, iv, dv));
            if (h->ret == FFI_TFLOAT) FFI_RET_FLOAT(ffi_call_ff_2(f, iv, dv));
            FFI_RET_INT(ffi_call_if_2(f, iv, dv));
        }
        if (h->ret == FFI_TVOID) FFI_RET_VOID(ffi_call_v_2(f, iv, dv));
        if (h->ret == FFI_TFLOAT) FFI_RET_FLOAT(ffi_call_f_2(f, iv, dv));
        FFI_RET_INT(ffi_call_i_2(f, iv, dv));
    }
    if (h->nargs == 3) {
        if (all_float) {
            if (h->ret == FFI_TVOID) FFI_RET_VOID(ffi_call_vf_3(f, iv, dv));
            if (h->ret == FFI_TFLOAT) FFI_RET_FLOAT(ffi_call_ff_3(f, iv, dv));
            FFI_RET_INT(ffi_call_if_3(f, iv, dv));
        }
        if (h->ret == FFI_TVOID) FFI_RET_VOID(ffi_call_v_3(f, iv, dv));
        if (h->ret == FFI_TFLOAT) FFI_RET_FLOAT(ffi_call_f_3(f, iv, dv));
        FFI_RET_INT(ffi_call_i_3(f, iv, dv));
    }
    if (h->nargs == 4) {
        if (all_float) {
            if (h->ret == FFI_TVOID) FFI_RET_VOID(ffi_call_vf_4(f, iv, dv));
            if (h->ret == FFI_TFLOAT) FFI_RET_FLOAT(ffi_call_ff_4(f, iv, dv));
            FFI_RET_INT(ffi_call_if_4(f, iv, dv));
        }
        if (h->ret == FFI_TVOID) FFI_RET_VOID(ffi_call_v_4(f, iv, dv));
        if (h->ret == FFI_TFLOAT) FFI_RET_FLOAT(ffi_call_f_4(f, iv, dv));
        FFI_RET_INT(ffi_call_i_4(f, iv, dv));
    }

    return lib_error(L, "ffi: at most 4 arguments are supported");
}

static int ffi_declare(LunaState* L) {
    return ffi_declare_impl(L, 0, 0, 1, 2);
}

static int ffi_declare_symbol(LunaState* L) {
    return ffi_declare_impl(L, 0, 1, 2, 3);
}

static int ffi_load(LunaState* L) {
    size_t len = 0;
    const char* path = ffi_arg_string(L, 0, &len);
    if (!path) return -1;
    char* buf = (char*)malloc(len + 1);
    if (!buf) return lib_error(L, "ffi: out of memory");
    memcpy(buf, path, len);
    buf[len] = '\0';
    LunaDynLib lib = ffi_load_library(L, buf);
    if (!lib) {
        free(buf);
        return -1;
    }

    FfiLibrary* handle = (FfiLibrary*)luna_newuserdata_finalized(L, sizeof(FfiLibrary),
                                                                  ffi_library_finalizer);
    if (!handle) {
        luna_platform_dlclose(lib);
        free(buf);
        return -1;
    }
    handle->lib = lib;
    handle->path = buf;
    return 1;
}

static int ffi_extension(LunaState* L) {
    (void)L;
    luna_pushstring(L, luna_platform_dl_extension());
    return 1;
}

static void ffi_register_functions(LunaState* L, LunaTable* exports) {
    lib_export_function(L, exports, "declare", ffi_declare);
    lib_export_function(L, exports, "declare_symbol", ffi_declare_symbol);
    lib_export_function(L, exports, "load", ffi_load);
    lib_export_function(L, exports, "extension", ffi_extension);
}

static const LunaModule g_ffi_module = { "ffi_native", ffi_register_functions, NULL };

void luna_open_ffi_lib(LunaState* L) {
    luna_register_function(L, "__ffi_declare", ffi_declare);
    luna_register_module(L, &g_ffi_module);
}
