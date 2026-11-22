#ifndef LUNA_ALLOC_H
#define LUNA_ALLOC_H

#include <stddef.h>
#include "../../include/luna/luna_types.h"

/*
 * Allocator por defecto (malloc/realloc/free).
 * Útil como fallback cuando el usuario no provee uno.
 */
void* luna_alloc_default(void* ud, void* ptr, size_t old_size, size_t new_size);

#endif /* LUNA_ALLOC_H */