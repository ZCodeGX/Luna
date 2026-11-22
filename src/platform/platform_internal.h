#ifndef LUNA_PLATFORM_INTERNAL_H
#define LUNA_PLATFORM_INTERNAL_H

#include "platform.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* Helper para copiar mensajes de error con formato printf */
static inline void platform_set_error(char* error_out, size_t error_size,
                                      const char* fmt, ...) {
    if (!error_out || error_size == 0) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(error_out, error_size, fmt, ap);
    va_end(ap);
}

/* strdup portable (no POSIX) */
static inline char* platform_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len + 1);
    return out;
}

#endif /* LUNA_PLATFORM_INTERNAL_H */
