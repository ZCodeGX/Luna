#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void indent(int depth) {
    for (int i = 0; i < depth; i++) fputs("  ", stdout);
}

static const char* op_name(LunaTokenType t) { return luna_token_name(t); }

static void dump_node(LunaNode* n, int d);

static void dump_list(LunaNodeList* l, int d) {
    for (LunaNodeList* i = l; i; i = i->next) dump_node(i->node, d);
}

static void dump_node(LunaNode* n, int d) {
    if (!n) return;
    indent(d);
    switch (n->type) {
        case NODE_NIL:    puts("Nil"); break;
        case NODE_BOOL:   printf("Bool %s\n", n->as.boolean ? "true" : "false"); break;
        case NODE_INT:    printf("Int %lld\n", (long long)n->as.integer); break;
        case NODE_FLOAT:  printf("Float %g\n", n->as.floating); break;
        case NODE_STRING: printf("String \"%s\"\n", n->as.string.data); break;
        case NODE_IDENT:  printf("Ident %s\n", n->as.string.data); break;
        case NODE_BINARY:
            printf("Binary %s\n", op_name(n->as.binary.op));
            dump_node(n->as.binary.left, d + 1);
            dump_node(n->as.binary.right, d + 1);
            break;
        case NODE_UNARY:
            printf("Unary %s\n", op_name(n->as.unary.op));
            dump_node(n->as.unary.operand, d + 1);
            break;
        case NODE_CALL:
            puts("Call");
            dump_node(n->as.call.callee, d + 1);
            for (LunaNodeList* i = n->as.call.args; i; i = i->next) dump_node(i->node, d + 1);
            break;
        case NODE_INDEX:
            puts("Index");
            dump_node(n->as.index.object, d + 1);
            dump_node(n->as.index.index, d + 1);
            break;
        case NODE_FIELD:
            printf("Field .%s\n", n->as.field.name);
            dump_node(n->as.field.object, d + 1);
            break;
        case NODE_LIST:
            puts("List");
            dump_list(n->as.list.elements, d + 1);
            break;
        case NODE_TABLE:
            puts("Table");
            for (LunaTableEntry* e = n->as.table.entries; e; e = e->next) {
                indent(d + 1);
                printf("key %s =\n", e->key);
                dump_node(e->value, d + 2);
            }
            break;
        case NODE_LAMBDA:
            puts("Lambda");
            indent(d + 1); puts("params:");
            dump_list(n->as.lambda.params, d + 2);
            indent(d + 1); puts("body:");
            dump_node(n->as.lambda.body, d + 2);
            break;
        case NODE_LET:
            printf("Let%s %s =\n", n->as.var.is_pub ? " pub" : "", n->as.var.name);
            dump_node(n->as.var.init, d + 1);
            break;
        case NODE_CONST:
            printf("Const%s %s =\n", n->as.var.is_pub ? " pub" : "", n->as.var.name);
            dump_node(n->as.var.init, d + 1);
            break;
        case NODE_FN:
            printf("Fn%s %s\n", n->as.fn.is_pub ? " pub" : "", n->as.fn.name);
            indent(d + 1); puts("params:");
            dump_list(n->as.fn.params, d + 2);
            indent(d + 1); puts("body:");
            dump_node(n->as.fn.body, d + 2);
            break;
        case NODE_RETURN:
            puts("Return");
            dump_node(n->as.return_value, d + 1);
            break;
        case NODE_IF:
            puts("If");
            indent(d + 1); puts("cond:");   dump_node(n->as.if_stmt.cond, d + 2);
            indent(d + 1); puts("then:");   dump_node(n->as.if_stmt.then_block, d + 2);
            for (LunaNodeList* i = n->as.if_stmt.elifs; i; i = i->next) {
                indent(d + 1); puts("elif:");
                dump_node(i->node->as.if_stmt.cond, d + 2);
                dump_node(i->node->as.if_stmt.then_block, d + 2);
            }
            if (n->as.if_stmt.else_block) {
                indent(d + 1); puts("else:"); dump_node(n->as.if_stmt.else_block, d + 2);
            }
            break;
        case NODE_FOR:
            puts("For");
            indent(d + 1); puts("vars:"); dump_list(n->as.for_stmt.vars, d + 2);
            indent(d + 1); puts("iter:"); dump_node(n->as.for_stmt.iter, d + 2);
            indent(d + 1); puts("body:"); dump_node(n->as.for_stmt.body, d + 2);
            break;
        case NODE_WHILE:
            puts("While");
            dump_node(n->as.while_stmt.cond, d + 1);
            dump_node(n->as.while_stmt.body, d + 1);
            break;
        case NODE_BREAK:    puts("Break"); break;
        case NODE_CONTINUE: puts("Continue"); break;
        case NODE_IMPORT:
            puts("Import");
            dump_list(n->as.import_stmt.path, d + 1);
            break;
        case NODE_MOD:
            printf("Mod%s %s\n", n->as.mod_stmt.is_pub ? " pub" : "", n->as.mod_stmt.name);
            break;
        case NODE_THROW:
            puts("Throw");
            dump_node(n->as.throw_value, d + 1);
            break;
        case NODE_TRY:
            puts("Try");
            indent(d + 1); puts("body:"); dump_node(n->as.try_stmt.body, d + 2);
            indent(d + 1); printf("catch %s:\n", n->as.try_stmt.catch_name);
            dump_node(n->as.try_stmt.catch_body, d + 2);
            break;
        case NODE_ASSIGN:
            printf("Assign %s\n", op_name(n->as.assign.op));
            dump_node(n->as.assign.target, d + 1);
            dump_node(n->as.assign.value, d + 1);
            break;
        case NODE_EXPR_STMT:
            puts("ExprStmt");
            dump_node(n->as.expr, d + 1);
            break;
        case NODE_BLOCK:
            puts("Block");
            dump_list(n->as.block.stmts, d + 1);
            break;
        case NODE_PROGRAM:
            puts("Program");
            dump_list(n->as.program.stmts, d + 1);
            break;
    }
}

int main(int argc, char** argv) {
    if (argc != 2) { fprintf(stderr, "usage: %s file.luna\n", argv[0]); return 2; }
    FILE* f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char* src = malloc((size_t)sz + 1);
    if (fread(src, 1, (size_t)sz, f) != (size_t)sz) { perror("read"); return 1; }
    src[sz] = 0;
    fclose(f);

    LunaParser P;
    LunaNode* prog = luna_parse(src, (size_t)sz, &P);
    if (!prog) {
        fprintf(stderr, "%d:%d: %s\n", P.error_line, P.error_column,
                luna_parser_error(&P) ? luna_parser_error(&P) : "(no message)");
        free(src);
        return 1;
    }
    dump_node(prog, 0);
    luna_ast_free(prog);
    free(src);
    return 0;
}