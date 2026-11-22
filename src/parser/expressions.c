#include "parser_internal.h"
#include "precedence.h"
#include <stdlib.h>
#include <string.h>

static size_t unescape_string(char* s, size_t len) {
    size_t r = 0, w = 0;
    while (r < len) {
        if (s[r] == '\\' && r + 1 < len) {
            char c = s[r + 1];
            switch (c) {
                case 'n': s[w++] = '\n'; break;
                case 't': s[w++] = '\t'; break;
                case 'r': s[w++] = '\r'; break;
                case '0': s[w++] = '\0'; break;
                case '\\': s[w++] = '\\'; break;
                case '"': s[w++] = '"'; break;
                default: s[w++] = '\\'; s[w++] = c; break;
            }
            r += 2;
        } else {
            s[w++] = s[r++];
        }
    }
    return w;
}

LunaNodeList* parse_arg_list(LunaParser* P) {
    LunaNodeList* head = NULL;
    LunaNodeList* tail = NULL;
    if (check(P, LUNA_TOK_RPAREN)) return NULL;
    for (;;) {
        LunaNode* e = parse_expr(P, 0);
        if (P->failed) { return head; }
        LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
        item->node = e;
        if (!head) head = item; else tail->next = item;
        tail = item;
        if (!match(P, LUNA_TOK_COMMA)) break;
    }
    return head;
}

LunaNodeList* parse_param_list(LunaParser* P) {
    LunaNodeList* head = NULL;
    LunaNodeList* tail = NULL;
    if (check(P, LUNA_TOK_RPAREN)) return NULL;
    for (;;) {
        if (!check(P, LUNA_TOK_IDENT)) {
            fail_current(P, "se esperaba un nombre de parámetro");
            return head;
        }
        char* name = copy_token_text(&P->current);
        LunaNode* n = luna_node_ident(name, P->current.line, P->current.column);
        advance(P);
        LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
        item->node = n;
        if (!head) head = item; else tail->next = item;
        tail = item;
        if (!match(P, LUNA_TOK_COMMA)) break;
    }
    return head;
}

static LunaNode* parse_postfix(LunaParser* P, LunaNode* node) {
    for (;;) {
        if (check(P, LUNA_TOK_LPAREN)) {
            int line = P->current.line, col = P->current.column;
            advance(P);
            LunaNodeList* args = parse_arg_list(P);
            if (!expect(P, LUNA_TOK_RPAREN, "')'")) return node;
            node = luna_node_call(node, args, line, col);
        } else if (check(P, LUNA_TOK_LBRACKET)) {
            int line = P->current.line, col = P->current.column;
            advance(P);
            LunaNode* idx = parse_expr(P, 0);
            if (!expect(P, LUNA_TOK_RBRACKET, "']'")) return node;
            node = luna_node_index(node, idx, line, col);
        } else if (check(P, LUNA_TOK_DOT)) {
            int line = P->current.line, col = P->current.column;
            advance(P);
            if (!check(P, LUNA_TOK_IDENT)) {
                fail_current(P, "se esperaba un nombre de campo después de '.'");
                return node;
            }
            char* name = copy_token_text(&P->current);
            advance(P);
            node = luna_node_field(node, name, line, col);
        } else {
            break;
        }
    }
    return node;
}

static LunaNode* parse_list_literal(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    LunaNodeList* head = NULL;
    LunaNodeList* tail = NULL;
    if (!check(P, LUNA_TOK_RBRACKET)) {
        for (;;) {
            LunaNode* e = parse_expr(P, 0);
            if (P->failed) return luna_node_list(head, line, col);
            LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
            item->node = e;
            if (!head) head = item; else tail->next = item;
            tail = item;
            if (!match(P, LUNA_TOK_COMMA)) break;
            if (check(P, LUNA_TOK_RBRACKET)) break;
        }
    }
    if (!expect(P, LUNA_TOK_RBRACKET, "']'")) {}
    return luna_node_list(head, line, col);
}

