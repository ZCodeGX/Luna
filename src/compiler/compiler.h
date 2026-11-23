#ifndef LUNA_COMPILER_H
#define LUNA_COMPILER_H

#include "../ast/ast.h"
#include "../bytecode/proto.h"
#include <stdbool.h>

typedef struct LunaCompiler LunaCompiler;

/*
 * Compila un AST completo (NODE_PROGRAM) a bytecode.
 * Devuelve NULL en error. El proto devuelto es propiedad del llamante.
 */
LunaProto* luna_compile(LunaNode* program);

/*
 * Compila una fuente y reporta errores detallados en error_out.
 */
struct LunaState;
LunaProto* luna_compile_source(struct LunaState* L, const char* source,
                               size_t length, const char* filename,
                               char* error_out, size_t error_size);

/* Para tests: obtener el mensaje de error */
const char* luna_compiler_error(LunaCompiler* C);

#endif