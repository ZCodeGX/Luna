#include "api_internal.h"

void luna_pushnil(LunaState* L) {
    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = luna_nil();
}

void luna_pushboolean(LunaState* L, int b) {
    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = luna_bool(b);
}

void luna_pushinteger(LunaState* L, int64_t n) {
    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = luna_int(n);
}

void luna_pushfloat(LunaState* L, double n) {
    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = luna_float(n);
}

void luna_pushstring(LunaState* L, const char* s) {
    if (!s) {
        luna_pushnil(L);
        return;
    }
    luna_pushlstring(L, s, strlen(s));
}

void luna_pushlstring(LunaState* L, const char* s, size_t len) {
    LunaString* str = luna_string_new(L, s, len);
    if (!str) {
        luna_error_set_simple(L, LUNA_ERR_MEMORY, "out of memory creating string");
        return;
    }
    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = luna_string_val(str);
}

void luna_pushcfunction(LunaState* L, LunaCFunction fn) {
    LunaFunction* f = luna_function_new_c(L, fn);
    if (!f) {
        luna_error_set_simple(L, LUNA_ERR_MEMORY, "out of memory creating function");
        return;
    }
    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = luna_function_val(f);
}

void luna_pushcclosure(LunaState* L, LunaCFunction fn, int nupvalues) {
    /* Los upvalues están en el top del stack.
     * Creamos una función C y asociamos los upvalues. */
    LunaFunction* f = luna_function_new_c(L, fn);
    if (!f) {
        luna_error_set_simple(L, LUNA_ERR_MEMORY, "out of memory creating closure");
        return;
    }

    if (nupvalues > 0) {
        f->upvalues = (LunaUpvalue**)luna_alloc(L, NULL, 0,
                                                 (size_t)nupvalues * sizeof(LunaUpvalue*));
        if (!f->upvalues) {
            luna_error_set_simple(L, LUNA_ERR_MEMORY, "out of memory for upvalues");
            return;
        }
        f->num_upvalues = (size_t)nupvalues;

        /* Los upvalues están en stack[top-nupvalues .. top-1].
         * Los "cerramos" inmediatamente: cada upvalue apunta a su propio closed. */
        for (int i = 0; i < nupvalues; i++) {
            LunaUpvalue* uv = (LunaUpvalue*)luna_alloc(L, NULL, 0, sizeof(LunaUpvalue));
            if (!uv) return;
            uv->closed = L->stack[L->stack_top - nupvalues + i];
            uv->location = &uv->closed;
            uv->next = NULL;
            f->upvalues[i] = uv;
        }

        /* Quitar los upvalues del stack */
        for (int i = 0; i < nupvalues; i++) {
            L->stack[--L->stack_top] = luna_nil();
        }
    }

    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = luna_function_val(f);
}

void luna_pushlightuserdata(LunaState* L, void* p) {
    /* Light userdata: puntero crudo sin GC.
     * Lo representamos como userdata con size=0 y data=p, sin finalizer. */
    LunaUserdata* u = (LunaUserdata*)luna_alloc(L, NULL, 0, sizeof(LunaUserdata));
    if (!u) {
        luna_error_set_simple(L, LUNA_ERR_MEMORY, "out of memory");
        return;
    }
    u->header.type = LUNA_TUSERDATA;
    u->header.marked = 0;
    u->header.gc_next = L->all_objects;
    L->all_objects = &u->header;
    u->data = p;
    u->size = 0;
    u->finalizer = NULL;

    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return;
    L->stack[L->stack_top++] = luna_userdata_val(u);
}

void* luna_newuserdata(LunaState* L, size_t size) {
    LunaUserdata* u = luna_userdata_new(L, size);
    if (!u) {
        luna_error_set_simple(L, LUNA_ERR_MEMORY, "out of memory");
        return NULL;
    }
    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return NULL;
    L->stack[L->stack_top++] = luna_userdata_val(u);
    return u->data;
}

void* luna_newuserdata_finalized(LunaState* L, size_t size, LunaFinalizer fin) {
    LunaUserdata* u = luna_userdata_new_finalized(L, size, fin);
    if (!u) {
        luna_error_set_simple(L, LUNA_ERR_MEMORY, "out of memory");
        return NULL;
    }
    api_ensure_stack(L, 1);
    if (L->status != LUNA_OK) return NULL;
    L->stack[L->stack_top++] = luna_userdata_val(u);
    return u->data;
}