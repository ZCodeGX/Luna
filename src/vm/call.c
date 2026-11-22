#include "vm_internal.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Upvalues
 * ============================================================ */

/*
 * Busca o crea un upvalue abierto que apunte a `slot`.
 * Los upvalues abiertos se mantienen en una lista ordenada por slot
 * (de mayor a menor) para facilitar el cierre.
 */
LunaUpvalue* vm_find_upvalue(LunaState* L, CallFrame* frame, LunaValue* slot) {
    LunaVM* vm = state_vm(L);
    (void)frame;

    /* Los upvalues abiertos se mantienen en una lista global */
    LunaUpvalue** p = &vm->open_upvalues;
    LunaUpvalue* uv;

    while (*p != NULL && (*p)->location > slot) {
        p = &(*p)->next;
    }

    if (*p != NULL && (*p)->location == slot) {
        return *p;
    }

    /* Crear nuevo upvalue */
    uv = (LunaUpvalue*)luna_alloc(L, NULL, 0, sizeof(LunaUpvalue));
    if (!uv) return NULL;

    uv->location = slot;
    uv->closed = luna_nil();
    uv->next = *p;
    *p = uv;

    return uv;
}

/*
 * Cierra todos los upvalues abiertos desde `from` hacia arriba.
 * "Cerrar" significa copiar el valor del slot al campo `closed`
 * y hacer que `location` apunte a `&closed`.
 */
void vm_close_upvalues(LunaState* L, LunaValue* from) {
    LunaVM* vm = state_vm(L);

    while (vm->open_upvalues != NULL && vm->open_upvalues->location >= from) {
        LunaUpvalue* uv = vm->open_upvalues;
        uv->closed = *uv->location;
        uv->location = &uv->closed;
        vm->open_upvalues = uv->next;
    }
}

/* ============================================================
 * Protocolo de llamadas
 * ============================================================ */

/*
 * Prepara un frame para llamar a una función Luna.
 */
static int setup_luna_call(LunaState* L, LunaFunction* func, int func_index,
                           int nargs, int nresults) {
    LunaVM* vm = state_vm(L);

    if (vm->frame_top >= LUNA_MAX_CALLS) {
        luna_error_set_simple(L, LUNA_ERR_RUNTIME, "call stack overflow");
        return -1;
    }

    LunaProto* proto = func->proto;
    if (!proto) {
        luna_error_set_simple(L, LUNA_ERR_RUNTIME, "attempt to call a nil proto");
        return -1;
    }

    /* Ajustar nargs vs num_params */
    /* Los argumentos están en el stack justo antes de la base del nuevo frame.
     * La función callee está en base[-1] (o en la posición que indique el caller). */

    /* La base del nuevo frame empieza donde están los argumentos.
     * Convención: stack[stack_top - nargs - 1] = función
     *            stack[stack_top - nargs ... stack_top - 1] = argumentos */

    /* Para simplificar, asumimos que el caller ya colocó la función y los args.
     * El frame base apunta al primer argumento (o al slot de la función). */

    CallFrame* caller = vm->frame_top > 0 ? &vm->frames[vm->frame_top - 1] : NULL;
    size_t caller_base_off = caller ? (size_t)(caller->base - L->stack) : 0;
    CallFrame* frame = &vm->frames[vm->frame_top++];
    frame->proto = proto;
    frame->func = func;
    frame->pc = 0;
    frame->num_results = (uint8_t)nresults;
    frame->num_args = (uint8_t)nargs;
    frame->is_c = 0;
    frame->open_upvalues = NULL;
    frame->num_open_upvalues = 0;
    frame->num_ups = 0;
    frame->handler_pc = -1;

    /* La base del frame es el slot donde está la función.
     * Los slots base[1..n] son los parámetros. */
    frame->base = &L->stack[func_index];

    /* Asegurar que el stack tenga espacio para locales */
    size_t needed = (size_t)func_index + proto->stack_size + 2;
    vm_stack_ensure(L, needed > L->stack_size ? needed - L->stack_size : 0);
    if (L->status != LUNA_OK) return -1;

    /* Reajustar bases tras posible realloc */
    frame->base = &L->stack[func_index];
    if (caller) caller->base = &L->stack[caller_base_off];

    /* Rellenar parámetros faltantes con nil */
    for (int i = nargs; i < proto->num_params; i++) {
        frame->base[1 + i] = luna_nil();
    }

    L->stack_top = (size_t)func_index + (size_t)nargs + 1;

    /* Ligar upvalues desde el frame llamador */
    frame->num_ups = (int)proto->num_upvalues;
    for (int i = 0; i < frame->num_ups; i++) {
        LunaUpvalueDesc* desc = &proto->upvalues[i];
        LunaUpvalue* uv = NULL;
        if (caller && !caller->is_c) {
            if (desc->in_stack) {
                uv = vm_find_upvalue(L, caller, &caller->base[desc->index + 1]);
            } else if (desc->index < caller->num_ups) {
                uv = caller->ups[desc->index];
            }
        }
        if (!uv) {
            luna_error_set_simple(L, LUNA_ERR_RUNTIME, "invalid upvalue binding");
            return -1;
        }
        frame->ups[i] = uv;
    }

    return 0;
}

