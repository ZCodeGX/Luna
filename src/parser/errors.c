#include "parser_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t offset_from_linecol(LunaParser* P, int line, int col) {
    if (line <= 1 && col <= 1) return 0;
    uint32_t off = 0;
    int cur_line = 1, cur_col = 1;
    while (off < P->source_len && (cur_line < line || cur_col < col)) {
        if (P->source[off] == '\n') {
            cur_line++;
            cur_col = 1;
        } else {
            cur_col++;
        }
        off++;
    }
    return off;
}

static LunaSpan span_from_token(LunaParser* P, const LunaToken* tok) {
    if (!tok) return LUNA_SPAN_NONE;

    uint32_t start;
    if (tok->start && P->source &&
        tok->start >= P->source && tok->start <= P->source + P->source_len) {
        start = (uint32_t)(tok->start - P->source);
    } else {
        start = offset_from_linecol(P, tok->line, tok->column);
    }

    uint32_t end = start + (tok->length > 0 ? (uint32_t)tok->length : 1);
    if (end > P->source_len) end = (uint32_t)P->source_len;
    if (end < start) end = start;
    return (LunaSpan){ start, end };
}

static const char* token_description(LunaTokenType type) {
    switch (type) {
        case LUNA_TOK_EOF: return "fin de archivo";
        case LUNA_TOK_ERROR: return "error léxico";
        case LUNA_TOK_NEWLINE: return "salto de línea";
        case LUNA_TOK_INDENT: return "indentación";
        case LUNA_TOK_DEDENT: return "fin de bloque";
        case LUNA_TOK_INTEGER: return "entero";
        case LUNA_TOK_FLOAT: return "decimal";
        case LUNA_TOK_STRING: return "cadena";
        case LUNA_TOK_IDENT: return "identificador";
        default: return luna_token_name(type);
    }
}

static bool close_enough(LunaTokenType got, LunaTokenType want) {
    const char* a = luna_token_name(got);
    const char* b = luna_token_name(want);
    if (!a || !b || a[0] == '<' || b[0] == '<') return false;

    size_t la = strlen(a), lb = strlen(b);
    if (la < 2 || lb < 2) return false;

    int dist = 0;
    size_t i = 0, j = 0;
    while (i < la && j < lb && dist <= 1) {
        if (a[i] == b[j]) {
            i++;
            j++;
        } else if (i + 1 < la && a[i + 1] == b[j]) {
            i++;
            dist++;
        } else if (j + 1 < lb && a[i] == b[j + 1]) {
            j++;
            dist++;
        } else {
            i++;
            j++;
            dist++;
        }
    }
    dist += (int)((la - i) + (lb - j));
    return dist <= 1;
}

static bool emit_error(LunaParser* P, LunaSpan span, const LunaToken* tok,
                       const char* code, const char* fmt, ...) {
    if (P->panic_mode || !P->diags) return false;

    va_list ap;
    va_start(ap, fmt);
    char* msg = NULL;
    va_list cp;
    va_copy(cp, ap);
    int n = vsnprintf(NULL, 0, fmt, cp);
    va_end(cp);
    if (n >= 0) {
        msg = (char*)malloc((size_t)n + 1);
        if (msg) vsnprintf(msg, (size_t)n + 1, fmt, ap);
    }
    va_end(ap);

    LunaDiagnostic d = luna_diag_make(LUNA_DIAG_ERROR, code, span,
                                      "%s", msg ? msg : "<out of memory>");
    free(msg);

    if (tok) {
        luna_diag_add_note(&d, LUNA_SPAN_NONE, "token encontrado: %s",
                           token_description(tok->type));
    }

    if (P->diags->error_count == 0 && P->error_message[0] == 0) {
        snprintf(P->error_message, sizeof(P->error_message), "%s", d.message);
        P->error_line = tok ? tok->line : 1;
        P->error_column = tok ? tok->column : 1;
    }

    luna_diag_emit(P->diags, d);
    P->panic_mode = true;
    return true;
}

void fail_at(LunaParser* P, int line, int col, const char* msg) {
    LunaToken synthetic;
    memset(&synthetic, 0, sizeof(synthetic));
    synthetic.line = line;
    synthetic.column = col;
    emit_error(P, offset_from_linecol(P, line, col) == 0
                      ? LUNA_SPAN_NONE
                      : (LunaSpan){ offset_from_linecol(P, line, col),
                                    offset_from_linecol(P, line, col) + 1 },
               &synthetic, "E0002", "%s", msg);
}

void fail_current(LunaParser* P, const char* msg) {
    if (check(P, LUNA_TOK_ERROR) && P->current.start) {
        msg = (const char*)P->current.start;
    }
    emit_error(P, span_from_token(P, &P->current), &P->current, "E0002", "%s", msg);
}

bool expect(LunaParser* P, LunaTokenType t, const char* what) {
    if (match(P, t)) return true;

    bool emitted = emit_error(P, span_from_token(P, &P->current), &P->current,
                              "E0001", "se esperaba %s pero se encontró %s",
                              what, token_description(P->current.type));
    if (emitted && close_enough(P->current.type, t) &&
        P->diags && P->diags->count > 0) {
        LunaDiagnostic* last = &P->diags->items[P->diags->count - 1];
        luna_diag_set_help(last, "quizás quisiste decir '%s'", luna_token_name(t));
    }
    return false;
}

bool expect_block_end(LunaParser* P, const char* what) {
    match(P, LUNA_TOK_COLON);
    return expect(P, LUNA_TOK_NEWLINE, what);
}

static bool is_statement_start(LunaTokenType t) {
    switch (t) {
        case LUNA_TOK_LET:
        case LUNA_TOK_CONST:
        case LUNA_TOK_FN:
        case LUNA_TOK_IF:
        case LUNA_TOK_FOR:
        case LUNA_TOK_WHILE:
        case LUNA_TOK_RETURN:
        case LUNA_TOK_IMPORT:
        case LUNA_TOK_MOD:
        case LUNA_TOK_PUB:
        case LUNA_TOK_TRY:
        case LUNA_TOK_THROW:
        case LUNA_TOK_BREAK:
        case LUNA_TOK_CONTINUE:
            return true;
        default:
            return false;
    }
}

void synchronize(LunaParser* P) {
    P->panic_mode = false;
    while (!check(P, LUNA_TOK_EOF)) {
        if (check(P, LUNA_TOK_NEWLINE)) {
            advance(P);
            return;
        }
        if (check(P, LUNA_TOK_DEDENT) || check(P, LUNA_TOK_RBRACE) ||
            check(P, LUNA_TOK_RPAREN) || check(P, LUNA_TOK_RBRACKET)) {
            return;
        }
        if (is_statement_start(P->current.type)) return;
        advance(P);
    }
}

char* copy_token_text(const LunaToken* tok) {
    if (!tok->start || tok->length == 0) {
        char* s = (char*)malloc(1);
        if (s) s[0] = 0;
        return s;
    }
    char* s = (char*)malloc(tok->length + 1);
    if (!s) return NULL;
    memcpy(s, tok->start, tok->length);
    s[tok->length] = 0;
    return s;
}
