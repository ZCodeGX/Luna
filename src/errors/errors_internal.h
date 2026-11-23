#ifndef LUNA_ERRORS_INTERNAL_H
#define LUNA_ERRORS_INTERNAL_H

#include "error.h"
#include <stdarg.h>

struct LunaState;

/* ============================================================
 * API interna (usada por el runtime)
 * ============================================================ */

/* Establece un error con formato printf-like */
void luna_error_set(struct LunaState* L, LunaStatus status,
                    const char* file, int line, int column,
                    const char* fmt, ...);

/* Versión con va_list */
void luna_error_setv(struct LunaState* L, LunaStatus status,
                     const char* file, int line, int column,
                     const char* fmt, va_list ap);

/* Establece un error simple (sin file/line/column) */
void luna_error_set_simple(struct LunaState* L, LunaStatus status,
                           const char* fmt, ...);

/* Limpia el error actual (para recuperarse de una excepción capturada) */
void luna_error_clear(struct LunaState* L);

/* ============================================================
 * Traceback (API interna)
 * ============================================================ */

/* Añade un frame al traceback del error actual */
void luna_traceback_push(struct LunaState* L,
                         const char* function_name,
                         const char* source_file,
                         int line, int pc);

/* Limpia el traceback */
void luna_traceback_clear(struct LunaState* L);

/* ============================================================
 * Acceso directo al error (para código interno)
 * ============================================================ */

/* Obtiene puntero al error embebido en el estado */
LunaError* luna_state_error(struct LunaState* L);
const LunaError* luna_state_error_const(const struct LunaState* L);

#endif /* LUNA_ERRORS_INTERNAL_H */
