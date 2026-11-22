#include "parser_internal.h"
#include <stdlib.h>

LunaNode* parse_block(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    if (!expect(P, LUNA_TOK_INDENT, "indented block")) {
        return luna_node_block(NULL, line, col);
    }
    LunaNodeList* head = NULL;
    LunaNodeList* tail = NULL;
    for (;;) {
        while (check(P, LUNA_TOK_NEWLINE)) advance(P);
        if (check(P, LUNA_TOK_DEDENT) || check(P, LUNA_TOK_EOF)) break;
        LunaNode* stmt = parse_statement(P);
        if (P->failed) break;
        LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
        item->node = stmt;
        if (!head) head = item; else tail->next = item;
        tail = item;
        if (check(P, LUNA_TOK_NEWLINE)) advance(P);
    }
    match(P, LUNA_TOK_DEDENT);
    return luna_node_block(head, line, col);
}

static LunaNode* parse_if(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    LunaNode* cond = parse_expr(P, 0);
    if (!expect(P, LUNA_TOK_NEWLINE, "newline after condition")) return NULL;
    LunaNode* then_block = parse_block(P);

    LunaNodeList* elif_head = NULL;
    LunaNodeList* elif_tail = NULL;
    LunaNode* else_block = NULL;

    for (;;) {
        if (!check(P, LUNA_TOK_ELSE)) break;
        advance(P);
        if (check(P, LUNA_TOK_IF)) {
            int eline = P->current.line, ecol = P->current.column;
            advance(P);
            LunaNode* econd = parse_expr(P, 0);
            if (!expect(P, LUNA_TOK_NEWLINE, "newline after condition")) break;
            LunaNode* eblock = parse_block(P);
            LunaNode* einode = luna_node_new(NODE_IF, eline, ecol);
            einode->as.if_stmt.cond = econd;
            einode->as.if_stmt.then_block = eblock;
            LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
            item->node = einode;
            if (!elif_head) elif_head = item; else elif_tail->next = item;
            elif_tail = item;
        } else {
            if (!expect(P, LUNA_TOK_NEWLINE, "newline after else")) break;
            else_block = parse_block(P);
            break;
        }
    }

    LunaNode* n = luna_node_new(NODE_IF, line, col);
    n->as.if_stmt.cond = cond;
    n->as.if_stmt.then_block = then_block;
    n->as.if_stmt.elifs = elif_head;
    n->as.if_stmt.else_block = else_block;
    return n;
}

static LunaNode* parse_for(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    LunaNodeList* vars = NULL;
    LunaNodeList* tail = NULL;
    for (;;) {
        if (!check(P, LUNA_TOK_IDENT)) {
            fail_current(P, "expected loop variable");
            return NULL;
        }
        char* name = copy_token_text(&P->current);
        LunaNode* id = luna_node_ident(name, P->current.line, P->current.column);
        advance(P);
        LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
        item->node = id;
        if (!vars) vars = item; else tail->next = item;
        tail = item;
        if (!match(P, LUNA_TOK_COMMA)) break;
    }
    if (!expect(P, LUNA_TOK_IN, "'in'")) return NULL;
    LunaNode* iter = parse_expr(P, 0);
    if (!expect(P, LUNA_TOK_NEWLINE, "newline after for")) return NULL;
    LunaNode* body = parse_block(P);
    LunaNode* n = luna_node_new(NODE_FOR, line, col);
    n->as.for_stmt.vars = vars;
    n->as.for_stmt.iter = iter;
    n->as.for_stmt.body = body;
    return n;
}

static LunaNode* parse_while(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    LunaNode* cond = parse_expr(P, 0);
    if (!expect(P, LUNA_TOK_NEWLINE, "newline after while")) return NULL;
    LunaNode* body = parse_block(P);
    LunaNode* n = luna_node_new(NODE_WHILE, line, col);
    n->as.while_stmt.cond = cond;
    n->as.while_stmt.body = body;
    return n;
}

static LunaNode* parse_return(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    LunaNode* n = luna_node_new(NODE_RETURN, line, col);
    if (!check(P, LUNA_TOK_NEWLINE) && !check(P, LUNA_TOK_DEDENT) && !check(P, LUNA_TOK_EOF)) {
        n->as.return_value = parse_expr(P, 0);
    }
    return n;
}

static LunaNode* parse_import(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    LunaNodeList* head = NULL;
    LunaNodeList* tail = NULL;
    for (;;) {
        if (!check(P, LUNA_TOK_IDENT)) {
            fail_current(P, "expected module name");
            break;
        }
        char* name = copy_token_text(&P->current);
        LunaNode* id = luna_node_ident(name, P->current.line, P->current.column);
        advance(P);
        LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
        item->node = id;
        if (!head) head = item; else tail->next = item;
        tail = item;
        if (!match(P, LUNA_TOK_DOT)) break;
    }
    LunaNode* n = luna_node_new(NODE_IMPORT, line, col);
    n->as.import_stmt.path = head;
    return n;
}

static LunaNode* parse_mod(LunaParser* P, int is_pub) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    if (!check(P, LUNA_TOK_IDENT)) {
        fail_current(P, "expected module name");
        return NULL;
    }
    char* name = copy_token_text(&P->current);
    advance(P);
    LunaNode* n = luna_node_new(NODE_MOD, line, col);
    n->as.mod_stmt.name = name;
    n->as.mod_stmt.is_pub = is_pub;
    return n;
}

