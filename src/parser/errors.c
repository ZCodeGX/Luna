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

static void linecol_from_offset(const LunaParser* P, uint32_t off,
                                int* line, int* col) {
    int cur_line = 1, cur_col = 1;
    for (uint32_t i = 0; i < off && i < P->source_len; i++) {
        if (P->source[i] == '\n') {
            cur_line++;
            cur_col = 1;
        } else {
            cur_col++;
        }
    }
    *line = cur_line;
    *col = cur_col;
}

LunaSpan token_span(LunaParser* P, const LunaToken* tok) {
    if (!tok) return LUNA_SPAN_NONE;
    if (tok->type == LUNA_TOK_ERROR) {
        uint32_t start = offset_from_linecol(P, tok->line, tok->column);
        uint32_t end = start + 1;
        if (end > P->source_len) end = (uint32_t)P->source_len;
        return (LunaSpan){ start, end };
    }

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

static LunaSpan span_after_previous(LunaParser* P) {
    if (P->has_previous && P->previous.start && P->source &&
        P->previous.start >= P->source &&
        P->previous.start <= P->source + P->source_len) {
        uint32_t off = (uint32_t)(P->previous.start + P->previous.length - P->source);
        if (off > P->source_len) off = (uint32_t)P->source_len;
        return (LunaSpan){ off, off };
    }
    LunaSpan span = token_span(P, &P->current);
    return (LunaSpan){ span.start, span.start };
}

static const char* token_description(LunaTokenType type) {
    switch (type) {
        case LUNA_TOK_EOF: return "end of file";
        case LUNA_TOK_ERROR: return "lexer error";
        case LUNA_TOK_NEWLINE: return "newline";
        case LUNA_TOK_INDENT: return "indent";
        case LUNA_TOK_DEDENT: return "dedent";
        case LUNA_TOK_INTEGER: return "integer";
        case LUNA_TOK_FLOAT: return "float";
        case LUNA_TOK_STRING: return "string";
        case LUNA_TOK_IDENT: return "identifier";
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

static bool emit_error(LunaParser* P, LunaErrorType type, LunaSpan span,
                       const LunaToken* tok, const char* help,
                       const char* fmt, ...) {
    if (P->failed || P->panic_mode || !P->diags) return false;

    char* msg = NULL;
    va_list ap;
    va_start(ap, fmt);
    va_list cp;
    va_copy(cp, ap);
    int n = vsnprintf(NULL, 0, fmt, cp);
    va_end(cp);
    if (n >= 0) {
        msg = (char*)malloc((size_t)n + 1);
        if (msg) vsnprintf(msg, (size_t)n + 1, fmt, ap);
    }
    va_end(ap);

    LunaDiagnostic d = luna_diag_make(LUNA_DIAG_ERROR, type, span,
                                      "%s", msg ? msg : "invalid syntax");
    free(msg);
    if (help) luna_diag_set_help(&d, "%s", help);
    if (tok && tok->type != LUNA_TOK_ERROR) {
        luna_diag_add_note(&d, LUNA_SPAN_NONE, "found %s", token_description(tok->type));
    }

    if (P->error_message[0] == 0) {
        snprintf(P->error_message, sizeof(P->error_message), "%s", d.message);
        if (tok) {
            P->error_line = tok->line;
            P->error_column = tok->column;
        } else {
            linecol_from_offset(P, span.start, &P->error_line, &P->error_column);
        }
    }

    luna_diag_emit(P->diags, d);
    P->error_count++;
    P->panic_mode = true;
    return true;
}

void syntax_error(LunaParser* P, LunaSpan span, const char* help, const char* fmt, ...) {
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    emit_error(P, LUNA_ERROR_SYNTAX, span, NULL, help, "%s", msg);
}

void indentation_error(LunaParser* P, LunaSpan span, const char* help, const char* fmt, ...) {
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    emit_error(P, LUNA_ERROR_INDENTATION, span, NULL, help, "%s", msg);
}

void fail_at(LunaParser* P, int line, int col, const char* msg) {
    uint32_t off = offset_from_linecol(P, line, col);
    LunaSpan span = { off, off + 1 };
    syntax_error(P, span, NULL, "%s", msg);
}

void fail_current(LunaParser* P, const char* msg) {
    LunaErrorType type = LUNA_ERROR_SYNTAX;
    const char* help = NULL;
    if (check(P, LUNA_TOK_ERROR)) {
        P->panic_mode = false;
        if (P->current.start) {
            msg = (const char*)P->current.start;
        }
        if (strstr(msg, "indent") || strstr(msg, "dedent") || strstr(msg, "tab") ||
            strstr(msg, "unindent")) {
            type = LUNA_ERROR_INDENTATION;
            if (strstr(msg, "tabs")) {
                help = "use spaces only (4 spaces per level)";
            } else {
                help = "check your indentation; use 4 spaces per level";
            }
        }
    }
    emit_error(P, type, token_span(P, &P->current), &P->current, help, "%s", msg);
    if (check(P, LUNA_TOK_ERROR)) P->failed = true;
}

bool expect(LunaParser* P, LunaTokenType t, const char* what) {
    if (match(P, t)) return true;

    bool emitted = emit_error(P, LUNA_ERROR_SYNTAX, token_span(P, &P->current),
                              &P->current, NULL,
                              "expected %s but found %s", what,
                              token_description(P->current.type));
    if (emitted && close_enough(P->current.type, t) &&
        P->diags && P->diags->count > 0) {
        LunaDiagnostic* last = &P->diags->items[P->diags->count - 1];
        luna_diag_set_help(last, "did you mean '%s'?", luna_token_name(t));
    }
    return false;
}

bool expect_help(LunaParser* P, LunaTokenType t, const char* what, const char* help) {
    int before = P->diags ? P->diags->count : 0;
    bool ok = expect(P, t, what);
    if (!ok && help && P->diags && P->diags->count > before) {
        luna_diag_set_help(&P->diags->items[P->diags->count - 1], "%s", help);
    }
    return ok;
}

bool expect_block_end(LunaParser* P, const char* context) {
    if (!match(P, LUNA_TOK_COLON)) {
        syntax_error(P, span_after_previous(P),
                     "insert ':' at the end of the line",
                     "expected ':' after %s", context);
    }
    return expect(P, LUNA_TOK_NEWLINE, "newline after ':'");
}

void require_statement_end(LunaParser* P) {
    if (P->failed || P->panic_mode) return;

    if (P->after_block_expr) {
        P->after_block_expr = false;
        return;
    }

    LunaTokenType t = P->current.type;
    if (t == LUNA_TOK_NEWLINE || t == LUNA_TOK_DEDENT || t == LUNA_TOK_EOF) return;

    if (t == LUNA_TOK_COLON) {
        syntax_error(P, token_span(P, &P->current),
                     "remove ':' after expression; ':' is only valid after block headers",
                     "invalid syntax");
        return;
    }

    syntax_error(P, token_span(P, &P->current),
                 "insert a newline or ';' between statements",
                 "invalid syntax");
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
    while (!P->failed && !check(P, LUNA_TOK_EOF) && !check(P, LUNA_TOK_ERROR)) {
        if (check(P, LUNA_TOK_NEWLINE)) {
            advance(P);
            return;
        }
        if (check(P, LUNA_TOK_DEDENT)) return;
        if (check(P, LUNA_TOK_RBRACE) || check(P, LUNA_TOK_RPAREN) ||
            check(P, LUNA_TOK_RBRACKET)) {
            advance(P);
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
