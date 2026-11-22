#include "modules_internal.h"
#include "module.h"
#include "../runtime/runtime_internal.h"
#include "../runtime/table.h"
#include "../runtime/lstring.h"
#include <string.h>
#include <stdio.h>

/*
 * Verifica si un símbolo es accesible desde fuera del módulo.
 *
 * Reglas:
 *   - El módulo debe tener una tabla de exports.
 *   - El símbolo debe estar presente en exports.
 *   - Los símbolos no marcados pub durante la compilación NO se añaden a exports,
 *     por lo que esta función solo necesita comprobar presencia.
 *
 * En el futuro, si queremos metadatos de visibilidad más ricos
 * (ej: "pub(restricted)"), esta función se extenderá.
 */
int modules_check_visibility(LunaModules* M, const LunaModuleInternal* mod,
                             const char* symbol_name, char* error_out, size_t error_size) {
    (void)M;
    
    if (!mod || !mod->exports) {
        if (error_out) {
            snprintf(error_out, error_size, "module not loaded");
        }
        return -1;
    }
    
    /* Buscar el símbolo en la tabla de exports */
    /* Nota: necesitamos un estado para crear el string.
     * En la práctica, el caller pasa el estado. Aquí usamos una búsqueda
     * iterando la tabla. */
    
    /* Búsqueda lineal (suficiente para v0.1; las tablas de exports son pequeñas) */
    for (uint32_t i = 0; i < mod->exports->size; i++) {
        LunaTableNode* n = &mod->exports->nodes[i];
        if (n->key.type == LUNA_TSTRING) {
            LunaString* k = luna_val_string(n->key);
            if (k->len == strlen(symbol_name) &&
                memcmp(k->data, symbol_name, k->len) == 0) {
                return 0;  /* encontrado y accesible */
            }
        }
    }
    
    if (error_out) {
        snprintf(error_out, error_size,
                 "symbol '%s' is not public in module '%s'",
                 symbol_name, mod->name ? mod->name : "<unknown>");
    }
    return -1;
}

/*
 * Helper para el compilador: marca un símbolo como público en la tabla de exports.
 * Esto se llama al final de la compilación de un módulo Luna, para cada
 * declaración con 'pub'.
 */
int modules_export_symbol(struct LunaState* L, struct LunaTable* exports,
                          const char* name, LunaValue value) {
    LunaString* key = luna_string_newz(L, name);
    if (!key) return -1;
    
    luna_table_set(L, exports, luna_string_val(key), value);
    return 0;
}