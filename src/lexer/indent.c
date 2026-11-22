#include "indent.h"
#include "lexer_internal.h"

static bool is_blank_line(const LunaLexer* L) {
    size_t p = L->pos;
    while (p < L->length && (L->source[p] == ' ' || L->source[p] == '\t')) p++;
    if (p >= L->length) return true;
    char c = L->source[p];
    if (c == '\n' || c == '\r' || c == '#') return true;
    return false;
}

static void consume_blank_line(LunaLexer* L) {
    while (L->pos < L->length) {
        char c = L->source[L->pos];
        adv(L);
        if (c == '\n') return;
        if (c == '\r') {
            if (L->pos < L->length && L->source[L->pos] == '\n') adv(L);
            return;
        }
    }
}

LunaIndentResult luna_indent_handle(LunaLexer* L) {
    if (!L->at_line_start || L->paren_depth != 0) {
        return LUNA_INDENT_NONE;
    }

    if (is_blank_line(L)) {
        consume_blank_line(L);
        return LUNA_INDENT_CONTINUE;
    }

    int indent = 0;
    while (L->pos < L->length) {
        char c = L->source[L->pos];
        if (c == ' ') {
            indent++;
        } else if (c == '\t') {
            set_error(L, "tabs are not allowed for indentation");
            return LUNA_INDENT_NONE;
        } else {
            break;
        }
        adv(L);
    }

    int top = L->indent_stack[L->indent_depth - 1];

    if (indent > top) {
        if (L->indent_depth >= LUNA_LEXER_MAX_INDENT) {
            set_error(L, "indentation too deep");
            return LUNA_INDENT_NONE;
        }
        L->indent_stack[L->indent_depth++] = indent;
        L->pending_indent = true;
        L->at_line_start = false;
        /* El bucle principal se encargará de emitir el INDENT en la siguiente iteración */
        return LUNA_INDENT_CONTINUE;
    }

    if (indent < top) {
        while (L->indent_depth > 1 && L->indent_stack[L->indent_depth - 1] > indent) {
            L->indent_depth--;
            L->pending_dedents++;
        }
        if (L->indent_stack[L->indent_depth - 1] != indent) {
            set_error(L, "unindent does not match any outer indentation level");
            return LUNA_INDENT_NONE;
        }
        L->at_line_start = false;
        /* El bucle principal emitirá los DEDENTs pendientes */
        return LUNA_INDENT_CONTINUE;
    }

    /* Mismo nivel de indent: no se emite nada */
    L->at_line_start = false;
    return LUNA_INDENT_NONE;
}