/*
 * Prepara un frame para llamar a una función C.
 */
static int setup_c_call(LunaState* L, LunaFunction* func, int func_index,
                        int nargs, int nresults) {
    LunaVM* vm = state_vm(L);

    if (vm->frame_top >= LUNA_MAX_CALLS) {
        luna_error_set_simple(L, LUNA_ERR_RUNTIME, "call stack overflow");
        return -1;
    }

    CallFrame* frame = &vm->frames[vm->frame_top++];
    frame->proto = NULL;
    frame->func = func;
    frame->pc = 0;
    frame->num_results = (uint8_t)nresults;
    frame->num_args = nargs;
    frame->is_c = 1;
    frame->open_upvalues = NULL;
    frame->num_open_upvalues = 0;
    frame->num_ups = 0;
    frame->handler_pc = -1;

    /* La base para una función C es el slot de la función */
    frame->base = &L->stack[func_index];

    return 0;
}

/*
 * Ejecuta una llamada a función.
 * func_index: índice en el stack donde está la función.
 * nargs: número de argumentos (están en func_index+1 .. func_index+nargs).
 * nresults: número de resultados esperados (-1 = todos).
 */
int vm_call_function(LunaState* L, int func_index, int nargs, int nresults) {
    LunaValue func_val = L->stack[func_index];

    if (func_val.type != LUNA_TFUNCTION) {
        luna_error_set(L, LUNA_ERR_TYPE, NULL, 0, 0,
                       "attempt to call a %s value", luna_type_name(func_val.type));
        return -1;
    }

    LunaFunction* func = luna_val_function(func_val);

    if (func->c_function) {
        /* Función C */
        size_t saved_top = L->stack_top;
        if (setup_c_call(L, func, func_index, nargs, nresults) != 0) return -1;

        LunaVM* vm = state_vm(L);

        int ret = func->c_function(L);

        /* Pop del frame */
        vm->frame_top--;

        if (ret < 0) {
            /* Error: el mensaje está en el top del stack */
            if (L->stack_top > 0) {
                LunaValue err = L->stack[L->stack_top - 1];
                if (err.type == LUNA_TSTRING) {
                    LunaString* s = luna_val_string(err);
                    luna_error_set_simple(L, LUNA_ERR_NATIVE, "%.*s",
                                          (int)s->len, s->data);
                } else {
                    luna_error_set_simple(L, LUNA_ERR_NATIVE, "native function error");
                }
            }
            return -1;
        }

        /* Resultados: los últimos `ret` valores del stack */
        size_t base_idx = (size_t)func_index;
        size_t results_start = L->stack_top - (size_t)ret;

        for (int i = 0; i < ret; i++) {
            L->stack[base_idx + (size_t)i] = L->stack[results_start + (size_t)i];
        }

        int produced = ret;
        if (nresults >= 0) {
            for (int i = ret; i < nresults; i++) {
                L->stack[base_idx + (size_t)i] = luna_nil();
            }
            produced = nresults;
        }
        if (produced == 0) L->stack[base_idx] = luna_nil();

        size_t min_top = base_idx + (size_t)(produced > 0 ? produced : 1);
        L->stack_top = saved_top > min_top ? saved_top : min_top;

        return 0;
    } else {
        /* Función Luna */
        return setup_luna_call(L, func, func_index, nargs, nresults);
    }
}

/*
 * Llama a una función directamente (sin índice de stack).
 */
int vm_call_internal(LunaState* L, LunaValue func, int nargs, int nresults) {
    vm_stack_push(L, func);
    if (L->status != LUNA_OK) return -1;
    int func_idx = (int)L->stack_top - 1 - nargs;
    return vm_call_function(L, func_idx, nargs, nresults);
}

/* ============================================================
 * Throw / manejo de excepciones
 * ============================================================ */

/*
 * Lanza un error. Busca un handler en la pila de frames.
 * Si no hay handler, el error es fatal.
 */
int vm_throw(LunaState* L, LunaValue value) {
    LunaVM* vm = state_vm(L);

    /* Poner el valor en el stack */
    vm_stack_push(L, value);
    if (L->status != LUNA_OK) return -1;

    /* Buscar frame con handler */
    while (vm->frame_top > 0) {
        CallFrame* frame = &vm->frames[vm->frame_top - 1];

        if (frame->handler_pc >= 0) {
            /* Hay handler: saltar a él */
            frame->pc = (size_t)frame->handler_pc;
            frame->handler_pc = -1;  /* consumir el handler */
            return 0;
        }

        /* Cerrar upvalues de este frame */
        vm_close_upvalues(L, frame->base);

        /* Pop del frame */
        vm->frame_top--;
    }

    /* Sin handler: error fatal */
    if (value.type == LUNA_TSTRING) {
        LunaString* s = luna_val_string(value);
        luna_error_set_simple(L, LUNA_ERR_USER, "%.*s", (int)s->len, s->data);
    } else {
        luna_error_set_simple(L, LUNA_ERR_USER, "unhandled exception");
    }
    return -1;
}