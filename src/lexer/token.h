#ifndef LUNA_TOKEN_H
#define LUNA_TOKEN_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    LUNA_TOK_EOF = 0,
    LUNA_TOK_ERROR,
    LUNA_TOK_NEWLINE,
    LUNA_TOK_INDENT,
    LUNA_TOK_DEDENT,

    /* literales */
    LUNA_TOK_INTEGER,
    LUNA_TOK_FLOAT,
    LUNA_TOK_STRING,
    LUNA_TOK_IDENT,

    /* palabras reservadas */
    LUNA_TOK_LET,
    LUNA_TOK_CONST,
    LUNA_TOK_FN,
    LUNA_TOK_RETURN,
    LUNA_TOK_IF,
    LUNA_TOK_ELSE,
    LUNA_TOK_FOR,
    LUNA_TOK_IN,
    LUNA_TOK_WHILE,
    LUNA_TOK_BREAK,
    LUNA_TOK_CONTINUE,
    LUNA_TOK_IMPORT,
    LUNA_TOK_MOD,
    LUNA_TOK_PUB,
    LUNA_TOK_TRUE,
    LUNA_TOK_FALSE,
    LUNA_TOK_NIL,
    LUNA_TOK_AND,
    LUNA_TOK_OR,
    LUNA_TOK_NOT,
    LUNA_TOK_TRY,
    LUNA_TOK_CATCH,
    LUNA_TOK_THROW,

    /* operadores */
    LUNA_TOK_PLUS,
    LUNA_TOK_MINUS,
    LUNA_TOK_STAR,
    LUNA_TOK_SLASH,
    LUNA_TOK_PERCENT,
    LUNA_TOK_STAR_STAR,
    LUNA_TOK_EQ,
    LUNA_TOK_NEQ,
    LUNA_TOK_LT,
    LUNA_TOK_GT,
    LUNA_TOK_LE,
    LUNA_TOK_GE,
    LUNA_TOK_ASSIGN,
    LUNA_TOK_PLUS_ASSIGN,
    LUNA_TOK_MINUS_ASSIGN,
    LUNA_TOK_STAR_ASSIGN,
    LUNA_TOK_SLASH_ASSIGN,

    /* puntuación */
    LUNA_TOK_LPAREN,
    LUNA_TOK_RPAREN,
    LUNA_TOK_LBRACKET,
    LUNA_TOK_RBRACKET,
    LUNA_TOK_LBRACE,
    LUNA_TOK_RBRACE,
    LUNA_TOK_COMMA,
    LUNA_TOK_DOT,
    LUNA_TOK_COLON
} LunaTokenType;

typedef struct {
    LunaTokenType type;
    const char*   start;
    size_t        length;
    int           line;
    int           column;
    int64_t       int_value;
    double        float_value;
} LunaToken;

const char* luna_token_name(LunaTokenType type);

#endif /* LUNA_TOKEN_H */