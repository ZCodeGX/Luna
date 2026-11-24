#include "parser_internal.h"
#include <stdlib.h>
#include <string.h>

LunaNode* parse_block_after(LunaParser* P, const char* context) {
    int line = P->has_previous ? P->previous.line : P->current.line;
    int col = P->has_previous ? P->previous.column : P->current.column;
    if (!check(P, LUNA_TOK_INDENT)) {
        if (check(P, LUNA_TOK_ERROR)) return luna_node_block(NULL, line, col);
        indentation_error(P, token_span(P, &P->current),
                          "indent the block with 4 spaces",
                          "expected an indented block after %s on line %d",
                          context, line);
        return luna_node_block(NULL, line, col);
    }
    advance(P);
    LunaNodeList* head = NULL;
    LunaNodeList* tail = NULL;
    for (;;) {
        while (check(P, LUNA_TOK_NEWLINE)) advance(P);
        if (P->failed || check(P, LUNA_TOK_DEDENT) || check(P, LUNA_TOK_EOF)) break;

        LunaTokenType before_type = P->current.type;
        const char* before_start = P->current.start;
        size_t before_length = P->current.length;

        LunaNode* stmt = parse_statement(P);
        if (stmt) {
            LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
            item->node = stmt;
            if (!head) head = item; else tail->next = item;
            tail = item;
        }

        if (P->panic_mode) synchronize(P);
        if (P->current.type == before_type && P->current.start == before_start &&
            P->current.length == before_length &&
            !check(P, LUNA_TOK_DEDENT) && !check(P, LUNA_TOK_EOF)) {
            advance(P);
        }
        if (check(P, LUNA_TOK_NEWLINE)) advance(P);
    }
    match(P, LUNA_TOK_DEDENT);
    return luna_node_block(head, line, col);
}

LunaNode* parse_block(LunaParser* P) {
    return parse_block_after(P, "a statement");
}

static LunaNode* parse_if(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    LunaNode* n = luna_node_new(NODE_IF, line, col);
    LunaNode* cond = parse_expr(P, 0);
    if (n) n->as.if_stmt.cond = cond;
    if (!expect_block_end(P, "'if' condition")) return n;
    LunaNode* then_block = parse_block_after(P, "if statement");

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
            if (!expect_block_end(P, "'if' condition")) break;
            LunaNode* eblock = parse_block_after(P, "else if statement");
            LunaNode* einode = luna_node_new(NODE_IF, eline, ecol);
            einode->as.if_stmt.cond = econd;
            einode->as.if_stmt.then_block = eblock;
            LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
            item->node = einode;
            if (!elif_head) elif_head = item; else elif_tail->next = item;
            elif_tail = item;
        } else {
            if (!expect_block_end(P, "'else'")) break;
            else_block = parse_block_after(P, "else statement");
            break;
        }
    }

    if (n) {
        n->as.if_stmt.then_block = then_block;
        n->as.if_stmt.elifs = elif_head;
        n->as.if_stmt.else_block = else_block;
    }
    return n;
}

static LunaNode* parse_for(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    LunaNode* n = luna_node_new(NODE_FOR, line, col);
    LunaNodeList* vars = NULL;
    LunaNodeList* tail = NULL;
    int count = 0;
    for (;;) {
        if (!check(P, LUNA_TOK_IDENT)) {
            syntax_error(P, token_span(P, &P->current),
                      "names must start with a letter or '_'",
                      "expected identifier");
            return n;
        }
        char* name = copy_token_text(&P->current);
        LunaNode* id = luna_node_ident(name, P->current.line, P->current.column);
        advance(P);
        LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
        item->node = id;
        if (!vars) vars = item; else tail->next = item;
        tail = item;
        count++;
        if (!match(P, LUNA_TOK_COMMA)) break;
    }
    if (count > 2) {
        syntax_error(P, token_span(P, &P->previous),
                     "use one variable for elements or two for key and value",
                     "too many loop variables");
    }
    if (n) n->as.for_stmt.vars = vars;
    if (!expect(P, LUNA_TOK_IN, "'in' after loop variable")) return n;
    LunaNode* iter = parse_expr(P, 0);
    if (n) n->as.for_stmt.iter = iter;
    if (!expect_block_end(P, "'for' clause")) return n;
    P->loop_depth++;
    LunaNode* body = parse_block_after(P, "for statement");
    P->loop_depth--;
    if (n) n->as.for_stmt.body = body;
    return n;
}

