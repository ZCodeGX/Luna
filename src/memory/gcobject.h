#ifndef LUNA_GCOBJECT_H
#define LUNA_GCOBJECT_H

#include <stdint.h>

/*
 * Cabecera común de TODOS los objetos gestionados por el GC.
 * Este es el único lugar donde se define LunaGCObject.
 *
 * Todos los objetos del runtime (LunaString, LunaTable, LunaList,
 * LunaFunction, LunaUserdata) comienzan con este header como primer campo.
 */
typedef struct LunaGCObject {
    uint8_t              type;     /* LunaType (LUNA_TSTRING, LUNA_TTABLE, ...) */
    uint8_t              marked;   /* bit 0: marcado; bit 1: es gris (para GC incremental futuro) */
    struct LunaGCObject* gc_next;  /* siguiente en la lista de todos los objetos */
} LunaGCObject;

/* Flags de marcado */
#define LUNA_GC_MARK_WHITE 0   /* no visitado (candidato a liberar) */
#define LUNA_GC_MARK_GRAY  1   /* visitado, hijos no visitados */
#define LUNA_GC_MARK_BLACK 2   /* visitado, hijos visitados */

#endif /* LUNA_GCOBJECT_H */