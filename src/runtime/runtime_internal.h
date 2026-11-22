#ifndef LUNA_RUNTIME_INTERNAL_H
#define LUNA_RUNTIME_INTERNAL_H

#include "value.h"
#include "../memory/alloc.h"
#include "../memory/gcobject.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../modules/modules_internal.h"
#include "../errors/error.h"

/* Forward declaration */
struct LunaModules;
struct LunaUpvalue;

struct LunaState {
    /* ---------- Allocator ---------- */
    void*        alloc_ud;
    LunaAllocFn  alloc;

    /* ---------- Objetos GC ---------- */
    LunaGCObject* all_objects;

    /* ---------- Pool de strings internados ---------- */
    LunaString** string_buckets;
    uint32_t     string_bucket_count;
    uint32_t     string_pool_count;

    /* ---------- Tabla global ---------- */
    struct LunaTable* globals;

    /* ---------- Stack de valores ---------- */
    LunaValue* stack;
    size_t     stack_size;
    size_t     stack_top;

    /* ---------- Upvalues abiertos ---------- */
    struct LunaUpvalue* open_upvalues;

    /* ---------- Contabilidad de memoria ---------- */
    size_t   bytes_allocated;
    size_t   memory_limit;      /* 0 = ilimitado */
    size_t   gc_threshold;      /* umbral para GC automático */

    /* ---------- Contabilidad de instrucciones ---------- */
    uint64_t instructions_executed;
    uint64_t instruction_limit; /* 0 = ilimitado */

    /* ---------- Estadísticas de GC ---------- */
    size_t   total_collections;
    size_t   total_freed;

    /* ---------- Estado de error ---------- */
    int      status;
    char     error_message[512];
    char     error_file[256];
    int      error_line;
    LunaError error;

    /* ---------- Sistema de módulos ---------- */
    struct LunaModules modules;   /* <-- AÑADE ESTA LÍNEA */

    /* ---------- VM ---------- */
    void* vm_data;   /* puntero opaco a LunaVM */

    /* ---------- Referencias de la API ---------- */
    struct LunaTable* ref_table;
    int               ref_free;
};

/*
 * Allocator wrapper con contabilidad.
 * Ajusta bytes_allocated automáticamente tras cada operación.
 */
void* luna_alloc(LunaState* L, void* ptr, size_t old_size, size_t new_size);

#endif /* LUNA_RUNTIME_INTERNAL_H */