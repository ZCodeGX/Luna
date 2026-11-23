#include "gc.h"
#include "memory_internal.h"
#include "../runtime/runtime_internal.h"
#include "../runtime/lstring.h"
#include "../runtime/table.h"
#include "../runtime/list.h"
#include "../runtime/function.h"
#include "../runtime/userdata.h"
#include <string.h>

/* ---------- Contabilidad ---------- */

void luna_gc_adjust_bytes(struct LunaState* L, ptrdiff_t delta) {
    L->bytes_allocated = (size_t)((ptrdiff_t)L->bytes_allocated + delta);
}

/* ---------- Marcado de valores ---------- */

void gc_mark_value(struct LunaState* L, struct LunaValue v) {
    switch (v.type) {
        case LUNA_TSTRING:
        case LUNA_TTABLE:
        case LUNA_TLIST:
        case LUNA_TFUNCTION:
        case LUNA_TUSERDATA:
            gc_mark_object(L, v.as.gcobj);
            break;
        default:
            /* nil, bool, int, float: no son objetos GC */
            break;
    }
}

void gc_mark_object(struct LunaState* L, LunaGCObject* obj) {
    if (!obj) return;
    if (obj->marked != LUNA_GC_MARK_WHITE) return;  /* ya visitado */
    
    obj->marked = LUNA_GC_MARK_GRAY;
    
    /* Añadir a la lista de grises para procesar hijos después */
    /* (en v0.1 usamos recursión directa; en v0.2 será cola explícita) */
    gc_mark_gray_children(L, obj);
}

/* ---------- Marcado de hijos por tipo ---------- */

static void mark_table(struct LunaState* L, LunaTable* t) {
    for (uint32_t i = 0; i < t->size; i++) {
        LunaTableNode* n = &t->nodes[i];
        if (n->key.type != LUNA_TNIL) {
            gc_mark_value(L, n->key);
            gc_mark_value(L, n->value);
        }
    }
}

static void mark_list(struct LunaState* L, LunaList* list) {
    for (size_t i = 0; i < list->length; i++) {
        gc_mark_value(L, list->items[i]);
    }
}

static void mark_function(struct LunaState* L, LunaFunction* f) {
    for (size_t i = 0; i < f->num_upvalues; i++) {
        LunaUpvalue* uv = f->upvalues[i];
        if (uv) {
            if (uv->location) {
                gc_mark_value(L, *uv->location);
            } else {
                gc_mark_value(L, uv->closed);
            }
        }
    }
}

static void mark_userdata(struct LunaState* L, LunaUserdata* u) {
    (void)L;
    (void)u;
    /* Userdata no contiene valores Luna; el usuario es responsable.
     * Si en el futuro añadimos metatablas, se marcarían aquí. */
}

void gc_mark_gray_children(struct LunaState* L, LunaGCObject* obj) {
    switch (obj->type) {
        case LUNA_TSTRING:
            /* Strings no tienen hijos */
            break;
        case LUNA_TTABLE:
            mark_table(L, (LunaTable*)obj);
            break;
        case LUNA_TLIST:
            mark_list(L, (LunaList*)obj);
            break;
        case LUNA_TFUNCTION:
            mark_function(L, (LunaFunction*)obj);
            break;
        case LUNA_TUSERDATA:
            mark_userdata(L, (LunaUserdata*)obj);
            break;
        default:
            break;
    }
    obj->marked = LUNA_GC_MARK_BLACK;
}

/* ---------- Raíces ---------- */

void gc_mark_root(struct LunaState* L) {
    /* 1. Stack */
    for (size_t i = 0; i < L->stack_top; i++) {
        gc_mark_value(L, L->stack[i]);
    }
    
    /* 2. Tabla global */
    if (L->globals) {
        gc_mark_object(L, (LunaGCObject*)L->globals);
    }
    
    /* 3. Valor lanzado por la excepción activa */
    gc_mark_value(L, L->thrown_value);

    /* 4. Upvalues abiertos */
    LunaUpvalue* uv = L->open_upvalues;
    while (uv) {
        if (uv->location) {
            gc_mark_value(L, *uv->location);
        }
        uv = uv->next;
    }
}

/* ---------- Liberación de objetos ---------- */

