#include "api_internal.h"
#include "../bytecode/proto.h"

/* ============================================================
 * Runtime
 * ============================================================ */

LunaRuntime* luna_runtime_new(const LunaAllocator* allocator) {
    LunaRuntime* rt = (LunaRuntime*)calloc(1, sizeof(LunaRuntime));
    if (!rt) return NULL;

    if (allocator && allocator->alloc) {
        rt->allocator = *allocator;
    } else {
        rt->allocator.ud = NULL;
        rt->allocator.alloc = luna_alloc_default;
    }

    rt->memory_limit = 0;
    rt->instruction_limit = 0;
    return rt;
}

void luna_runtime_free(LunaRuntime* rt) {
    free(rt);
}

void luna_runtime_set_memory_limit(LunaRuntime* rt, size_t bytes) {
    if (rt) rt->memory_limit = bytes;
}

void luna_runtime_set_instruction_limit(LunaRuntime* rt, uint64_t n) {
    if (rt) rt->instruction_limit = n;
}

/* ============================================================
 * Estado
 * ============================================================ */

LunaState* luna_new(LunaRuntime* rt) {
    LunaState* L = (LunaState*)calloc(1, sizeof(LunaState));
    if (!L) return NULL;

    /* Configurar allocator */
    if (rt) {
        L->alloc_ud = rt->allocator.ud;
        L->alloc = rt->allocator.alloc;
        L->memory_limit = rt->memory_limit;
        L->instruction_limit = rt->instruction_limit;
    } else {
        L->alloc_ud = NULL;
        L->alloc = luna_alloc_default;
        L->memory_limit = 0;
        L->instruction_limit = 0;
    }

    L->all_objects = NULL;
    L->open_upvalues = NULL;
    L->thrown_value = luna_nil();

    luna_string_pool_init(L);

    /* Tabla global */
    L->globals = luna_table_new(L);
    if (!L->globals) {
        free(L);
        return NULL;
    }

    /* Stack inicial */
    L->stack_size = 256;
    L->stack_top = 0;
    L->stack = (LunaValue*)luna_alloc(L, NULL, 0, L->stack_size * sizeof(LunaValue));
    if (!L->stack) {
        free(L);
        return NULL;
    }
    for (size_t i = 0; i < L->stack_size; i++) {
        L->stack[i] = luna_nil();
    }

    /* Contabilidad */
    L->bytes_allocated = 0;
    L->gc_threshold = L->memory_limit ? (L->memory_limit / 2) : (4u * 1024u * 1024u);
    L->instructions_executed = 0;
    L->total_collections = 0;
    L->total_freed = 0;

    /* Referencias */
    L->ref_table = NULL;  /* se crea bajo demanda */
    L->ref_free = 0;

    /* Error */
    luna_clear_error(L);

    /* VM */
    L->vm_data = NULL;
    luna_vm_init(L);

    /* Módulos */
    modules_init(&L->modules);

    luna_open_libs(L);

    return L;
}

void luna_close(LunaState* L) {
    if (!L) return;

    /* Liberar subsistemas */
    modules_free(L, &L->modules);
    luna_vm_shutdown(L);

    /* Liberar todos los objetos GC */
    LunaGCObject* obj = L->all_objects;
    while (obj) {
        LunaGCObject* next = obj->gc_next;
        switch (obj->type) {
            case LUNA_TSTRING: {
                LunaString* s = (LunaString*)obj;
                L->alloc(L->alloc_ud, s, sizeof(LunaString) + s->len + 1, 0);
                break;
            }
            case LUNA_TTABLE: {
                LunaTable* t = (LunaTable*)obj;
                L->alloc(L->alloc_ud, t->nodes, t->size * sizeof(LunaTableNode), 0);
                L->alloc(L->alloc_ud, t, sizeof(LunaTable), 0);
                break;
            }
            case LUNA_TLIST: {
                LunaList* list = (LunaList*)obj;
                L->alloc(L->alloc_ud, list->items, list->capacity * sizeof(LunaValue), 0);
                L->alloc(L->alloc_ud, list, sizeof(LunaList), 0);
                break;
            }
            case LUNA_TFUNCTION: {
                LunaFunction* f = (LunaFunction*)obj;
                luna_proto_release(f->proto);
                if (f->upvalues) {
                    for (size_t i = 0; i < f->num_upvalues; i++) {
                        luna_upvalue_release(L, f->upvalues[i]);
                    }
                    L->alloc(L->alloc_ud, f->upvalues,
                             f->num_upvalues * sizeof(LunaUpvalue*), 0);
                }
                L->alloc(L->alloc_ud, f, sizeof(LunaFunction), 0);
                break;
            }
            case LUNA_TUSERDATA: {
                LunaUserdata* u = (LunaUserdata*)obj;
                if (u->finalizer) u->finalizer(L, u->data);
                if (u->data) L->alloc(L->alloc_ud, u->data, u->size, 0);
                L->alloc(L->alloc_ud, u, sizeof(LunaUserdata), 0);
                break;
            }
        }
        obj = next;
    }

    /* Liberar stack */
    if (L->stack) {
        L->alloc(L->alloc_ud, L->stack, L->stack_size * sizeof(LunaValue), 0);
    }

    luna_string_pool_free(L);

    free(L);
}