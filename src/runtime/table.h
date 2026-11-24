#ifndef LUNA_TABLE_H
#define LUNA_TABLE_H

#include "runtime_internal.h"

typedef struct {
    LunaValue key;
    LunaValue value;
    uint64_t  hash;
    int       next;   /* -1 si vacío, índice del siguiente en la cadena */
} LunaTableNode;

struct LunaTable {
    LunaGCObject header;
    LunaTableNode* nodes;
    uint32_t       size;      /* potencia de 2 */
    uint32_t       count;     /* número de entradas */
    uint32_t       free_list; /* índice del primer nodo libre, o UINT32_MAX */
};

/* Crear una tabla vacía */
LunaTable* luna_table_new(LunaState* L);

/* Obtener valor por clave. Devuelve true si existe. */
bool luna_table_get(LunaTable* t, LunaValue key, LunaValue* out);

/* Establecer valor por clave. */
void luna_table_set(LunaState* L, LunaTable* t, LunaValue key, LunaValue value);

/* Eliminar clave. */
void luna_table_remove(LunaTable* t, LunaValue key);

/* Número de entradas. */
static inline uint32_t luna_table_count(LunaTable* t) { return t->count; }

/* Claves de la tabla en orden de almacenamiento interno. */
LunaList* luna_table_keys(LunaState* L, LunaTable* t);

#endif /* LUNA_TABLE_H */