static LunaNode* parse_fn_decl(LunaParser* P, int is_pub) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    if (!check(P, LUNA_TOK_IDENT)) {
        fail_current(P, "expected function name");
        return NULL;
    }
    char* name = copy_token_text(&P->current);
    advance(P);
    if (!expect(P, LUNA_TOK_LPAREN, "'(' after function name")) { free(name); return NULL; }
    LunaNodeList* params = parse_param_list(P);
    if (!expect(P, LUNA_TOK_RPAREN, "')'")) { free(name); return NULL; }
    if (!expect(P, LUNA_TOK_NEWLINE, "newline before function body")) { free(name); return NULL; }
    LunaNode* body = parse_block(P);
    LunaNode* n = luna_node_new(NODE_FN, line, col);
    n->as.fn.name = name;
    n->as.fn.params = params;
    n->as.fn.body = body;
    n->as.fn.is_pub = is_pub;
    return n;
}

static bool is_assignable(const LunaNode* n) {
    return n && (n->type == NODE_IDENT || n->type == NODE_INDEX || n->type == NODE_FIELD);
}

static bool is_assign_op(LunaTokenType t) {
    return t == LUNA_TOK_ASSIGN
        || t == LUNA_TOK_PLUS_ASSIGN
        || t == LUNA_TOK_MINUS_ASSIGN
        || t == LUNA_TOK_STAR_ASSIGN
        || t == LUNA_TOK_SLASH_ASSIGN;
}

static LunaNode* parse_simple_statement(LunaParser* P) {
    if (check(P, LUNA_TOK_LET) || check(P, LUNA_TOK_CONST)) {
        int line = P->current.line, col = P->current.column;
        bool is_const = check(P, LUNA_TOK_CONST);
        advance(P);
        if (!check(P, LUNA_TOK_IDENT)) {
            fail_current(P, "expected variable name");
            return NULL;
        }
        char* name = copy_token_text(&P->current);
        advance(P);
        if (!expect(P, LUNA_TOK_ASSIGN, "'='")) { free(name); return NULL; }
        LunaNode* init = parse_expr(P, 0);
        LunaNode* n = luna_node_new(is_const ? NODE_CONST : NODE_LET, line, col);
        n->as.var.name = name;
        n->as.var.init = init;
        n->as.var.is_pub = 0;
        return n;
    }
    if (check(P, LUNA_TOK_RETURN))  return parse_return(P);
    if (check(P, LUNA_TOK_IMPORT))  return parse_import(P);
    if (check(P, LUNA_TOK_BREAK)) {
        int line = P->current.line, col = P->current.column;
        advance(P);
        return luna_node_new(NODE_BREAK, line, col);
    }
    if (check(P, LUNA_TOK_CONTINUE)) {
        int line = P->current.line, col = P->current.column;
        advance(P);
        return luna_node_new(NODE_CONTINUE, line, col);
    }
    if (check(P, LUNA_TOK_THROW)) {
        int line = P->current.line, col = P->current.column;
        advance(P);
        LunaNode* value = parse_expr(P, 0);
        LunaNode* n = luna_node_new(NODE_THROW, line, col);
        n->as.throw_value = value;
        return n;
    }

    int line = P->current.line, col = P->current.column;
    LunaNode* expr = parse_expr(P, 0);
    if (P->failed) return expr;

    if (is_assign_op(P->current.type)) {
        if (!is_assignable(expr)) {
            fail_at(P, line, col, "invalid assignment target");
            return expr;
        }
        LunaTokenType op = P->current.type;
        advance(P);
        LunaNode* value = parse_expr(P, 0);
        LunaNode* n = luna_node_new(NODE_ASSIGN, line, col);
        n->as.assign.target = expr;
        n->as.assign.op = op;
        n->as.assign.value = value;
        return n;
    }

    LunaNode* n = luna_node_new(NODE_EXPR_STMT, line, col);
    n->as.expr = expr;
    return n;
}

LunaNode* parse_statement(LunaParser* P) {
    if (check(P, LUNA_TOK_PUB)) {
        advance(P);
        if (check(P, LUNA_TOK_FN))  return parse_fn_decl(P, 1);
        if (check(P, LUNA_TOK_MOD)) return parse_mod(P, 1);
        if (check(P, LUNA_TOK_LET) || check(P, LUNA_TOK_CONST)) {
            LunaNode* n = parse_simple_statement(P);
            if (n) n->as.var.is_pub = 1;
            return n;
        }
        fail_current(P, "'pub' must precede fn, let, const or mod");
        return NULL;
    }

    if (check(P, LUNA_TOK_FN))  return parse_fn_decl(P, 0);
    if (check(P, LUNA_TOK_MOD)) return parse_mod(P, 0);
    if (check(P, LUNA_TOK_IF))  return parse_if(P);
    if (check(P, LUNA_TOK_FOR)) return parse_for(P);
    if (check(P, LUNA_TOK_WHILE)) return parse_while(P);
    if (check(P, LUNA_TOK_TRY)) {
        int line = P->current.line, col = P->current.column;
        advance(P);
        if (!expect(P, LUNA_TOK_NEWLINE, "newline after try")) return NULL;
        LunaNode* body = parse_block(P);
        if (!expect(P, LUNA_TOK_CATCH, "'catch'")) return NULL;
        if (!check(P, LUNA_TOK_IDENT)) {
            fail_current(P, "expected error name after 'catch'");
            return NULL;
        }
        char* cname = copy_token_text(&P->current);
        advance(P);
        if (!expect(P, LUNA_TOK_NEWLINE, "newline after catch")) { free(cname); return NULL; }
        LunaNode* cbody = parse_block(P);
        LunaNode* n = luna_node_new(NODE_TRY, line, col);
        n->as.try_stmt.body = body;
        n->as.try_stmt.catch_name = cname;
        n->as.try_stmt.catch_body = cbody;
        return n;
    }

    return parse_simple_statement(P);
}