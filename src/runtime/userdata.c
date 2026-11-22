#include "userdata.h"
#include "runtime_internal.h"
#include <stdlib.h>

LunaUserdata* luna_userdata_new(LunaState* L, size_t size) {
    return luna_userdata_new_finalized(L, size, NULL);
}

LunaUserdata* luna_userdata_new_finalized(LunaState* L, size_t size, LunaFinalizer fin) {
    LunaUserdata* u = (LunaUserdata*)luna_alloc(L, NULL, 0, sizeof(LunaUserdata));
    if (!u) return NULL;
    
    u->header.type = LUNA_TUSERDATA;
    u->header.marked = 0;
    u->header.gc_next = L->all_objects;
    L->all_objects = &u->header;
    
    u->size = size;
    u->data = luna_alloc(L, NULL, 0, size);
    if (!u->data && size > 0) {
        luna_alloc(L, u, sizeof(LunaUserdata), 0);
        return NULL;
    }
    
    u->finalizer = fin;
    
    return u;
}