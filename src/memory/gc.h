#ifndef LUNA_GC_H
#define LUNA_GC_H

#include <stddef.h>
#include <stdint.h>

struct LunaState;

/*
 * Ejecuta un ciclo completo de GC (mark + sweep).
 * Libera todos los objetos no alcanzables desde las raíces.
 *
 * Las raíces son:
 *   - El stack de valores
 *   - La tabla global
 *   - Los upvalues abiertos
 *
 * Devuelve el número de bytes liberados.
 */
size_t luna_gc_collect(struct LunaState* L);

/*
 * Ejecuta un "paso" de GC. En esta implementación v0.1 es equivalente
 * a luna_gc_collect. En versiones futuras podrá ser incremental.
 *
 * Devuelve true si el GC quedó "descansado" (no hay más trabajo pendiente).
 */
int luna_gc_step(struct LunaState* L);

/*
 * Configura el umbral de bytes en el que se dispara el GC automático.
 * Cuando bytes_allocated supera este umbral tras una asignación,
 * se ejecuta un paso de GC.
 *
 * Un valor de 0 desactiva el GC automático (solo manual).
 */
void luna_gc_set_threshold(struct LunaState* L, size_t bytes);

/*
 * Devuelve estadísticas del GC.
 */
typedef struct {
    size_t bytes_allocated;     /* bytes actualmente en uso */
    size_t gc_threshold;        /* umbral actual */
    size_t total_objects;       /* número de objetos GC vivos */
    size_t total_collections;   /* número de ciclos completos ejecutados */
    size_t total_freed;         /* bytes totales liberados desde el inicio */
} LunaGCStats;

void luna_gc_get_stats(struct LunaState* L, LunaGCStats* out);

#endif /* LUNA_GC_H */