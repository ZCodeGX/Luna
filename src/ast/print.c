#include "ast.h"
#include "../lexer/token.h"
#include <stdio.h>

static void indent(int depth) {
    for (int i = 0; i < depth; i++) fputs("  ", stdout);
}

static const char* op_name(LunaTokenType t) { return luna_token_name(t); }

static void print_node(LunaNode* n, int d);

static void print_list(LunaNodeList* l, int d) {
    for (LunaNodeList* i = l; i; i = i->next) print_node(i->node, d);
}

static void print_node(LunaNode* n, int d) {
    if (!n) return;
    indent(d);
    switch (n->type) {
        case NODE_NIL:    puts("Nil"); break;
        case NODE_BOOL:   printf("Bool %s\n", n->as.boolean ? "true" : "false"); break;
        case NODE_INT:    printf("Int %lld\n", (long long)n->as.integer); break;
        case NODE_FLOAT:  printf("Float %g\n", n->as.floating); break;
        case NODE_STRING: printf("String \"%.*s\"\n", (int)n->as.string.len, n->as.string.data); break;
        case NODE_IDENT:  printf("Ident %s\n", n->as.string.data); break;
        case NODE_BINARY:
            printf("Binary %s\n", op_name(n->as.binary.op));
            print_node(n->as.binary.left, d + 1);
            print_node(n->as.binary.right, d + 1);
            break;
        case NODE_UNARY:
            printf("Unary %s\n", op_name(n->as.unary.op));
            print_node(n->as.unary.operand, d + 1);
            break;
        case NODE_CALL:
            puts("Call");
            print_node(n->as.call.callee, d + 1);
            for (LunaNodeList* i = n->as.call.args; i; i = i->next) print_node(i->node, d + 1);
            break;
        case NODE_INDEX:
            puts("Index");
            print_node(n->as.index.object, d + 1);
            print_node(n->as.index.index, d + 1);
            break;
        case NODE_FIELD:
            printf("Field .%s\n", n->as.field.name);
            print_node(n->as.field.object, d + 1);
            break;
        case NODE_LIST:
            puts("List");
            print_list(n->as.list.elements, d + 1);
            break;
        case NODE_TABLE:
            puts("Table");
            for (LunaTableEntry* e = n->as.table.entries; e; e = e->next) {
                indent(d + 1);
                printf("key %s =\n", e->key);
                print_node(e->value, d + 2);
            }
            break;
        case NODE_LAMBDA:
            puts("Lambda");
            indent(d + 1); puts("params:");
            print_list(n->as.lambda.params, d + 2);
            indent(d + 1); puts("body:");
            print_node(n->as.lambda.body, d + 2);
            break;
        case NODE_LET:
            printf("Let%s %s =\n", n->as.var.is_pub ? " pub" : "", n->as.var.name);
            print_node(n->as.var.init, d + 1);
            break;
        case NODE_CONST:
            printf("Const%s %s =\n", n->as.var.is_pub ? " pub" : "", n->as.var.name);
            print_node(n->as.var.init, d + 1);
            break;
        case NODE_FN:
            printf("Fn%s %s\n", n->as.fn.is_pub ? " pub" : "", n->as.fn.name);
            indent(d + 1); puts("params:");
            print_list(n->as.fn.params, d + 2);
            indent(d + 1); puts("body:");
            print_node(n->as.fn.body, d + 2);
            break;
        case NODE_RETURN:
            puts("Return");
            print_node(n->as.return_value, d + 1);
            break;
        case NODE_IF:
            puts("If");
            indent(d + 1); puts("cond:");   print_node(n->as.if_stmt.cond, d + 2);
            indent(d + 1); puts("then:");   print_node(n->as.if_stmt.then_block, d + 2);
            for (LunaNodeList* i = n->as.if_stmt.elifs; i; i = i->next) {
                indent(d + 1); puts("elif:");
                print_node(i->node->as.if_stmt.cond, d + 2);
                print_node(i->node->as.if_stmt.then_block, d + 2);
            }
            if (n->as.if_stmt.else_block) {
                indent(d + 1); puts("else:"); print_node(n->as.if_stmt.else_block, d + 2);
            }
            break;
        case NODE_FOR:
            puts("For");
            indent(d + 1); puts("vars:"); print_list(n->as.for_stmt.vars, d + 2);
            indent(d + 1); puts("iter:"); print_node(n->as.for_stmt.iter, d + 2);
            indent(d + 1); puts("body:"); print_node(n->as.for_stmt.body, d + 2);
            break;
        case NODE_WHILE:
            puts("While");
            print_node(n->as.while_stmt.cond, d + 1);
            print_node(n->as.while_stmt.body, d + 1);
            break;
        case NODE_BREAK:    puts("Break"); break;
        case NODE_CONTINUE: puts("Continue"); break;
        case NODE_IMPORT:
            puts("Import");
            print_list(n->as.import_stmt.path, d + 1);
            break;
        case NODE_MOD:
            printf("Mod%s %s\n", n->as.mod_stmt.is_pub ? " pub" : "", n->as.mod_stmt.name);
            break;
        case NODE_THROW:
            puts("Throw");
            print_node(n->as.throw_value, d + 1);
            break;
        case NODE_TRY:
            puts("Try");
            indent(d + 1); puts("body:"); print_node(n->as.try_stmt.body, d + 2);
            indent(d + 1); printf("catch %s:\n", n->as.try_stmt.catch_name);
            print_node(n->as.try_stmt.catch_body, d + 2);
            break;
        case NODE_ASSIGN:
            printf("Assign %s\n", op_name(n->as.assign.op));
            print_node(n->as.assign.target, d + 1);
            print_node(n->as.assign.value, d + 1);
            break;
        case NODE_EXPR_STMT:
            puts("ExprStmt");
            print_node(n->as.expr, d + 1);
            break;
        case NODE_BLOCK:
            puts("Block");
            print_list(n->as.block.stmts, d + 1);
            break;
        case NODE_PROGRAM:
            puts("Program");
            print_list(n->as.program.stmts, d + 1);
            break;
    }
}

void luna_ast_print(LunaNode* node) {
    print_node(node, 0);
}