#ifndef LUNA_LEXER_INTERNAL_H
#define LUNA_LEXER_INTERNAL_H

#include "lexer.h"
#include <stdio.h> 
#include <string.h>

/* ---------- avance con tracking de línea/columna ---------- */
static inline void adv(LunaLexer* L) {
    if (L->pos >= L->length) return;
    char c = L->source[L->pos];
    if (c == '\n') {
        L->line++;
        L->column = 1;
    } else if (c == '\r') {
        if (L->pos + 1 < L->length && L->source[L->pos + 1] == '\n') {
            L->column++;
        } else {
            L->line++;
            L->column = 1;
        }
    } else {
        L->column++;
    }
    L->pos++;
}

/* ---------- reporte de errores ---------- */
static inline void set_error(LunaLexer* L, const char* msg) {
    if (L->error) return;
    L->error = true;
    snprintf(L->error_message, sizeof(L->error_message), "%s", msg);
    L->error_line   = L->line;
    L->error_column = L->column;
}

static inline void set_error_char(LunaLexer* L, char c) {
    if (L->error) return;
    L->error = true;
    snprintf(L->error_message, sizeof(L->error_message),
             "unexpected character '%c' (0x%02X)", c, (unsigned char)c);
    L->error_line   = L->line;
    L->error_column = L->column;
}

/* ---------- emisión de tokens ---------- */
static inline void fill_tok(LunaToken* out, LunaTokenType t, const char* s, size_t n,
                            int line, int col) {
    out->type        = t;
    out->start       = s;
    out->length      = n;
    out->line        = line;
    out->column      = col;
    out->int_value   = 0;
    out->float_value = 0.0;
}

static inline void fill_tok_empty(LunaToken* out, LunaTokenType t, int line, int col) {
    out->type        = t;
    out->start       = NULL;
    out->length      = 0;
    out->line        = line;
    out->column      = col;
    out->int_value   = 0;
    out->float_value = 0.0;
}

#endif /* LUNA_LEXER_INTERNAL_H */