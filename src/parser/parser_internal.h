#ifndef LUNA_PARSER_INTERNAL_H
#define LUNA_PARSER_INTERNAL_H

#include "parser.h"
#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include <stdbool.h>

/* Forward decls de funciones que se llaman entre archivos */
LunaNode* parse_expr(LunaParser* P, int min_bp);
LunaNode* parse_block(LunaParser* P);
LunaNode* parse_block_after(LunaParser* P, const char* context);
LunaNode* parse_statement(LunaParser* P);
LunaNodeList* parse_param_list(LunaParser* P);
LunaNodeList* parse_arg_list(LunaParser* P);

/* Helpers de infraestructura (inline para performance) */
static inline void advance(LunaParser* P) {
    if (P->has_current) {
        P->previous = P->current;
        P->has_previous = true;
    }
    if (!luna_lexer_next(&P->lexer, &P->current)) {
        /* EOF or ERROR is already in current */
    }
    P->has_current = true;
}

static inline bool check(LunaParser* P, LunaTokenType t) {
    return P->current.type == t;
}

static inline bool match(LunaParser* P, LunaTokenType t) {
    if (check(P, t)) { advance(P); return true; }
    return false;
}

/* Funciones de error y utilidades (implementadas en errors.c) */
void fail_at(LunaParser* P, int line, int col, const char* msg);
void fail_current(LunaParser* P, const char* msg);
LunaSpan token_span(LunaParser* P, const LunaToken* tok);
void syntax_error(LunaParser* P, LunaSpan span, const char* help, const char* fmt, ...);
void indentation_error(LunaParser* P, LunaSpan span, const char* help, const char* fmt, ...);
bool expect(LunaParser* P, LunaTokenType t, const char* what);
bool expect_help(LunaParser* P, LunaTokenType t, const char* what, const char* help);
bool expect_block_end(LunaParser* P, const char* context);
void require_statement_end(LunaParser* P);
void synchronize(LunaParser* P);
char* copy_token_text(const LunaToken* tok);

#endif