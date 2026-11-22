#ifndef LUNA_LIST_H
#define LUNA_LIST_H

#include "runtime_internal.h"

struct LunaList {
    LunaGCObject header;
    LunaValue*   items;
    size_t       length;
    size_t       capacity;
};

/* Crear una lista vacía */
LunaList* luna_list_new(LunaState* L);

/* Añadir elemento al final */
void luna_list_push_value(LunaState* L, LunaList* list, LunaValue value);

/* Obtener elemento por índice (0-based). Devuelve false si fuera de rango. */
bool luna_list_get(LunaList* list, size_t index, LunaValue* out);

/* Establecer elemento por índice. */
void luna_list_set(LunaState* L, LunaList* list, size_t index, LunaValue value);

/* Longitud */
static inline size_t luna_list_len(LunaList* list) { return list->length; }

#endif /* LUNA_LIST_H */