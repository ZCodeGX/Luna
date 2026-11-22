#ifndef LUNA_USERDATA_H
#define LUNA_USERDATA_H

#include "runtime_internal.h"
#include <stddef.h>
#include "../../include/luna/luna_types.h"

struct LunaUserdata {
    LunaGCObject  header;
    void*         data;
    size_t        size;
    LunaFinalizer finalizer;
};

/* Crear userdata con tamaño dado */
LunaUserdata* luna_userdata_new(LunaState* L, size_t size);

/* Crear userdata con finalizador */
LunaUserdata* luna_userdata_new_finalized(LunaState* L, size_t size, LunaFinalizer fin);

/* Obtener puntero a datos */
static inline void* luna_userdata_data(LunaUserdata* u) { return u->data; }

#endif /* LUNA_USERDATA_H */