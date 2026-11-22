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

    /* Buscar en los upvalues abiertos del frame actual */
    LunaUpvalue** prev = &frame->open_upvalues[0];
    /* En realidad, mantenemos una lista global en vm->open_upvalues */

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
static int setup_luna_call(LunaState* L, LunaFunction* func, int nargs, int nresults) {
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

    CallFrame* frame = &vm->frames[vm->frame_top++];
    frame->proto = proto;
    frame->func = func;
    frame->pc = 0;
    frame->num_results = (uint8_t)nresults;
    frame->is_c = 0;
    frame->open_upvalues = NULL;
    frame->num_open_upvalues = 0;
    frame->handler_pc = -1;

    /* La base del frame es el slot donde está la función.
     * Los slots base[1..n] son los parámetros. */
    frame->base = &L->stack[L->stack_top - nargs - 1];

    /* Asegurar que el stack tenga espacio para locales */
    size_t needed = (size_t)(frame->base - L->stack) + proto->stack_size + 1;
    vm_stack_ensure(L, needed > L->stack_size ? needed - L->stack_size : 0);
    if (L->status != LUNA_OK) return -1;

    /* Reajustar base tras posible realloc */
    frame->base = &L->stack[L->stack_top - nargs - 1];

    /* Rellenar parámetros faltantes con nil */
    for (int i = nargs; i < proto->num_params; i++) {
        frame->base[1 + i] = luna_nil();
    }

    /* Ajustar stack_top para incluir los slots del nuevo frame */
    L->stack_top = (size_t)(frame->base - L->stack) + proto->stack_size + 1;

    return 0;
}

/*
 * Prepara un frame para llamar a una función C.
 */
static int setup_c_call(LunaState* L, LunaFunction* func, int nargs, int nresults) {
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
    frame->is_c = 1;
    frame->open_upvalues = NULL;
    frame->num_open_upvalues = 0;
    frame->handler_pc = -1;

    /* La base para una función C es donde están los argumentos */
    frame->base = &L->stack[L->stack_top - nargs];

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
        if (setup_c_call(L, func, nargs, nresults) != 0) return -1;

        /* Reubicar base tras setup */
        LunaVM* vm = state_vm(L);
        CallFrame* frame = &vm->frames[vm->frame_top - 1];

        /* Mover argumentos para que el C los vea en 1..nargs */
        /* En la API de Luna, los argumentos empiezan en índice 1.
         * Para simplificar, los dejamos donde están y el C los lee vía API. */

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
        /* Ajustar: quitar la función y args, dejar solo los resultados */
        size_t base_idx = (size_t)(frame->base - L->stack);
        size_t results_start = L->stack_top - (size_t)ret;

        /* Mover resultados a la posición de la función */
        for (int i = 0; i < ret; i++) {
            L->stack[base_idx + (size_t)i] = L->stack[results_start + (size_t)i];
        }

        /* Rellenar con nil si faltan resultados */
        if (nresults >= 0) {
            for (int i = ret; i < nresults; i++) {
                L->stack[base_idx + (size_t)i] = luna_nil();
            }
            L->stack_top = base_idx + (size_t)nresults;
        } else {
            L->stack_top = base_idx + (size_t)ret;
        }

        return 0;
    } else {
        /* Función Luna */
        return setup_luna_call(L, func, nargs, nresults);
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