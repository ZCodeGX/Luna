#ifndef LUNA_ERROR_H
#define LUNA_ERROR_H

#include <stddef.h>
#include <stdint.h>
#include "../../include/luna/luna_types.h"

struct LunaState;

/* Devuelve el nombre legible del código (ej: "LUNA_ERR_SYNTAX") */
const char* luna_status_name(LunaStatus status);

/* ============================================================
 * Traceback
 * ============================================================ */

#define LUNA_MAX_TRACEBACK_FRAMES 64

typedef struct LunaTracebackFrame {
    const char* function_name;   /* nombre de la función (o "<main>") */
    const char* source_file;     /* ruta del archivo fuente */
    int         line;            /* línea en el fuente */
    int         pc;              /* pc dentro del bytecode */
} LunaTracebackFrame;

typedef struct {
    LunaTracebackFrame frames[LUNA_MAX_TRACEBACK_FRAMES];
    int                count;
} LunaTraceback;

/* ============================================================
 * Error estructurado
 * ============================================================ */

typedef struct LunaError {
    LunaStatus   status;
    char         message[512];
    char         file[256];
    int          line;
    int          column;
    LunaTraceback traceback;
    int          has_error;       /* 1 si hay un error activo */
} LunaError;

/* ============================================================
 * API pública (operaciones sobre LunaState)
 * ============================================================ */

/* Consulta */
LunaStatus  luna_get_status(const struct LunaState* L);
const char* luna_get_error_message(const struct LunaState* L);
const char* luna_get_error_file(const struct LunaState* L);
int         luna_get_error_line(const struct LunaState* L);
int         luna_get_error_column(const struct LunaState* L);
const LunaTraceback* luna_get_traceback(const struct LunaState* L);
int         luna_has_error(const struct LunaState* L);

/* Limpieza */
void luna_clear_error(struct LunaState* L);

/* Formateo */
/*
 * Escribe una representación legible del error en `out`.
 * Devuelve el número de caracteres escritos (sin contar '\0').
 * Si `out` es NULL, solo devuelve el tamaño necesario.
 *
 * Formato:
 *   error: <status_name>: <message>
 *     --> <file>:<line>:<column>
 *     |
 *   N | <source_line>
 *     |       ^ <message>
 *     |
 *     = traceback:
 *         at function_name (file:line)
 *         at function_name (file:line)
 *         ...
 */
size_t luna_format_error(const struct LunaState* L, char* out, size_t out_size);

#endif /* LUNA_ERROR_H */
