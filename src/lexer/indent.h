#ifndef LUNA_INDENT_H
#define LUNA_INDENT_H

#include "lexer.h"
#include "token.h"
#include <stdbool.h>

/*
 * Resultado de intentar procesar la indentación al inicio de una línea.
 */
typedef enum {
    LUNA_INDENT_NONE,     /* no estamos al inicio de línea, o paren_depth > 0 */
    LUNA_INDENT_CONTINUE, /* línea en blanco consumida; el bucle debe continuar */
    LUNA_INDENT_EMITS     /* se marcó INDENT o DEDENT pendiente; el bucle principal lo emitirá */
} LunaIndentResult;

/*
 * Maneja la lógica de INDENT / DEDENT al inicio de una línea lógica.
 *
 * - LUNA_INDENT_EMITS:    el bucle principal emitirá INDENT/DEDENT en la siguiente iteración.
 * - LUNA_INDENT_CONTINUE: el bucle debe saltar a la siguiente iteración (línea en blanco, etc.).
 * - LUNA_INDENT_NONE:     el lexer debe continuar con el resto de su lógica.
 */
LunaIndentResult luna_indent_handle(LunaLexer* L);

#endif /* LUNA_INDENT_H */