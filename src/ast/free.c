#include "ast.h"
#include <stdlib.h>

static void free_list(LunaNodeList* l);

static void free_node(LunaNode* n) {
    if (!n) return;
    switch (n->type) {
        case NODE_STRING: free(n->as.string.data); break;
        case NODE_IDENT:  free(n->as.string.data); break;
        case NODE_BINARY: free_node(n->as.binary.left); free_node(n->as.binary.right); break;
        case NODE_UNARY:  free_node(n->as.unary.operand); break;
        case NODE_CALL:   free_node(n->as.call.callee); free_list(n->as.call.args); break;
        case NODE_INDEX:  free_node(n->as.index.object); free_node(n->as.index.index); break;
        case NODE_FIELD:  free_node(n->as.field.object); free(n->as.field.name); break;
        case NODE_LIST:   free_list(n->as.list.elements); break;
        case NODE_TABLE: {
            LunaTableEntry* e = n->as.table.entries;
            while (e) {
                LunaTableEntry* next = e->next;
                free(e->key); free_node(e->value); free(e);
                e = next;
            }
            break;
        }
        case NODE_LAMBDA: free_list(n->as.lambda.params); free_node(n->as.lambda.body); break;
        case NODE_LET:
        case NODE_CONST:  free(n->as.var.name); free_node(n->as.var.init); break;
        case NODE_FN:     free(n->as.fn.name); free_list(n->as.fn.params); free_node(n->as.fn.body); break;
        case NODE_RETURN: free_node(n->as.return_value); break;
        case NODE_IF:
            free_node(n->as.if_stmt.cond);
            free_node(n->as.if_stmt.then_block);
            free_list(n->as.if_stmt.elifs);
            free_node(n->as.if_stmt.else_block);
            break;
        case NODE_FOR:
            free_list(n->as.for_stmt.vars);
            free_node(n->as.for_stmt.iter);
            free_node(n->as.for_stmt.body);
            break;
        case NODE_WHILE:
            free_node(n->as.while_stmt.cond);
            free_node(n->as.while_stmt.body);
            break;
        case NODE_IMPORT: free_list(n->as.import_stmt.path); break;
        case NODE_MOD:    free(n->as.mod_stmt.name); break;
        case NODE_THROW:  free_node(n->as.throw_value); break;
        case NODE_TRY:
            free_node(n->as.try_stmt.body);
            free(n->as.try_stmt.catch_name);
            free_node(n->as.try_stmt.catch_body);
            break;
        case NODE_ASSIGN:
            free_node(n->as.assign.target);
            free_node(n->as.assign.value);
            break;
        case NODE_EXPR_STMT: free_node(n->as.expr); break;
        case NODE_BLOCK:     free_list(n->as.block.stmts); break;
        case NODE_PROGRAM:   free_list(n->as.program.stmts); break;
        default: break;
    }
    free(n);
}

static void free_list(LunaNodeList* l) {
    while (l) {
        LunaNodeList* next = l->next;
        free_node(l->node);
        free(l);
        l = next;
    }
}

void luna_ast_free(LunaNode* root) { free_node(root); }

const char* luna_node_name(LunaNodeType t) {
    switch (t) {
        case NODE_NIL: return "Nil";
        case NODE_BOOL: return "Bool";
        case NODE_INT: return "Int";
        case NODE_FLOAT: return "Float";
        case NODE_STRING: return "String";
        case NODE_IDENT: return "Ident";
        case NODE_BINARY: return "Binary";
        case NODE_UNARY: return "Unary";
        case NODE_CALL: return "Call";
        case NODE_INDEX: return "Index";
        case NODE_FIELD: return "Field";
        case NODE_LIST: return "List";
        case NODE_TABLE: return "Table";
        case NODE_LAMBDA: return "Lambda";
        case NODE_LET: return "Let";
        case NODE_CONST: return "Const";
        case NODE_FN: return "Fn";
        case NODE_RETURN: return "Return";
        case NODE_IF: return "If";
        case NODE_FOR: return "For";
        case NODE_WHILE: return "While";
        case NODE_BREAK: return "Break";
        case NODE_CONTINUE: return "Continue";
        case NODE_IMPORT: return "Import";
        case NODE_MOD: return "Mod";
        case NODE_THROW: return "Throw";
        case NODE_TRY: return "Try";
        case NODE_ASSIGN: return "Assign";
        case NODE_EXPR_STMT: return "ExprStmt";
        case NODE_BLOCK: return "Block";
        case NODE_PROGRAM: return "Program";
    }
    return "?";
}