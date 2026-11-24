#include "table.h"
#include "list.h"
#include "runtime_internal.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_SIZE 8
#define EMPTY_NODE (-1)

LunaTable* luna_table_new(LunaState* L) {
    LunaTable* t = (LunaTable*)luna_alloc(L, NULL, 0, sizeof(LunaTable));
    if (!t) return NULL;
    
    t->header.type = LUNA_TTABLE;
    t->header.marked = 0;
    t->header.gc_next = L->all_objects;
    L->all_objects = &t->header;
    
    t->size = INITIAL_SIZE;
    t->count = 0;
    t->nodes = (LunaTableNode*)luna_alloc(L, NULL, 0, t->size * sizeof(LunaTableNode));
    if (!t->nodes) {
        luna_alloc(L, t, sizeof(LunaTable), 0);
        return NULL;
    }
    
    for (uint32_t i = 0; i < t->size; i++) {
        t->nodes[i].key = luna_nil();
        t->nodes[i].value = luna_nil();
        t->nodes[i].hash = 0;
        t->nodes[i].next = EMPTY_NODE;
    }
    
    t->free_list = 0;
    
    return t;
}

static uint32_t hash_position(LunaTable* t, uint64_t hash) {
    return (uint32_t)(hash & (t->size - 1));
}

static void resize(LunaState* L, LunaTable* t, uint32_t new_size) {
    LunaTableNode* old_nodes = t->nodes;
    uint32_t old_size = t->size;
    
    t->size = new_size;
    t->nodes = (LunaTableNode*)luna_alloc(L, NULL, 0, new_size * sizeof(LunaTableNode));
    if (!t->nodes) return;
    
    for (uint32_t i = 0; i < new_size; i++) {
        t->nodes[i].key = luna_nil();
        t->nodes[i].value = luna_nil();
        t->nodes[i].hash = 0;
        t->nodes[i].next = EMPTY_NODE;
    }
    
    t->count = 0;
    t->free_list = 0;
    
    /* Reinsertar todas las entradas */
    for (uint32_t i = 0; i < old_size; i++) {
        if (old_nodes[i].key.type != LUNA_TNIL) {
            luna_table_set(L, t, old_nodes[i].key, old_nodes[i].value);
        }
    }
    
    luna_alloc(L, old_nodes, old_size * sizeof(LunaTableNode), 0);
}

static int find_node(LunaTable* t, LunaValue key, uint64_t hash) {
    uint32_t pos = hash_position(t, hash);
    int idx = (int)pos;
    
    while (idx != EMPTY_NODE) {
        LunaTableNode* n = &t->nodes[idx];
        if (n->hash == hash && luna_value_equal(n->key, key)) {
            return idx;
        }
        idx = n->next;
    }
    
    return EMPTY_NODE;
}

static int alloc_node(LunaState* L, LunaTable* t, uint64_t hash) {
    if (t->count >= t->size * 3 / 4) {
        resize(L, t, t->size * 2);
    }
    
    uint32_t pos = hash_position(t, hash);
    if (t->nodes[pos].key.type == LUNA_TNIL) {
        return (int)pos;
    }
    
    int tail = (int)pos;
    while (t->nodes[tail].next != EMPTY_NODE) {
        tail = t->nodes[tail].next;
    }
    
    int idx = (int)t->free_list;
    while (idx < (int)t->size && t->nodes[idx].key.type != LUNA_TNIL) {
        idx++;
    }
    
    if (idx >= (int)t->size) {
        resize(L, t, t->size * 2);
        return alloc_node(L, t, hash);
    }
    
    t->free_list = (uint32_t)(idx + 1);
    t->nodes[idx].next = EMPTY_NODE;
    t->nodes[tail].next = idx;
    
    return idx;
}

bool luna_table_get(LunaTable* t, LunaValue key, LunaValue* out) {
    uint64_t hash = luna_value_hash(key);
    int idx = find_node(t, key, hash);
    
    if (idx == EMPTY_NODE) {
        return false;
    }
    
    *out = t->nodes[idx].value;
    return true;
}

void luna_table_set(LunaState* L, LunaTable* t, LunaValue key, LunaValue value) {
    uint64_t hash = luna_value_hash(key);
    int idx = find_node(t, key, hash);
    
    if (idx != EMPTY_NODE) {
        t->nodes[idx].value = value;
        return;
    }
    
    idx = alloc_node(L, t, hash);
    if (idx == EMPTY_NODE) return;
    
    t->nodes[idx].key = key;
    t->nodes[idx].value = value;
    t->nodes[idx].hash = hash;
    t->count++;
}

void luna_table_remove(LunaTable* t, LunaValue key) {
    uint64_t hash = luna_value_hash(key);
    uint32_t pos = hash_position(t, hash);
    int idx = (int)pos;
    int prev = EMPTY_NODE;
    
    while (idx != EMPTY_NODE) {
        LunaTableNode* n = &t->nodes[idx];
        if (n->hash == hash && luna_value_equal(n->key, key)) {
            /* Eliminar nodo */
            if (prev == EMPTY_NODE) {
                /* Primer nodo de la cadena */
                int next = n->next;
                if (next != EMPTY_NODE) {
                    t->nodes[pos] = t->nodes[next];
                    t->nodes[next].key = luna_nil();
                    t->nodes[next].value = luna_nil();
                    t->nodes[next].next = EMPTY_NODE;
                } else {
                    n->key = luna_nil();
                    n->value = luna_nil();
                    n->next = EMPTY_NODE;
                }
            } else {
                t->nodes[prev].next = n->next;
                n->key = luna_nil();
                n->value = luna_nil();
                n->next = EMPTY_NODE;
            }
            t->count--;
            return;
        }
        prev = idx;
        idx = n->next;
    }
}

LunaList* luna_table_keys(LunaState* L, LunaTable* t) {
    LunaList* keys = luna_list_new(L);
    if (!keys) return NULL;

    for (uint32_t i = 0; i < t->size; i++) {
        LunaTableNode* n = &t->nodes[i];
        if (n->key.type != LUNA_TNIL) {
            luna_list_push_value(L, keys, n->key);
        }
    }
    return keys;
}