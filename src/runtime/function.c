#include "function.h"
#include "runtime_internal.h"
#include <stdlib.h>
#include <string.h>

LunaUpvalue* luna_upvalue_new(LunaState* L, LunaValue* location) {
    LunaUpvalue* uv = (LunaUpvalue*)luna_alloc(L, NULL, 0, sizeof(LunaUpvalue));
    if (!uv) return NULL;
    uv->location = location;
    uv->closed = luna_nil();
    uv->next = NULL;
    uv->refcount = 1;
    return uv;
}

void luna_upvalue_retain(LunaUpvalue* uv) {
    if (uv) uv->refcount++;
}

void luna_upvalue_release(LunaState* L, LunaUpvalue* uv) {
    if (!uv) return;
    if (--uv->refcount <= 0) {
        luna_alloc(L, uv, sizeof(LunaUpvalue), 0);
    }
}

LunaFunction* luna_function_new(LunaState* L, LunaProto* proto) {
    LunaFunction* f = (LunaFunction*)luna_alloc(L, NULL, 0, sizeof(LunaFunction));
    if (!f) return NULL;
    
    f->header.type = LUNA_TFUNCTION;
    f->header.marked = 0;
    f->header.gc_next = L->all_objects;
    L->all_objects = &f->header;
    
    f->proto = proto;
    f->c_function = NULL;
    f->num_upvalues = proto ? proto->num_upvalues : 0;
    
    if (f->num_upvalues > 0) {
        f->upvalues = (LunaUpvalue**)luna_alloc(L, NULL, 0, f->num_upvalues * sizeof(LunaUpvalue*));
        if (!f->upvalues) {
            luna_alloc(L, f, sizeof(LunaFunction), 0);
            return NULL;
        }
        memset(f->upvalues, 0, f->num_upvalues * sizeof(LunaUpvalue*));
    } else {
        f->upvalues = NULL;
    }
    
    return f;
}

LunaFunction* luna_function_new_c(LunaState* L, int (*fn)(struct LunaState* L)) {
    LunaFunction* f = (LunaFunction*)luna_alloc(L, NULL, 0, sizeof(LunaFunction));
    if (!f) return NULL;
    
    f->header.type = LUNA_TFUNCTION;
    f->header.marked = 0;
    f->header.gc_next = L->all_objects;
    L->all_objects = &f->header;
    
    f->proto = NULL;
    f->c_function = fn;
    f->num_upvalues = 0;
    f->upvalues = NULL;
    
    return f;
}