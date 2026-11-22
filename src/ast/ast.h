#ifndef LUNA_AST_H
#define LUNA_AST_H

#include "../lexer/token.h"
#include <stddef.h>
#include <stdint.h>

typedef struct LunaNode LunaNode;
typedef struct LunaNodeList LunaNodeList;
typedef struct LunaTableEntry LunaTableEntry;

typedef enum {
    /* expresiones */
    NODE_NIL, NODE_BOOL, NODE_INT, NODE_FLOAT, NODE_STRING,
    NODE_IDENT, NODE_BINARY, NODE_UNARY,
    NODE_CALL, NODE_INDEX, NODE_FIELD,
    NODE_LIST, NODE_TABLE, NODE_LAMBDA,
    /* sentencias */
    NODE_LET, NODE_CONST, NODE_FN, NODE_RETURN,
    NODE_IF, NODE_FOR, NODE_WHILE, NODE_BREAK, NODE_CONTINUE,
    NODE_IMPORT, NODE_MOD, NODE_THROW, NODE_TRY,
    NODE_ASSIGN, NODE_EXPR_STMT,
    /* estructurales */
    NODE_BLOCK, NODE_PROGRAM
} LunaNodeType;

struct LunaNodeList {
    LunaNode* node;
    LunaNodeList* next;
};

struct LunaTableEntry {
    char* key;
    LunaNode* value;
    LunaTableEntry* next;
};

struct LunaNode {
    LunaNodeType type;
    int line;
    int column;
    union {
        int       boolean;
        int64_t   integer;
        double    floating;
        struct { char* data; size_t len; } string;

        struct { LunaTokenType op; LunaNode* left; LunaNode* right; } binary;
        struct { LunaTokenType op; LunaNode* operand; } unary;

        struct { LunaNode* callee; LunaNodeList* args; } call;
        struct { LunaNode* object; LunaNode* index; } index;
        struct { LunaNode* object; char* name; } field;

        struct { LunaNodeList* elements; } list;
        struct { LunaTableEntry* entries; } table;

        struct { LunaNodeList* params; LunaNode* body; } lambda;

        struct { char* name; LunaNode* init; int is_pub; } var;
        struct { char* name; LunaNodeList* params; LunaNode* body; int is_pub; } fn;
        LunaNode* return_value;
        struct {
            LunaNode* cond;
            LunaNode* then_block;
            LunaNodeList* elifs;
            LunaNode* else_block;
        } if_stmt;
        struct { LunaNodeList* vars; LunaNode* iter; LunaNode* body; } for_stmt;
        struct { LunaNode* cond; LunaNode* body; } while_stmt;
        struct { LunaNodeList* path; } import_stmt;
        struct { char* name; int is_pub; } mod_stmt;
        LunaNode* throw_value;
        struct {
            LunaNode* body;
            char* catch_name;
            LunaNode* catch_body;
        } try_stmt;
        struct { LunaNode* target; LunaTokenType op; LunaNode* value; } assign;
        LunaNode* expr;
        struct { LunaNodeList* stmts; } block;
        struct { LunaNodeList* stmts; } program;
    } as;
};

/* constructores */
LunaNode* luna_node_new(LunaNodeType type, int line, int col);
LunaNode* luna_node_nil(int line, int col);
LunaNode* luna_node_bool(int v, int line, int col);
LunaNode* luna_node_int(int64_t v, int line, int col);
LunaNode* luna_node_float(double v, int line, int col);
LunaNode* luna_node_string(char* owned_data, size_t len, int line, int col);
LunaNode* luna_node_ident(char* owned_name, int line, int col);
LunaNode* luna_node_binary(LunaTokenType op, LunaNode* l, LunaNode* r, int line, int col);
LunaNode* luna_node_unary(LunaTokenType op, LunaNode* operand, int line, int col);
LunaNode* luna_node_call(LunaNode* callee, LunaNodeList* args, int line, int col);
LunaNode* luna_node_index(LunaNode* obj, LunaNode* idx, int line, int col);
LunaNode* luna_node_field(LunaNode* obj, char* owned_name, int line, int col);
LunaNode* luna_node_list(LunaNodeList* elems, int line, int col);
LunaNode* luna_node_table(LunaTableEntry* entries, int line, int col);
LunaNode* luna_node_lambda(LunaNodeList* params, LunaNode* body, int line, int col);
LunaNode* luna_node_block(LunaNodeList* stmts, int line, int col);
LunaNode* luna_node_program(LunaNodeList* stmts, int line, int col);

/* listas */
LunaNodeList* luna_node_list_append(LunaNodeList* tail, LunaNode* node);
LunaNodeList* luna_node_list_one(LunaNode* node);

/* liberar (recursivo) */
void luna_ast_free(LunaNode* node);

/* nombre del nodo (para debug) */
const char* luna_node_name(LunaNodeType type);

/* imprimir AST (para debug) */
void luna_ast_print(LunaNode* node);

#endif