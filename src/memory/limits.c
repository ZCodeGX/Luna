#include "gc.h"
#include "../runtime/runtime_internal.h"
#include <stdio.h>
#include <string.h>

/*
 * Verifica que una nueva asignación no exceda el límite de memoria.
 * Devuelve 0 si OK, -1 si se excede el límite.
 *
 * Si el GC automático está activo y bytes_allocated supera el umbral,
 * dispara un paso de GC antes de comprobar.
 */
int luna_check_memory_limit(struct LunaState* L, size_t requested) {
    /* GC automático si corresponde */
    if (L->gc_threshold > 0 && L->bytes_allocated > L->gc_threshold) {
        luna_gc_collect(L);
    }
    
    if (L->memory_limit == 0) return 0;  /* ilimitado */
    
    if (L->bytes_allocated + requested > L->memory_limit) {
        if (!L->status) {  /* solo si no hay error previo */
            L->status = 4;  /* LUNA_ERR_MEMORY */
            snprintf(L->error_message, sizeof(L->error_message),
                     "memory limit exceeded (%zu + %zu > %zu)",
                     L->bytes_allocated, requested, L->memory_limit);
        }
        return -1;
    }
    return 0;
}

/*
 * Verifica que no se haya excedido el límite de instrucciones.
 * Devuelve 0 si OK, -1 si se excede.
 */
int luna_check_instruction_limit(struct LunaState* L) {
    L->instructions_executed++;
    
    if (L->instruction_limit == 0) return 0;  /* ilimitado */
    
    if (L->instructions_executed > L->instruction_limit) {
        if (!L->status) {
            L->status = 9;  /* LUNA_ERR_LIMIT */
            snprintf(L->error_message, sizeof(L->error_message),
                     "instruction limit exceeded (%llu > %llu)",
                     (unsigned long long)L->instructions_executed,
                     (unsigned long long)L->instruction_limit);
        }
        return -1;
    }
    return 0;
}

/*
 * Configura el límite de memoria (0 = ilimitado).
 */
void luna_set_memory_limit(struct LunaState* L, size_t bytes) {
    L->memory_limit = bytes;
}

/*
 * Configura el límite de instrucciones (0 = ilimitado).
 */
void luna_set_instruction_limit(struct LunaState* L, uint64_t n) {
    L->instruction_limit = n;
}