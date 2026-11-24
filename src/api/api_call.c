#include <stdio.h>

#include "api_internal.h"
#include "../compiler/compiler.h"
#include "../parser/parser.h"
#include "../platform/platform.h"
#include "../modules/modules_internal.h"

/* ============================================================
 * Ejecución protegida
 * ============================================================ */

LunaStatus luna_call(LunaState* L, int nargs, int nresults) {
    return luna_pcall(L, nargs, nresults, 0);
}

LunaStatus luna_pcall(LunaState* L, int nargs, int nresults, int errfunc) {
    if (nargs < 0 || nresults < -1) {
        luna_error_set_simple(L, LUNA_ERR_ARGUMENT, "invalid argument count");
        return L->error.status;
    }

    if (L->stack_top < (size_t)(nargs + 1)) {
        luna_error_set_simple(L, LUNA_ERR_ARGUMENT, "not enough values on stack");
        return L->error.status;
    }

    LunaVM* vm = state_vm(L);
    if (!vm) {
        luna_vm_init(L);
        vm = state_vm(L);
        if (!vm) {
            luna_error_set_simple(L, LUNA_ERR_MEMORY, "cannot allocate VM");
            return L->error.status;
        }
    }

    int func_idx = (int)L->stack_top - nargs - 1;
    int saved_frame_top = vm->frame_top;
    int saved_return_level = vm->return_level;

    int handler_idx = 0;
    if (errfunc != 0) {
        handler_idx = errfunc > 0 ? errfunc - 1 : (int)L->stack_top + errfunc;
        if (handler_idx < 0 || handler_idx >= (int)L->stack_top ||
            L->stack[handler_idx].type != LUNA_TFUNCTION) {
            handler_idx = 0;
        }
    }

    LunaValue handler = handler_idx ? L->stack[handler_idx] : luna_nil();

    vm->return_level = vm->frame_top;
    int r = vm_call_function(L, func_idx, nargs, nresults < 0 ? -1 : nresults);
    if (r == 0 && vm->frame_top > vm->return_level) {
        r = vm_run(L);
    }
    vm->return_level = saved_return_level;

    if (r != 0 || L->error.status != LUNA_OK) {
        LunaStatus status = L->error.status ? L->error.status : LUNA_ERR_RUNTIME;
        const char* msg = L->error.message[0] ? L->error.message : L->error_message;
        LunaValue errval = luna_nil();

        if (msg && msg[0]) {
            LunaString* s = luna_string_new(L, msg, strlen(msg));
            if (s) errval = luna_string_val(s);
        }

        luna_error_clear(L);
        vm->frame_top = saved_frame_top;
        L->stack_top = (size_t)(func_idx > 0 ? func_idx : 0);

        if (handler_idx) {
            size_t handler_base = L->stack_top;
            vm_stack_push(L, handler);
            vm_stack_push(L, errval);
            int hidx = (int)handler_base;
            vm->return_level = vm->frame_top;
            int hr = vm_call_function(L, hidx, 1, 1);
            if (hr == 0 && vm->frame_top > vm->return_level) {
                hr = vm_run(L);
            }
            vm->return_level = saved_return_level;
            if (hr == 0 && L->stack_top > handler_base) {
                errval = L->stack[handler_base];
            }
            luna_error_clear(L);
            L->stack_top = handler_base;
        }

        vm_stack_push(L, errval);
        return status;
    }

    if (L->stack_top < (size_t)func_idx) {
        L->stack_top = (size_t)func_idx;
    }

    return LUNA_OK;
}

/* ============================================================
 * Carga y ejecución de archivos/strings
 * ============================================================ */

LunaStatus luna_dostring(LunaState* L, const char* src, const char* chunkname) {
    if (!src) {
        luna_error_set_simple(L, LUNA_ERR_ARGUMENT, "null source");
        return LUNA_ERR_ARGUMENT;
    }

    luna_clear_error(L);

    char error_buf[512] = {0};
    LunaProto* proto = luna_compile_source(L, src, strlen(src), chunkname,
                                           error_buf, sizeof(error_buf));
    if (!proto) {
        (void)error_buf;
        return L->error.status ? L->error.status : LUNA_ERR_SYNTAX;
    }

    /* Ejecutar */
    int r = luna_vm_execute(L, proto);
    luna_proto_free(proto);

    if (r != 0) {
        return L->error.status ? L->error.status : LUNA_ERR_RUNTIME;
    }

    return LUNA_OK;
}

LunaStatus luna_dofile(LunaState* L, const char* path) {
    if (!path) {
        luna_error_set_simple(L, LUNA_ERR_ARGUMENT, "null path");
        return LUNA_ERR_ARGUMENT;
    }

    modules_set_current_dir(&L->modules, path);

    char errbuf[256] = {0};
    size_t src_len = 0;
    char* src = luna_platform_read_file(path, &src_len, errbuf, sizeof(errbuf));
    if (!src) {
        luna_error_set(L, LUNA_ERR_IO, path, 0, 0, "%s", errbuf);
        return LUNA_ERR_IO;
    }

    LunaStatus status = luna_dostring(L, src, path);
    free(src);
    return status;
}