static LunaNode* parse_table_literal(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    LunaTableEntry* head = NULL;
    LunaTableEntry* tail = NULL;
    if (!check(P, LUNA_TOK_RBRACE)) {
        for (;;) {
            if (!check(P, LUNA_TOK_IDENT)) {
                fail_current(P, "se esperaba una clave en la tabla");
                return luna_node_table(head, line, col);
            }
            char* key = copy_token_text(&P->current);
            advance(P);
            if (!expect(P, LUNA_TOK_ASSIGN, "'=' after table key")) {
                free(key);
                return luna_node_table(head, line, col);
            }
            LunaNode* value = parse_expr(P, 0);
            if (P->failed) { free(key); return luna_node_table(head, line, col); }
            LunaTableEntry* e = (LunaTableEntry*)calloc(1, sizeof(LunaTableEntry));
            e->key = key; e->value = value;
            if (!head) head = e; else tail->next = e;
            tail = e;
            if (!match(P, LUNA_TOK_COMMA)) break;
            if (check(P, LUNA_TOK_RBRACE)) break;
        }
    }
    if (!expect(P, LUNA_TOK_RBRACE, "'}'")) {}
    return luna_node_table(head, line, col);
}

static LunaNode* parse_lambda(LunaParser* P) {
    int line = P->current.line, col = P->current.column;
    advance(P);
    if (!expect(P, LUNA_TOK_LPAREN, "'(' after 'fn'")) return NULL;
    LunaNodeList* params = parse_param_list(P);
    if (!expect(P, LUNA_TOK_RPAREN, "')'")) return NULL;
    if (!expect_block_end(P, "salto de línea antes del cuerpo de la lambda")) return NULL;
    LunaNode* body = parse_block(P);
    return luna_node_lambda(params, body, line, col);
}

static LunaNode* parse_primary(LunaParser* P) {
    LunaToken tok = P->current;
    switch (tok.type) {
        case LUNA_TOK_NIL:    advance(P); return luna_node_nil(tok.line, tok.column);
        case LUNA_TOK_TRUE:   advance(P); return luna_node_bool(1, tok.line, tok.column);
        case LUNA_TOK_FALSE:  advance(P); return luna_node_bool(0, tok.line, tok.column);
        case LUNA_TOK_INTEGER:advance(P); return luna_node_int(tok.int_value, tok.line, tok.column);
        case LUNA_TOK_FLOAT:  advance(P); return luna_node_float(tok.float_value, tok.line, tok.column);
        case LUNA_TOK_STRING: {
            advance(P);
            char* s = copy_token_text(&tok);
            size_t n = unescape_string(s, tok.length);
            return luna_node_string(s, n, tok.line, tok.column);
        }
        case LUNA_TOK_IDENT: {
            advance(P);
            char* name = copy_token_text(&tok);
            return luna_node_ident(name, tok.line, tok.column);
        }
        case LUNA_TOK_LPAREN: {
            advance(P);
            LunaNode* e = parse_expr(P, 0);
            if (!expect(P, LUNA_TOK_RPAREN, "')'")) return e;
            return e;
        }
        case LUNA_TOK_LBRACKET: return parse_list_literal(P);
        case LUNA_TOK_LBRACE:   return parse_table_literal(P);
        case LUNA_TOK_FN:       return parse_lambda(P);
        default:
            fail_current(P, "se esperaba una expresión");
            return luna_node_nil(tok.line, tok.column);
    }
}

static LunaNode* parse_prefix(LunaParser* P) {
    LunaNode* node;
    if (check(P, LUNA_TOK_MINUS)) {
        LunaToken tok = P->current; advance(P);
        LunaNode* operand = parse_expr(P, 7);
        node = luna_node_unary(LUNA_TOK_MINUS, operand, tok.line, tok.column);
    } else if (check(P, LUNA_TOK_NOT)) {
        LunaToken tok = P->current; advance(P);
        LunaNode* operand = parse_expr(P, 3);
        node = luna_node_unary(LUNA_TOK_NOT, operand, tok.line, tok.column);
    } else {
        node = parse_primary(P);
    }
    if (P->failed) return node;
    return parse_postfix(P, node);
}

LunaNode* parse_expr(LunaParser* P, int min_bp) {
    LunaNode* left = parse_prefix(P);
    if (P->failed) return left;

    for (;;) {
        int lbp = infix_bp(P->current.type);
        if (lbp < min_bp) break;
        LunaTokenType op = P->current.type;
        int line = P->current.line, col = P->current.column;
        advance(P);
        int rbp = is_right_assoc(op) ? lbp : lbp + 1;
        LunaNode* right = parse_expr(P, rbp);
        left = luna_node_binary(op, left, right, line, col);
        if (P->failed) return left;
    }
    return left;
}