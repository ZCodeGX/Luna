#ifndef LUNA_NUMBER_H
#define LUNA_NUMBER_H

#include "lexer.h"
#include "token.h"

/*
 * Lexa un literal numérico (entero decimal/hex/binario, o float con exponente).
 * Asume que el lexer está posicionado en el primer dígito.
 *
 * En caso de error (número demasiado largo, hex inválido, etc.), llama a set_error
 * y deja el token sin inicializar correctamente.
 */
void luna_lex_number(LunaLexer* L, LunaToken* out);

#endif /* LUNA_NUMBER_H */