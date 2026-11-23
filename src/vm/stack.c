#include "vm_internal.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Operaciones de stack
 * ============================================================ */

void vm_stack_ensure(LunaState* L, size_t extra) {
    if (L->stack_top + extra <= L->stack_size) return;

    LunaVM* vm = state_vm(L);
    size_t frame_count = vm ? (size_t)vm->frame_top : 0;
    size_t upvalue_count = 0;
    for (LunaUpvalue* uv = L->open_upvalues; uv; uv = uv->next) upvalue_count++;

    size_t* frame_offsets = NULL;
    size_t* upvalue_offsets = NULL;
    if (frame_count > 0) {
        frame_offsets = (size_t*)malloc(frame_count * sizeof(size_t));
        if (!frame_offsets) {
            luna_error_set_simple(L, LUNA_ERR_MEMORY, "stack overflow");
            return;
        }
    }
    if (upvalue_count > 0) {
        upvalue_offsets = (size_t*)malloc(upvalue_count * sizeof(size_t));
        if (!upvalue_offsets) {
            free(frame_offsets);
            luna_error_set_simple(L, LUNA_ERR_MEMORY, "stack overflow");
            return;
        }
    }

    LunaValue* old_stack = L->stack;
    size_t old_size = L->stack_size;
    uintptr_t old_begin = (uintptr_t)old_stack;
    uintptr_t old_end = old_begin + old_size * sizeof(LunaValue);

    for (size_t i = 0; i < frame_count; i++) {
        uintptr_t p = (uintptr_t)vm->frames[i].base;
        frame_offsets[i] = (p >= old_begin && p < old_end)
            ? (size_t)(vm->frames[i].base - old_stack) : SIZE_MAX;
    }

    size_t k = 0;
    for (LunaUpvalue* uv = L->open_upvalues; uv; uv = uv->next) {
        uintptr_t p = (uintptr_t)uv->location;
        upvalue_offsets[k++] = (p >= old_begin && p < old_end)
            ? (size_t)(uv->location - old_stack) : SIZE_MAX;
    }

    size_t new_size = L->stack_size * 2;
    while (new_size < L->stack_top + extra) new_size *= 2;

    LunaValue* new_stack = (LunaValue*)luna_alloc(L, old_stack,
                                                   old_size * sizeof(LunaValue),
                                                   new_size * sizeof(LunaValue));
    if (!new_stack) {
        free(frame_offsets);
        free(upvalue_offsets);
        luna_error_set_simple(L, LUNA_ERR_MEMORY, "stack overflow");
        return;
    }

    /* Rellenar los nuevos slots con nil */
    for (size_t i = old_size; i < new_size; i++) {
        new_stack[i] = luna_nil();
    }

    for (size_t i = 0; i < frame_count; i++) {
        if (frame_offsets[i] != SIZE_MAX) {
            vm->frames[i].base = new_stack + frame_offsets[i];
        }
    }

    k = 0;
    for (LunaUpvalue* uv = L->open_upvalues; uv; uv = uv->next) {
        if (upvalue_offsets[k] != SIZE_MAX) {
            uv->location = new_stack + upvalue_offsets[k];
        }
        k++;
    }

    L->stack = new_stack;
    L->stack_size = new_size;

    free(frame_offsets);
    free(upvalue_offsets);
}

void vm_stack_push(LunaState* L, LunaValue v) {
    vm_stack_ensure(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = v;
}

LunaValue vm_stack_pop(LunaState* L) {
    if (L->stack_top == 0) {
        luna_error_set_simple(L, LUNA_ERR_RUNTIME, "stack underflow");
        return luna_nil();
    }
    LunaValue v = L->stack[--L->stack_top];
    L->stack[L->stack_top] = luna_nil();
    return v;
}