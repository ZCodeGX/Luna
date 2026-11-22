#ifndef LUNA_LEXER_H
#define LUNA_LEXER_H

#include "token.h"
#include <stdbool.h>
#include <stddef.h>

#define LUNA_LEXER_MAX_INDENT 256

typedef struct {
    const char* source;
    size_t      length;
    size_t      pos;
    int         line;
    int         column;

    /* Pila de indentación. indent_stack[0] == 0 siempre. */
    int  indent_stack[LUNA_LEXER_MAX_INDENT];
    int  indent_depth;
    int  pending_dedents;
    bool pending_indent;

    int  paren_depth;     /* > 0 dentro de ( [ { => se suprimen NEWLINE/indent */
    bool at_line_start;
    bool error;
    char error_message[256];
    int  error_line;
    int  error_column;
} LunaLexer;

void luna_lexer_init(LunaLexer* L, const char* source, size_t length);
bool luna_lexer_next(LunaLexer* L, LunaToken* out);
const char* luna_lexer_error(const LunaLexer* L);
void luna_lexer_free(LunaLexer* L);

#endif /* LUNA_LEXER_H */