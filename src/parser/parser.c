#include "parser_internal.h"
#include <stdlib.h>
#include <string.h>

void luna_parser_init(LunaParser* P, const char* source, size_t length) {
    memset(P, 0, sizeof(*P));
    luna_lexer_init(&P->lexer, source, length);
    advance(P);
}

void luna_parser_destroy(LunaParser* P) {
    (void)P;
}

LunaNode* luna_parser_parse_program(LunaParser* P) {
    while (check(P, LUNA_TOK_NEWLINE)) advance(P);
    if (check(P, LUNA_TOK_INDENT)) {
        fail_current(P, "unexpected indentation at file start");
        return NULL;
    }
    LunaNodeList* head = NULL;
    LunaNodeList* tail = NULL;
    while (!P->failed && !check(P, LUNA_TOK_EOF) && !check(P, LUNA_TOK_ERROR)) {
        LunaNode* stmt = parse_statement(P);
        if (P->failed) { luna_ast_free(luna_node_program(head, 1, 1)); return NULL; }
        if (stmt) {
            LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
            item->node = stmt;
            if (!head) head = item; else tail->next = item;
            tail = item;
        }
        while (check(P, LUNA_TOK_NEWLINE)) advance(P);
    }
    if (P->failed || check(P, LUNA_TOK_ERROR)) {
        luna_ast_free(luna_node_program(head, 1, 1));
        return NULL;
    }
    return luna_node_program(head, 1, 1);
}

LunaNode* luna_parse(const char* source, size_t length, LunaParser* P_out) {
    luna_parser_init(P_out, source, length);
    LunaNode* prog = luna_parser_parse_program(P_out);
    return prog;
}

const char* luna_parser_error(const LunaParser* P) {
    return P->failed ? P->error_message : NULL;
}