#include "ast.h"
#include "ast_internal.h"
#include <stdlib.h>
#include <string.h>

static char* dup_owned(const char* s, size_t n) {
    char* out = (char*)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n);
    out[n] = 0;
    return out;
}

LunaNode* luna_node_new(LunaNodeType type, int line, int col) {
    LunaNode* n = (LunaNode*)calloc(1, sizeof(LunaNode));
    if (!n) return NULL;
    n->type = type;
    n->line = line;
    n->column = col;
    return n;
}

LunaNode* luna_node_nil(int line, int col) { return luna_node_new(NODE_NIL, line, col); }

LunaNode* luna_node_bool(int v, int line, int col) {
    LunaNode* n = luna_node_new(NODE_BOOL, line, col);
    if (n) n->as.boolean = v ? 1 : 0;
    return n;
}

LunaNode* luna_node_int(int64_t v, int line, int col) {
    LunaNode* n = luna_node_new(NODE_INT, line, col);
    if (n) n->as.integer = v;
    return n;
}

LunaNode* luna_node_float(double v, int line, int col) {
    LunaNode* n = luna_node_new(NODE_FLOAT, line, col);
    if (n) n->as.floating = v;
    return n;
}

LunaNode* luna_node_string(char* owned_data, size_t len, int line, int col) {
    LunaNode* n = luna_node_new(NODE_STRING, line, col);
    if (n) { n->as.string.data = owned_data; n->as.string.len = len; }
    else free(owned_data);
    return n;
}

LunaNode* luna_node_ident(char* owned_name, int line, int col) {
    LunaNode* n = luna_node_new(NODE_IDENT, line, col);
    if (n) n->as.string.data = owned_name;
    else free(owned_name);
    return n;
}

LunaNode* luna_node_binary(LunaTokenType op, LunaNode* l, LunaNode* r, int line, int col) {
    LunaNode* n = luna_node_new(NODE_BINARY, line, col);
    if (n) { n->as.binary.op = op; n->as.binary.left = l; n->as.binary.right = r; }
    return n;
}

LunaNode* luna_node_unary(LunaTokenType op, LunaNode* operand, int line, int col) {
    LunaNode* n = luna_node_new(NODE_UNARY, line, col);
    if (n) { n->as.unary.op = op; n->as.unary.operand = operand; }
    return n;
}

LunaNode* luna_node_call(LunaNode* callee, LunaNodeList* args, int line, int col) {
    LunaNode* n = luna_node_new(NODE_CALL, line, col);
    if (n) { n->as.call.callee = callee; n->as.call.args = args; }
    return n;
}

LunaNode* luna_node_index(LunaNode* obj, LunaNode* idx, int line, int col) {
    LunaNode* n = luna_node_new(NODE_INDEX, line, col);
    if (n) { n->as.index.object = obj; n->as.index.index = idx; }
    return n;
}

LunaNode* luna_node_field(LunaNode* obj, char* owned_name, int line, int col) {
    LunaNode* n = luna_node_new(NODE_FIELD, line, col);
    if (n) { n->as.field.object = obj; n->as.field.name = owned_name; }
    else free(owned_name);
    return n;
}

LunaNode* luna_node_list(LunaNodeList* elems, int line, int col) {
    LunaNode* n = luna_node_new(NODE_LIST, line, col);
    if (n) n->as.list.elements = elems;
    return n;
}

LunaNode* luna_node_table(LunaTableEntry* entries, int line, int col) {
    LunaNode* n = luna_node_new(NODE_TABLE, line, col);
    if (n) n->as.table.entries = entries;
    return n;
}

LunaNode* luna_node_lambda(LunaNodeList* params, LunaNode* body, int line, int col) {
    LunaNode* n = luna_node_new(NODE_LAMBDA, line, col);
    if (n) { n->as.lambda.params = params; n->as.lambda.body = body; }
    return n;
}

LunaNode* luna_node_block(LunaNodeList* stmts, int line, int col) {
    LunaNode* n = luna_node_new(NODE_BLOCK, line, col);
    if (n) n->as.block.stmts = stmts;
    return n;
}

LunaNode* luna_node_program(LunaNodeList* stmts, int line, int col) {
    LunaNode* n = luna_node_new(NODE_PROGRAM, line, col);
    if (n) n->as.program.stmts = stmts;
    return n;
}

LunaNodeList* luna_node_list_append(LunaNodeList* tail, LunaNode* node) {
    LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
    if (!item) return tail;
    item->node = node;
    if (!tail) return item;
    tail->next = item;
    return item;
}

LunaNodeList* luna_node_list_one(LunaNode* node) {
    return luna_node_list_append(NULL, node);
}

char* luna_strdup_n(const char* s, size_t n) { return dup_owned(s, n); }