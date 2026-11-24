#include "runtime_internal.h"
#include "lstring.h"
#include "table.h"
#include "list.h"
#include "function.h"
#include "userdata.h"
#include "../memory/memory_internal.h"
#include "../vm/vm.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_STACK_SIZE 256

/* ---------- Allocator con contabilidad ---------- */

void* luna_alloc(LunaState* L, void* ptr, size_t old_size, size_t new_size) {
    if (new_size > 0 && L->memory_limit > 0 &&
        (L->bytes_allocated >= L->memory_limit ||
         new_size > L->memory_limit - L->bytes_allocated)) {
        return NULL;
    }

    void* result = L->alloc(L->alloc_ud, ptr, old_size, new_size);
    
    /* Solo ajustar contabilidad si la operación tuvo éxito,
     * o si fue una liberación (new_size == 0 siempre "tiene éxito"). */
    if (result != NULL || new_size == 0) {
        ptrdiff_t delta = (ptrdiff_t)new_size - (ptrdiff_t)old_size;
        L->bytes_allocated = (size_t)((ptrdiff_t)L->bytes_allocated + delta);
    }
    
    return result;
}

