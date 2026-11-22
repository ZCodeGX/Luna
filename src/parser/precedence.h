#ifndef LUNA_PRECEDENCE_H
#define LUNA_PRECEDENCE_H

#include "../lexer/token.h"

static inline int infix_bp(LunaTokenType t) {
    switch (t) {
        case LUNA_TOK_OR:  return 1;
        case LUNA_TOK_AND: return 2;
        case LUNA_TOK_EQ: case LUNA_TOK_NEQ:
        case LUNA_TOK_LT: case LUNA_TOK_GT:
        case LUNA_TOK_LE: case LUNA_TOK_GE: return 4;
        case LUNA_TOK_PLUS: case LUNA_TOK_MINUS: return 5;
        case LUNA_TOK_STAR: case LUNA_TOK_SLASH: case LUNA_TOK_PERCENT: return 6;
        case LUNA_TOK_STAR_STAR: return 7;
        default: return -1;
    }
}

static inline bool is_right_assoc(LunaTokenType t) {
    return t == LUNA_TOK_STAR_STAR;
}

#endif