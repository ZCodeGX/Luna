#include "alloc.h"
#include <stdlib.h>

void* luna_alloc_default(void* ud, void* ptr, size_t old_size, size_t new_size) {
    (void)ud;
    (void)old_size;
    
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, new_size);
}