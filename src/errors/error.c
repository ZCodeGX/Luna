#include "error.h"
#include "errors_internal.h"
#include "../runtime/runtime_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* ============================================================
 * Acceso al error embebido en LunaState
 * ============================================================ */

LunaError* luna_state_error(struct LunaState* L) {
    return &L->error;
}

const LunaError* luna_state_error_const(const struct LunaState* L) {
    return &L->error;
}

/* ============================================================
 * API pública: consultas
 * ============================================================ */

LunaStatus luna_get_status(const struct LunaState* L) {
    return L->error.status;
}

const char* luna_get_error_message(const struct LunaState* L) {
    if (!L->error.has_error) return NULL;
    return L->error.message;
}

const char* luna_get_error_file(const struct LunaState* L) {
    if (!L->error.has_error || L->error.file[0] == '\0') return NULL;
    return L->error.file;
}

int luna_get_error_line(const struct LunaState* L) {
    return L->error.line;
}

int luna_get_error_column(const struct LunaState* L) {
    return L->error.column;
}

const LunaTraceback* luna_get_traceback(const struct LunaState* L) {
    if (!L->error.has_error) return NULL;
    return &L->error.traceback;
}

int luna_has_error(const struct LunaState* L) {
    return L->error.has_error;
}

/* ============================================================
 * API interna: establecimiento de errores
 * ============================================================ */

void luna_error_setv(struct LunaState* L, LunaStatus status,
                     const char* file, int line, int column,
                     const char* fmt, va_list ap) {
    LunaError* err = &L->error;

    /* No sobrescribir un error ya establecido (el primero es el más informativo) */
    if (err->has_error) return;

    err->status = status;
    L->status = status;
    err->line = line;
    err->column = column;
    err->has_error = 1;

    if (file && file[0]) {
        snprintf(err->file, sizeof(err->file), "%s", file);
    } else {
        err->file[0] = '\0';
    }

    vsnprintf(err->message, sizeof(err->message), fmt, ap);

    /* Traceback vacío al principio */
    err->traceback.count = 0;
}

void luna_error_set(struct LunaState* L, LunaStatus status,
                    const char* file, int line, int column,
                    const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    luna_error_setv(L, status, file, line, column, fmt, ap);
    va_end(ap);
}

void luna_error_set_simple(struct LunaState* L, LunaStatus status,
                           const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    luna_error_setv(L, status, NULL, 0, 0, fmt, ap);
    va_end(ap);
}

void luna_error_clear(struct LunaState* L) {
    memset(&L->error, 0, sizeof(L->error));
    L->status = LUNA_OK;
    L->error_message[0] = '\0';
    L->error_file[0] = '\0';
    L->error_line = 0;
}

/* ============================================================
 * Formateo de errores
 * ============================================================ */

size_t luna_format_error(const struct LunaState* L, char* out, size_t out_size) {
    const LunaError* err = &L->error;

    if (!err->has_error) {
        if (out && out_size > 0) out[0] = '\0';
        return 0;
    }

    /* Si out es NULL, solo calcular tamaño */
    if (!out) {
        /* Estimación conservadora */
        return 1024;
    }

    char buf[2048];
    int n = 0;

    /* Cabecera: error: <tipo>: <mensaje> */
    n += snprintf(buf + n, sizeof(buf) - (size_t)n,
                  "error: %s: %s\n",
                  luna_status_name(err->status),
                  err->message);

    /* Ubicación */
    if (err->file[0] != '\0') {
        if (err->line > 0) {
            if (err->column > 0) {
                n += snprintf(buf + n, sizeof(buf) - (size_t)n,
                              "  --> %s:%d:%d\n",
                              err->file, err->line, err->column);
            } else {
                n += snprintf(buf + n, sizeof(buf) - (size_t)n,
                              "  --> %s:%d\n",
                              err->file, err->line);
            }
        } else {
            n += snprintf(buf + n, sizeof(buf) - (size_t)n,
                          "  --> %s\n", err->file);
        }
    }

    /* Traceback */
    if (err->traceback.count > 0) {
        n += snprintf(buf + n, sizeof(buf) - (size_t)n,
                      "\nstack traceback:\n");
        for (int i = 0; i < err->traceback.count; i++) {
            const LunaTracebackFrame* f = &err->traceback.frames[i];
            if (f->source_file && f->source_file[0]) {
                n += snprintf(buf + n, sizeof(buf) - (size_t)n,
                              "    at %s (%s:%d)\n",
                              f->function_name ? f->function_name : "<anonymous>",
                              f->source_file, f->line);
            } else {
                n += snprintf(buf + n, sizeof(buf) - (size_t)n,
                              "    at %s\n",
                              f->function_name ? f->function_name : "<anonymous>");
            }
        }
    }

    /* Copiar al buffer de salida */
    size_t total = (size_t)n;
    if (total >= out_size) total = out_size - 1;
    memcpy(out, buf, total);
    out[total] = '\0';

    return total;
}
