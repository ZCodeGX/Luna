#include "number.h"
#include "lexer_internal.h"
#include <stdlib.h>
#include <string.h>

static bool is_digit(char c)       { return c >= '0' && c <= '9'; }
static bool is_hex_digit(char c)   { return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }

void luna_lex_number(LunaLexer* L, LunaToken* out) {
    int    line = L->line;
    int    col  = L->column;
    const char* start = L->source + L->pos;

    /* 0x / 0b */
    if (L->source[L->pos] == '0' && L->pos + 1 < L->length) {
        char nx = L->source[L->pos + 1];
        if (nx == 'x' || nx == 'X') {
            adv(L); adv(L);
            while (L->pos < L->length && is_hex_digit(L->source[L->pos])) adv(L);
            size_t n = (size_t)((L->source + L->pos) - start);
            if (n <= 2) { set_error(L, "invalid hex literal"); return; }
            char buf[64];
            if (n - 2 >= sizeof(buf)) { set_error(L, "number too long"); return; }
            memcpy(buf, start + 2, n - 2); buf[n - 2] = 0;
            fill_tok(out, LUNA_TOK_INTEGER, start, n, line, col);
            out->int_value = (int64_t)strtoll(buf, NULL, 16);
            return;
        }
        if (nx == 'b' || nx == 'B') {
            adv(L); adv(L);
            while (L->pos < L->length && (L->source[L->pos] == '0' || L->source[L->pos] == '1')) adv(L);
            size_t n = (size_t)((L->source + L->pos) - start);
            if (n <= 2) { set_error(L, "invalid binary literal"); return; }
            char buf[64];
            if (n - 2 >= sizeof(buf)) { set_error(L, "number too long"); return; }
            memcpy(buf, start + 2, n - 2); buf[n - 2] = 0;
            fill_tok(out, LUNA_TOK_INTEGER, start, n, line, col);
            out->int_value = (int64_t)strtoll(buf, NULL, 2);
            return;
        }
    }

    while (L->pos < L->length && is_digit(L->source[L->pos])) adv(L);
    bool is_float = false;

    if (L->pos < L->length && L->source[L->pos] == '.'
        && L->pos + 1 < L->length && is_digit(L->source[L->pos + 1])) {
        is_float = true;
        adv(L);
        while (L->pos < L->length && is_digit(L->source[L->pos])) adv(L);
    }

    if (L->pos < L->length && (L->source[L->pos] == 'e' || L->source[L->pos] == 'E')) {
        is_float = true;
        adv(L);
        if (L->pos < L->length && (L->source[L->pos] == '+' || L->source[L->pos] == '-')) adv(L);
        while (L->pos < L->length && is_digit(L->source[L->pos])) adv(L);
    }

    size_t n = (size_t)((L->source + L->pos) - start);
    char buf[128];
    if (n >= sizeof(buf)) { set_error(L, "number too long"); return; }
    memcpy(buf, start, n); buf[n] = 0;

    if (is_float) {
        fill_tok(out, LUNA_TOK_FLOAT, start, n, line, col);
        out->float_value = strtod(buf, NULL);
    } else {
        fill_tok(out, LUNA_TOK_INTEGER, start, n, line, col);
        out->int_value = (int64_t)strtoll(buf, NULL, 10);
    }
}