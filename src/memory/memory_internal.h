#ifndef LUNA_MEMORY_INTERNAL_H
#define LUNA_MEMORY_INTERNAL_H

#include "gcobject.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
struct LunaState;
struct LunaValue;

/* ---------- Allocator interno ---------- */

/*
 * Allocator por defecto (malloc/realloc/free).
 * Los usuarios pueden proveer el suyo propio vía LunaAllocator.
 */
void* luna_alloc_default(void* ud, void* ptr, size_t old_size, size_t new_size);

/* ---------- Contabilidad ---------- */

/*
 * Ajusta la contabilidad de bytes tras una asignación/liberación.
 * Llama a este helper desde luna_alloc del runtime.
 */
void luna_gc_adjust_bytes(struct LunaState* L, ptrdiff_t delta);

/* ---------- GC interno ---------- */

void gc_mark_value(struct LunaState* L, struct LunaValue v);
void gc_mark_object(struct LunaState* L, LunaGCObject* obj);
void gc_mark_root(struct LunaState* L);
void gc_sweep(struct LunaState* L);
void gc_mark_gray_children(struct LunaState* L, LunaGCObject* obj);

#endif /* LUNA_MEMORY_INTERNAL_H */