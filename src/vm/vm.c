#include "vm_internal.h"
#include <stdlib.h>
#include <string.h>

/* Declarada en opcodes.c */
extern int vm_execute_loop(LunaState* L);

/* ============================================================
 * Inicialización
 * ============================================================ */

void luna_vm_init(LunaState* L) {
    LunaVM* vm = (LunaVM*)calloc(1, sizeof(LunaVM));
    if (!vm) return;
    vm->instruction_budget = LUNA_INSTRUCTION_BUDGET;
    L->vm_data = vm;
}

void luna_vm_shutdown(LunaState* L) {
    while (L->open_upvalues) {
        LunaUpvalue* uv = L->open_upvalues;
        L->open_upvalues = uv->next;
        uv->next = NULL;
        luna_upvalue_release(L, uv);
    }

    if (L->vm_data) {
        free(L->vm_data);
        L->vm_data = NULL;
    }
}

/* ============================================================
 * Bucle principal de ejecución
 * ============================================================ */

static int vm_run(LunaState* L) {
    return vm_execute_loop(L);
}

/* ============================================================
 * API pública
 * ============================================================ */

int luna_vm_execute(LunaState* L, LunaProto* proto) {
    if (!proto) {
        luna_error_set_simple(L, LUNA_ERR_ARGUMENT, "null proto");
        return -1;
    }

    LunaVM* vm = state_vm(L);
    if (!vm) {
        luna_vm_init(L);
        vm = state_vm(L);
        if (!vm) {
            luna_error_set_simple(L, LUNA_ERR_MEMORY, "cannot allocate VM");
            return -1;
        }
    }

    vm->instruction_budget = LUNA_INSTRUCTION_BUDGET;

    /* Crear una función wrapper para el proto */
    LunaFunction* main_fn = luna_function_new(L, proto);
    if (!main_fn) return -1;

    /* Colocar la función en el stack */
    int old_return_level = vm->return_level;
    vm->return_level = vm->frame_top;
    int result = -1;

    size_t func_slot = L->stack_top;
    if (vm->frame_top > 0) {
        CallFrame* current = &vm->frames[vm->frame_top - 1];
        size_t current_base = (size_t)(current->base - L->stack);
        size_t limit = current_base + (size_t)current->proto->stack_size + 2;
        if (limit > func_slot) func_slot = limit;
    }
    vm_stack_ensure(L, func_slot + 1 - L->stack_top);
    if (L->status != LUNA_OK) goto done;
    L->stack[func_slot] = luna_function_val(main_fn);
    L->stack_top = func_slot + 1;

    /* Llamar sin argumentos, sin resultados esperados */
    int func_idx = (int)func_slot;
    if (vm_call_function(L, func_idx, 0, 0) != 0) {
        goto done;
    }

    result = vm_run(L);

done:
    vm->return_level = old_return_level;
    return result;
}

int luna_vm_execute_module(LunaState* L, LunaProto* proto, LunaTable* exports) {
    if (!proto || !exports) {
        luna_error_set_simple(L, LUNA_ERR_ARGUMENT, "null proto or exports");
        return -1;
    }

    /* Poner exports como global especial "__exports__" */
    LunaString* key = luna_string_newz(L, "__exports__");
    if (!key) return -1;
    luna_table_set(L, L->globals, luna_string_val(key), luna_table_val(exports));

    return luna_vm_execute(L, proto);
}