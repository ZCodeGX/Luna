#ifndef LUNA_PARSER_H
#define LUNA_PARSER_H

#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    LunaLexer lexer;
    LunaToken current;
    bool      has_current;
    bool      failed;
    char      error_message[256];
    int       error_line;
    int       error_column;
} LunaParser;

LunaNode* luna_parse(const char* source, size_t length, LunaParser* P_out);
const char* luna_parser_error(const LunaParser* P);

void luna_parser_init(LunaParser* P, const char* source, size_t length);
void luna_parser_destroy(LunaParser* P);
LunaNode* luna_parser_parse_program(LunaParser* P);

#endif