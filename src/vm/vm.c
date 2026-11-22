#include "vm_internal.h"
#include <stdlib.h>
#include <string.h>

/* Declarada en opcodes.c */
extern int vm_execute_instruction(LunaState* L, CallFrame* frame,
                                  const uint8_t* code, LunaValue* base);

/* Declarada en memory/limits.c */
extern int luna_report_instruction_limit(LunaState* L);

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
    if (L->vm_data) {
        free(L->vm_data);
        L->vm_data = NULL;
    }
}

/* ============================================================
 * Bucle principal de ejecución
 * ============================================================ */

static int vm_run(LunaState* L) {
    LunaVM* vm = state_vm(L);
    CallFrame* frame = NULL;
    const uint8_t* code = NULL;
    int frame_valid = 0;
    uint64_t budget = vm->instruction_budget ? vm->instruction_budget
                                             : LUNA_INSTRUCTION_BUDGET;

    for (;;) {
        if (!frame_valid) {
            if (vm->frame_top == 0) {
                return 0;  /* terminó */
            }
            frame = &vm->frames[vm->frame_top - 1];
            code = frame->proto->code;
            frame_valid = 1;
        }

        L->instructions_executed++;
        if (--budget == 0) {
            budget = LUNA_INSTRUCTION_BUDGET;
            if (L->instruction_limit != 0 &&
                L->instructions_executed > L->instruction_limit) {
                vm->instruction_budget = budget;
                luna_report_instruction_limit(L);
                return -1;
            }
        }

        /* base se recarga por si una llamada C reasignó el stack */
        LunaValue* base = frame->base;
        int r = vm_execute_instruction(L, frame, code, base);

        if (r < 0) {
            /* Error */
            return -1;
        }

        if (r == 2) {
            /* Chunk principal retornó */
            return 0;
        }

        /* r == 1: CALL/RETURN cambió el frame activo */
        if (r == 1) {
            frame_valid = 0;
        }
    }
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
    vm_stack_push(L, luna_function_val(main_fn));
    if (L->status != LUNA_OK) return -1;

    /* Llamar sin argumentos, sin resultados esperados */
    int func_idx = (int)L->stack_top - 1;
    if (vm_call_function(L, func_idx, 0, 0) != 0) {
        return -1;
    }

    return vm_run(L);
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