static void free_object(struct LunaState* L, LunaGCObject* obj) {
    switch (obj->type) {
        case LUNA_TSTRING: {
            LunaString* s = (LunaString*)obj;
            size_t sz = sizeof(LunaString) + s->len + 1;
            luna_gc_adjust_bytes(L, -(ptrdiff_t)sz);
            L->alloc(L->alloc_ud, s, sz, 0);
            break;
        }
        case LUNA_TTABLE: {
            LunaTable* t = (LunaTable*)obj;
            size_t nodes_sz = t->size * sizeof(LunaTableNode);
            luna_gc_adjust_bytes(L, -(ptrdiff_t)(sizeof(LunaTable) + nodes_sz));
            L->alloc(L->alloc_ud, t->nodes, nodes_sz, 0);
            L->alloc(L->alloc_ud, t, sizeof(LunaTable), 0);
            break;
        }
        case LUNA_TLIST: {
            LunaList* list = (LunaList*)obj;
            size_t items_sz = list->capacity * sizeof(LunaValue);
            luna_gc_adjust_bytes(L, -(ptrdiff_t)(sizeof(LunaList) + items_sz));
            L->alloc(L->alloc_ud, list->items, items_sz, 0);
            L->alloc(L->alloc_ud, list, sizeof(LunaList), 0);
            break;
        }
        case LUNA_TFUNCTION: {
            LunaFunction* f = (LunaFunction*)obj;
            size_t ups_sz = f->num_upvalues * sizeof(LunaUpvalue*);
            luna_gc_adjust_bytes(L, -(ptrdiff_t)(sizeof(LunaFunction) + ups_sz));
            if (f->upvalues) {
                for (size_t i = 0; i < f->num_upvalues; i++) {
                    luna_upvalue_release(L, f->upvalues[i]);
                }
                L->alloc(L->alloc_ud, f->upvalues, ups_sz, 0);
            }
            L->alloc(L->alloc_ud, f, sizeof(LunaFunction), 0);
            break;
        }
        case LUNA_TUSERDATA: {
            LunaUserdata* u = (LunaUserdata*)obj;
            if (u->finalizer) {
                u->finalizer(L, u->data);
            }
            if (u->data) {
                luna_gc_adjust_bytes(L, -(ptrdiff_t)u->size);
                L->alloc(L->alloc_ud, u->data, u->size, 0);
            }
            luna_gc_adjust_bytes(L, -(ptrdiff_t)sizeof(LunaUserdata));
            L->alloc(L->alloc_ud, u, sizeof(LunaUserdata), 0);
            break;
        }
        default:
            break;
    }
}

/* ---------- Sweep ---------- */

void gc_sweep(struct LunaState* L) {
    LunaGCObject** p = &L->all_objects;
    size_t freed_bytes = 0;
    
    while (*p) {
        LunaGCObject* obj = *p;
        if (obj->marked != LUNA_GC_MARK_BLACK) {
            /* No alcanzable: liberar */
            *p = obj->gc_next;
            
            size_t before = L->bytes_allocated;
            free_object(L, obj);
            freed_bytes += before - L->bytes_allocated;
        } else {
            /* Alcanzable: resetear marca para el próximo ciclo */
            obj->marked = LUNA_GC_MARK_WHITE;
            p = &obj->gc_next;
        }
    }
    
    L->total_freed += freed_bytes;
    L->total_collections++;
}

/* ---------- API pública ---------- */

size_t luna_gc_collect(struct LunaState* L) {
    size_t before = L->bytes_allocated;
    
    gc_mark_root(L);
    luna_string_pool_purge(L);
    gc_sweep(L);
    
    size_t freed = before - L->bytes_allocated;
    return freed;
}

int luna_gc_step(struct LunaState* L) {
    luna_gc_collect(L);
    return 1;  /* v0.1: siempre completo */
}

void luna_gc_set_threshold(struct LunaState* L, size_t bytes) {
    L->gc_threshold = bytes;
}

void luna_gc_get_stats(struct LunaState* L, LunaGCStats* out) {
    out->bytes_allocated = L->bytes_allocated;
    out->gc_threshold = L->gc_threshold;
    out->total_collections = L->total_collections;
    out->total_freed = L->total_freed;
    
    /* Contar objetos vivos */
    size_t count = 0;
    LunaGCObject* obj = L->all_objects;
    while (obj) {
        count++;
        obj = obj->gc_next;
    }
    out->total_objects = count;
}