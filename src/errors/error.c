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
    return L->error.has_error ? L->error.line : 0;
}

int luna_get_error_column(const struct LunaState* L) {
    return L->error.has_error ? L->error.column : 0;
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

static size_t error_append(char* out, size_t out_size, size_t used,
                           const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int need = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (need <= 0) return used;

    size_t total = (size_t)need;
    if (out && out_size > 0 && used + 1 < out_size) {
        size_t room = out_size - used - 1;
        va_start(ap, fmt);
        int wrote = vsnprintf(out + used, room + 1, fmt, ap);
        va_end(ap);
        total = (wrote > 0 && (size_t)wrote < room) ? (size_t)wrote : room;
    }
    return used + total;
}

size_t luna_format_error(const struct LunaState* L, char* out, size_t out_size) {
    const LunaError* err = &L->error;
    size_t used = 0;

    if (out && out_size > 0) out[0] = '\0';
    if (!err->has_error) return 0;

    used = error_append(out, out_size, used, "error: %s: %s\n",
                        luna_status_name(err->status), err->message);

    if (err->file[0] != '\0') {
        if (err->line > 0) {
            if (err->column > 0) {
                used = error_append(out, out_size, used, "  --> %s:%d:%d\n",
                                    err->file, err->line, err->column);
            } else {
                used = error_append(out, out_size, used, "  --> %s:%d\n",
                                    err->file, err->line);
            }
        } else {
            used = error_append(out, out_size, used, "  --> %s\n", err->file);
        }
    }

    if (err->traceback.count > 0) {
        used = error_append(out, out_size, used, "\nstack traceback:\n");
        for (int i = 0; i < err->traceback.count; i++) {
            const LunaTracebackFrame* f = &err->traceback.frames[i];
            const char* fn = f->function_name ? f->function_name : "<anonymous>";
            if (f->source_file && f->source_file[0]) {
                used = error_append(out, out_size, used, "    at %s (%s:%d)\n",
                                    fn, f->source_file, f->line);
            } else {
                used = error_append(out, out_size, used, "    at %s\n", fn);
            }
        }
    }

    return used;
}