static LunaNode* parse_while(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    LunaNode* n = luna_node_new(NODE_WHILE, line, col);
    LunaNode* cond = parse_expr(P, 0);
    if (n) n->as.while_stmt.cond = cond;
    if (!expect_block_end(P, "'while' condition")) return n;
    P->loop_depth++;
    LunaNode* body = parse_block_after(P, "while statement");
    P->loop_depth--;
    if (n) n->as.while_stmt.body = body;
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

static LunaNode* parse_extern(LunaParser* P) {
    int line = P->current.line;
    int col = P->current.column;
    advance(P);

    if (!check(P, LUNA_TOK_IDENT)) {
        syntax_error(P, token_span(P, &P->current),
                     "names must start with a letter or '_'",
                     "expected identifier after 'extern'");
        return NULL;
    }
    char* name = copy_token_text(&P->current);
    advance(P);

    if (!expect(P, LUNA_TOK_LPAREN, "'('")) {
        free(name);
        return NULL;
    }

    LunaNode* library = parse_expr(P, 0);
    if (!expect(P, LUNA_TOK_COMMA, "','")) {
        if (library) luna_ast_free(library);
        free(name);
        return NULL;
    }

    LunaNode* signature = parse_expr(P, 0);
    if (!expect(P, LUNA_TOK_RPAREN, "')'")) {
        if (library) luna_ast_free(library);
        if (signature) luna_ast_free(signature);
        free(name);
        return NULL;
    }

    if (!library || library->type != NODE_STRING ||
        !signature || signature->type != NODE_STRING) {
        syntax_error(P, token_span(P, &P->current),
                     "use string literals for library and signature",
                     "'extern' expects two string literals");
        if (library) luna_ast_free(library);
        if (signature) luna_ast_free(signature);
        free(name);
        return NULL;
    }

    char* callee_name = (char*)malloc(sizeof("__ffi_declare"));
    if (!callee_name) {
        if (library) luna_ast_free(library);
        if (signature) luna_ast_free(signature);
        free(name);
        return NULL;
    }
    memcpy(callee_name, "__ffi_declare", sizeof("__ffi_declare"));

    char* symbol_name = (char*)malloc(strlen(name) + 1);
    if (!symbol_name) {
        if (library) luna_ast_free(library);
        if (signature) luna_ast_free(signature);
        free(callee_name);
        free(name);
        return NULL;
    }
    memcpy(symbol_name, name, strlen(name) + 1);

    LunaNode* callee = luna_node_ident(callee_name, line, col);
    LunaNodeList* args = luna_node_list_one(luna_node_string(symbol_name, strlen(symbol_name), line, col));
    LunaNodeList* tail = luna_node_list_append(args, library);
    luna_node_list_append(tail, signature);
    LunaNode* init = luna_node_call(callee, args, line, col);

    LunaNode* n = luna_node_new(NODE_LET, line, col);
    n->as.var.name = name;
    n->as.var.init = init;
    n->as.var.is_pub = 0;
    return n;
}

static LunaNode* parse_import(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    LunaNodeList* head = NULL;
    LunaNodeList* tail = NULL;
    for (;;) {
        if (!check(P, LUNA_TOK_IDENT)) {
            syntax_error(P, token_span(P, &P->current),
                     "names must start with a letter or '_'",
                     "expected identifier");
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
        syntax_error(P, token_span(P, &P->current),
                     "names must start with a letter or '_'",
                     "expected identifier");
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
    LunaNode* n = luna_node_new(NODE_FN, line, col);
    if (n) n->as.fn.is_pub = is_pub;
    if (!check(P, LUNA_TOK_IDENT)) {
        syntax_error(P, token_span(P, &P->current),
            "names must start with a letter or '_'",
            "expected identifier");
        return n;
    }
    char* name = copy_token_text(&P->current);
    advance(P);
    if (!expect(P, LUNA_TOK_LPAREN, "'(' after function name")) { free(name); return n; }
    LunaNodeList* params = parse_param_list(P);
    if (n) n->as.fn.params = params;
    if (!expect_help(P, LUNA_TOK_RPAREN, "')' to close parameter list",
                         "insert ')' after parameter list")) { free(name); return n; }
    if (!expect_block_end(P, "function signature")) { free(name); return n; }
    int outer_loop_depth = P->loop_depth;
    P->loop_depth = 0;
    LunaNode* body = parse_block_after(P, "fn declaration");
    P->loop_depth = outer_loop_depth;
    if (n) {
        n->as.fn.name = name;
        n->as.fn.body = body;
    } else {
        free(name);
    }
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
            syntax_error(P, token_span(P, &P->current),
                     "names must start with a letter or '_'",
                     "expected identifier");
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
    if (check(P, LUNA_TOK_EXTERN)) return parse_extern(P);
    if (check(P, LUNA_TOK_IMPORT))  return parse_import(P);
    if (check(P, LUNA_TOK_BREAK)) {
        int line = P->current.line, col = P->current.column;
        if (P->loop_depth == 0) {
            syntax_error(P, token_span(P, &P->current),
                         "'break' can only be used inside 'while' or 'for'",
                         "'break' outside loop");
            advance(P);
            return NULL;
        }
        advance(P);
        return luna_node_new(NODE_BREAK, line, col);
    }
    if (check(P, LUNA_TOK_CONTINUE)) {
        int line = P->current.line, col = P->current.column;
        if (P->loop_depth == 0) {
            syntax_error(P, token_span(P, &P->current),
                         "'continue' can only be used inside 'while' or 'for'",
                         "'continue' outside loop");
            advance(P);
            return NULL;
        }
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
            fail_at(P, expr ? expr->line : line, expr ? expr->column : col,
                    "invalid assignment target");
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
        syntax_error(P, token_span(P, &P->current), NULL,
                     "'pub' must precede fn, let, const or mod");
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
        LunaNode* n = luna_node_new(NODE_TRY, line, col);
        if (!expect_block_end(P, "'try' block")) return n;
        LunaNode* body = parse_block_after(P, "try statement");
        if (n) n->as.try_stmt.body = body;
        if (!check(P, LUNA_TOK_CATCH)) {
            syntax_error(P, token_span(P, &P->current),
                         "add 'catch e:' at the same indentation level",
                         "'try' block requires a 'catch' clause");
            return n;
        }
        advance(P);
        if (!check(P, LUNA_TOK_IDENT)) {
            syntax_error(P, token_span(P, &P->current),
                     "names must start with a letter or '_'",
                     "expected identifier");
            return n;
        }
        char* cname = copy_token_text(&P->current);
        advance(P);
        if (!expect_block_end(P, "'catch' clause")) { free(cname); return n; }
        LunaNode* cbody = parse_block_after(P, "catch clause");
        if (n) {
            n->as.try_stmt.catch_name = cname;
            n->as.try_stmt.catch_body = cbody;
        } else {
            free(cname);
        }
        return n;
    }

    LunaNode* n = parse_simple_statement(P);
    if (n) require_statement_end(P);
    return n;
}