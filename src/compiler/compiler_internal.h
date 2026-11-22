#ifndef LUNA_COMPILER_INTERNAL_H
#define LUNA_COMPILER_INTERNAL_H

#include "compiler.h"
#include "scope.h"
#include "../ast/ast.h"
#include "../bytecode/proto.h"
#include <stdint.h>

#define LUNA_MAX_PROTO_DEPTH 64
#define LUNA_MAX_LOCALS 255
#define LUNA_MAX_UPVALUES 255

struct LunaCompiler {
    /* Stack de protos (para funciones anidadas) */
    LunaProto* proto_stack[LUNA_MAX_PROTO_DEPTH];
    int proto_depth;

    /* Stack de scopes (para resolución de variables) */
    LunaScope scope_stack[LUNA_MAX_PROTO_DEPTH];
    int scope_depth;

    /* Símbolos exportados por el módulo */
    bool has_exports;

    /* Estado de error */
    bool failed;
    char error_message[256];
    int error_line;
    int error_column;
};

/* Funciones de compilación por tipo de nodo */
void compile_node(LunaCompiler* C, LunaNode* node);
void compile_expr(LunaCompiler* C, LunaNode* node, int reg);
void compile_stmt(LunaCompiler* C, LunaNode* node);

/* Helpers de error */
void compiler_error(LunaCompiler* C, int line, int col, const char* msg);
void compiler_error_at(LunaCompiler* C, LunaNode* node, const char* msg);

/* Gestión de protos */
void compiler_push_proto(LunaCompiler* C, const char* name);
LunaProto* compiler_pop_proto(LunaCompiler* C);
LunaProto* compiler_current_proto(LunaCompiler* C);

#endif