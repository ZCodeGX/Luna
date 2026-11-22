#include "list.h"
#include "runtime_internal.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 8

LunaList* luna_list_new(LunaState* L) {
    LunaList* list = (LunaList*)luna_alloc(L, NULL, 0, sizeof(LunaList));
    if (!list) return NULL;
    
    list->header.type = LUNA_TLIST;
    list->header.marked = 0;
    list->header.gc_next = L->all_objects;
    L->all_objects = &list->header;
    
    list->length = 0;
    list->capacity = INITIAL_CAPACITY;
    list->items = (LunaValue*)luna_alloc(L, NULL, 0, list->capacity * sizeof(LunaValue));
    if (!list->items) {
        luna_alloc(L, list, sizeof(LunaList), 0);
        return NULL;
    }
    
    return list;
}

void luna_list_push_value(LunaState* L, LunaList* list, LunaValue value) {
    if (list->length >= list->capacity) {
        size_t new_cap = list->capacity * 2;
        LunaValue* new_items = (LunaValue*)luna_alloc(L, list->items,
                                                       list->capacity * sizeof(LunaValue),
                                                       new_cap * sizeof(LunaValue));
        if (!new_items) return;
        list->items = new_items;
        list->capacity = new_cap;
    }
    
    list->items[list->length++] = value;
}

bool luna_list_get(LunaList* list, size_t index, LunaValue* out) {
    if (index >= list->length) return false;
    *out = list->items[index];
    return true;
}

void luna_list_set(LunaState* L, LunaList* list, size_t index, LunaValue value) {
    (void)L;
    if (index < list->length) {
        list->items[index] = value;
    }
}