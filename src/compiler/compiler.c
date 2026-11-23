#include "compiler_internal.h"
#include "../parser/parser.h"
#include "../errors/errors_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declarations */
static void compile_program(LunaCompiler* C, LunaNode* node);
static void compile_block(LunaCompiler* C, LunaNode* node);
static void compile_fn_decl(LunaCompiler* C, LunaNode* node);
static void compile_let(LunaCompiler* C, LunaNode* node);
static void compile_const(LunaCompiler* C, LunaNode* node);
static void compile_if(LunaCompiler* C, LunaNode* node);
static void compile_while(LunaCompiler* C, LunaNode* node);
static void compile_for(LunaCompiler* C, LunaNode* node);
static void compile_return(LunaCompiler* C, LunaNode* node);
static void compile_expr_stmt(LunaCompiler* C, LunaNode* node);
static void compile_assign(LunaCompiler* C, LunaNode* node);
static void compile_import(LunaCompiler* C, LunaNode* node);
static void compile_field(LunaCompiler* C, LunaNode* node, int reg);

/* Extern de closures.c */
extern int resolve_variable(LunaCompiler* C, const char* name);
extern int variable_is_const(LunaCompiler* C, const char* name);

#define LUNA_TEMP_SLOTS 16
#define LUNA_EXPORTS_LOCAL "__luna_exports__"
#define LUNA_EXPORTS_GLOBAL "__exports__"

static uint8_t expr_base(LunaCompiler* C) {
    return (uint8_t)C->scope_stack[C->scope_depth - 1].num_locals;
}

/* ---------- Gestión de errores ---------- */
void compiler_error(LunaCompiler* C, int line, int col, const char* msg) {
    if (C->failed) return;
    C->failed = true;
    snprintf(C->error_message, sizeof(C->error_message), "%s", msg);
    C->error_line = line;
    C->error_column = col;
}

void compiler_error_at(LunaCompiler* C, LunaNode* node, const char* msg) {
    compiler_error(C, node->line, node->column, msg);
}

/* ---------- Gestión de protos ---------- */
void compiler_push_proto(LunaCompiler* C, const char* name) {
    if (C->proto_depth >= LUNA_MAX_PROTO_DEPTH) {
        compiler_error(C, 0, 0, "function nesting too deep");
        return;
    }
    
    LunaProto* proto = luna_proto_new(name);
    C->proto_stack[C->proto_depth] = proto;
    C->proto_depth++;
    
    /* Inicializar scope */
    scope_init(&C->scope_stack[C->scope_depth], C->scope_depth);
    C->scope_depth++;
}

LunaProto* compiler_pop_proto(LunaCompiler* C) {
    if (C->proto_depth == 0) return NULL;

    LunaScope* scope = &C->scope_stack[C->scope_depth - 1];
    LunaProto* proto = C->proto_stack[C->proto_depth - 1];

    proto->stack_size = (uint8_t)(scope->num_locals + LUNA_TEMP_SLOTS);
    for (int i = 0; i < scope->num_upvalues; i++) {
        luna_proto_add_upvalue(proto, scope->upvalues[i].is_local,
                               scope->upvalues[i].index);
    }

    C->proto_depth--;
    C->scope_depth--;
    scope_free(&C->scope_stack[C->scope_depth]);

    return C->proto_stack[C->proto_depth];
}

LunaProto* compiler_current_proto(LunaCompiler* C) {
    if (C->proto_depth == 0) return NULL;
    return C->proto_stack[C->proto_depth - 1];
}

/* ---------- Módulos y exportaciones ---------- */
static bool stmt_is_pub(LunaNode* node) {
    if (!node) return false;
    switch (node->type) {
        case NODE_LET:
        case NODE_CONST:
            return node->as.var.is_pub != 0;
        case NODE_FN:
            return node->as.fn.is_pub != 0;
        case NODE_MOD:
            return true;
        default:
            return false;
    }
}

static bool program_needs_exports(LunaNode* program) {
    if (!program || program->type != NODE_PROGRAM) return false;
    for (LunaNodeList* l = program->as.program.stmts; l; l = l->next) {
        if (stmt_is_pub(l->node)) return true;
    }
    return false;
}

static int ensure_exports_local(LunaCompiler* C) {
    LunaScope* scope = &C->scope_stack[C->scope_depth - 1];
    int slot = scope_find_local(scope, LUNA_EXPORTS_LOCAL);
    if (slot >= 0) return slot;

    slot = scope_add_local(scope, LUNA_EXPORTS_LOCAL, true);
    if (slot < 0) {
        compiler_error(C, 0, 0, "too many local variables for module exports");
        return -1;
    }
    luna_proto_emit_op_A(compiler_current_proto(C), OP_NEW_TABLE, (uint8_t)slot);
    C->has_exports = true;
    return slot;
}

static void emit_export(LunaCompiler* C, const char* name, int value_slot) {
    if (C->failed || !name) return;

    int exports_slot = ensure_exports_local(C);
    if (exports_slot < 0) return;

    uint16_t idx = luna_proto_add_const_string(compiler_current_proto(C), name, strlen(name));
    if (idx > 255) {
        compiler_error(C, 0, 0, "too many constants for module exports");
        return;
    }
    luna_proto_emit_op_ABC(compiler_current_proto(C), OP_SET_FIELD,
                           (uint8_t)exports_slot, (uint8_t)value_slot, (uint8_t)idx);
}

/* ---------- Compilación de expresiones ---------- */
void compile_expr(LunaCompiler* C, LunaNode* node, int reg);

static void compile_literal(LunaCompiler* C, LunaNode* node, int reg) {
    LunaProto* proto = compiler_current_proto(C);

    switch (node->type) {
        case NODE_NIL:
            luna_proto_emit_op_A(proto, OP_LOAD_NIL, (uint8_t)reg);
            break;
        case NODE_BOOL:
            luna_proto_emit_op_A(proto, node->as.boolean ? OP_LOAD_TRUE : OP_LOAD_FALSE,
                                 (uint8_t)reg);
            break;
        case NODE_INT: {
            int64_t val = node->as.integer;
            if (val >= -32767 && val <= 32767) {
                luna_proto_emit_op_AsBx(proto, OP_LOAD_INT, (uint8_t)reg, (int16_t)val);
            } else {
                uint16_t idx = luna_proto_add_const_int(proto, val);
                luna_proto_emit_op_ABx(proto, OP_LOAD_CONST, (uint8_t)reg, idx);
            }
            break;
        }
        case NODE_FLOAT: {
            uint16_t idx = luna_proto_add_const_float(proto, node->as.floating);
            luna_proto_emit_op_ABx(proto, OP_LOAD_CONST, (uint8_t)reg, idx);
            break;
        }
        case NODE_STRING: {
            uint16_t idx = luna_proto_add_const_string(proto, node->as.string.data,
                                                       node->as.string.len);
            luna_proto_emit_op_ABx(proto, OP_LOAD_CONST, (uint8_t)reg, idx);
            break;
        }
        default:
            compiler_error_at(C, node, "invalid literal node");
    }
}

static void compile_ident(LunaCompiler* C, LunaNode* node, int reg) {
    LunaProto* proto = compiler_current_proto(C);
    const char* name = node->as.string.data;

    int result = resolve_variable(C, name);

    if (result >= 0) {
        luna_proto_emit_op_AB(proto, OP_MOVE, (uint8_t)reg, (uint8_t)result);
    } else if (result <= -3) {
        luna_proto_emit_op_AB(proto, OP_GET_UPVALUE, (uint8_t)reg,
                              (uint8_t)(-3 - result));
    } else if (result == -2) {
        uint16_t idx = luna_proto_add_const_string(proto, name, strlen(name));
        luna_proto_emit_op_ABx(proto, OP_GET_GLOBAL, (uint8_t)reg, idx);
    } else {
        compiler_error_at(C, node, "variable resolution failed");
    }
}

typedef struct {
    int kind;
    int64_t integer;
    double floating;
    int boolean;
} LunaFoldValue;

static int fold_literal(LunaNode* node, LunaFoldValue* out) {
    switch (node->type) {
        case NODE_INT:
            out->kind = 0;
            out->integer = node->as.integer;
            return 1;
        case NODE_FLOAT:
            out->kind = 1;
            out->floating = node->as.floating;
            return 1;
        case NODE_BOOL:
            out->kind = 2;
            out->boolean = node->as.boolean;
            return 1;
        default:
            return 0;
    }
}

static int fold_binary_op(LunaTokenType op, LunaFoldValue a,
                          LunaFoldValue b, LunaFoldValue* out) {
    if (a.kind == 2 || b.kind == 2) return 0;

    int is_float = (a.kind == 1 || b.kind == 1);
    double af = a.kind == 1 ? a.floating : (double)a.integer;
    double bf = b.kind == 1 ? b.floating : (double)b.integer;

    switch (op) {
        case LUNA_TOK_PLUS:
        case LUNA_TOK_MINUS:
        case LUNA_TOK_STAR: {
            if (is_float) {
                double r = op == LUNA_TOK_PLUS ? af + bf :
                           op == LUNA_TOK_MINUS ? af - bf : af * bf;
                out->kind = 1;
                out->floating = r;
            } else {
                uint64_t ua = (uint64_t)a.integer;
                uint64_t ub = (uint64_t)b.integer;
                uint64_t r = op == LUNA_TOK_PLUS ? ua + ub :
                             op == LUNA_TOK_MINUS ? ua - ub : ua * ub;
                out->kind = 0;
                out->integer = (int64_t)r;
            }
            return 1;
        }
        case LUNA_TOK_SLASH: {
            if (bf == 0.0) return 0;
            out->kind = 1;
            out->floating = af / bf;
            return 1;
        }
        case LUNA_TOK_PERCENT: {
            if (is_float || b.integer == 0) return 0;
            out->kind = 0;
            out->integer = a.integer % b.integer;
            return 1;
        }
        case LUNA_TOK_EQ:
        case LUNA_TOK_NEQ:
        case LUNA_TOK_LT:
        case LUNA_TOK_LE:
        case LUNA_TOK_GT:
        case LUNA_TOK_GE: {
            int r;
            if (is_float) {
                switch (op) {
                    case LUNA_TOK_EQ:  r = af == bf; break;
                    case LUNA_TOK_NEQ: r = af != bf; break;
                    case LUNA_TOK_LT:  r = af <  bf; break;
                    case LUNA_TOK_LE:  r = af <= bf; break;
                    case LUNA_TOK_GT:  r = af >  bf; break;
                    default:           r = af >= bf; break;
                }
            } else {
                switch (op) {
                    case LUNA_TOK_EQ:  r = a.integer == b.integer; break;
                    case LUNA_TOK_NEQ: r = a.integer != b.integer; break;
                    case LUNA_TOK_LT:  r = a.integer <  b.integer; break;
                    case LUNA_TOK_LE:  r = a.integer <= b.integer; break;
                    case LUNA_TOK_GT:  r = a.integer >  b.integer; break;
                    default:           r = a.integer >= b.integer; break;
                }
            }
            out->kind = 2;
            out->boolean = r;
            return 1;
        }
        default:
            return 0;
    }
}

static int fold_expr(LunaNode* node, LunaFoldValue* out) {
    if (fold_literal(node, out)) return 1;
    if (node->type != NODE_BINARY) return 0;

    LunaFoldValue left, right;
    if (!fold_expr(node->as.binary.left, &left)) return 0;
    if (!fold_expr(node->as.binary.right, &right)) return 0;
    return fold_binary_op(node->as.binary.op, left, right, out);
}

static void emit_folded(LunaCompiler* C, LunaFoldValue value, int reg) {
    LunaProto* proto = compiler_current_proto(C);

    if (value.kind == 2) {
        luna_proto_emit_op_A(proto, value.boolean ? OP_LOAD_TRUE : OP_LOAD_FALSE,
                             (uint8_t)reg);
    } else if (value.kind == 0 && value.integer >= -32767 && value.integer <= 32767) {
        luna_proto_emit_op_AsBx(proto, OP_LOAD_INT, (uint8_t)reg,
                                (int16_t)value.integer);
    } else if (value.kind == 0) {
        uint16_t idx = luna_proto_add_const_int(proto, value.integer);
        luna_proto_emit_op_ABx(proto, OP_LOAD_CONST, (uint8_t)reg, idx);
    } else {
        uint16_t idx = luna_proto_add_const_float(proto, value.floating);
        luna_proto_emit_op_ABx(proto, OP_LOAD_CONST, (uint8_t)reg, idx);
    }
}

static void compile_binary(LunaCompiler* C, LunaNode* node, int reg) {
    LunaProto* proto = compiler_current_proto(C);
    LunaFoldValue folded;

    if (fold_expr(node, &folded)) {
        emit_folded(C, folded, reg);
        return;
    }

    compile_expr(C, node->as.binary.left, reg);
    compile_expr(C, node->as.binary.right, reg + 1);

    LunaOpcode op;
    switch (node->as.binary.op) {
        case LUNA_TOK_PLUS:      op = OP_ADD; break;
        case LUNA_TOK_MINUS:     op = OP_SUB; break;
        case LUNA_TOK_STAR:      op = OP_MUL; break;
        case LUNA_TOK_SLASH:     op = OP_DIV; break;
        case LUNA_TOK_PERCENT:   op = OP_MOD; break;
        case LUNA_TOK_STAR_STAR: op = OP_POW; break;
        case LUNA_TOK_EQ:        op = OP_EQ; break;
        case LUNA_TOK_NEQ:       op = OP_NEQ; break;
        case LUNA_TOK_LT:        op = OP_LT; break;
        case LUNA_TOK_LE:        op = OP_LE; break;
        case LUNA_TOK_GT:        op = OP_GT; break;
        case LUNA_TOK_GE:        op = OP_GE; break;
        case LUNA_TOK_AND:       op = OP_AND; break;
        case LUNA_TOK_OR:        op = OP_OR; break;
        default:
            compiler_error_at(C, node, "unknown binary operator");
            return;
    }

    luna_proto_emit_op_ABC(proto, op, (uint8_t)reg, (uint8_t)reg, (uint8_t)(reg + 1));
}

static void compile_unary(LunaCompiler* C, LunaNode* node, int reg) {
    LunaProto* proto = compiler_current_proto(C);

    compile_expr(C, node->as.unary.operand, reg);

    LunaOpcode op;
    switch (node->as.unary.op) {
        case LUNA_TOK_MINUS: op = OP_NEG; break;
        case LUNA_TOK_NOT:   op = OP_NOT; break;
        default:
            compiler_error_at(C, node, "unknown unary operator");
            return;
    }

    luna_proto_emit_op_AB(proto, op, (uint8_t)reg, (uint8_t)reg);
}

static void compile_call(LunaCompiler* C, LunaNode* node, int reg) {
    LunaProto* proto = compiler_current_proto(C);

    compile_expr(C, node->as.call.callee, reg);

    int nargs = 0;
    for (LunaNodeList* l = node->as.call.args; l; l = l->next) {
        compile_expr(C, l->node, reg + 1 + nargs);
        nargs++;
    }

    luna_proto_emit_op_ABC(proto, OP_CALL, (uint8_t)reg, (uint8_t)nargs, 1);
}

static void compile_field(LunaCompiler* C, LunaNode* node, int reg) {
    LunaProto* proto = compiler_current_proto(C);
    const char* name = node->as.field.name;

    compile_expr(C, node->as.field.object, reg);
    if (C->failed || !name) return;

    uint16_t idx = luna_proto_add_const_string(proto, name, strlen(name));
    if (idx > 255) {
        compiler_error_at(C, node, "too many constants for field access");
        return;
    }
    luna_proto_emit_op_ABC(proto, OP_GET_FIELD, (uint8_t)reg, (uint8_t)reg, (uint8_t)idx);
}

static void compile_index(LunaCompiler* C, LunaNode* node, int reg) {
    LunaProto* proto = compiler_current_proto(C);

    compile_expr(C, node->as.index.object, reg);
    compile_expr(C, node->as.index.index, reg + 1);
    luna_proto_emit_op_ABC(proto, OP_GET_INDEX, (uint8_t)reg, (uint8_t)reg,
                           (uint8_t)(reg + 1));
}

static void compile_list(LunaCompiler* C, LunaNode* node, int reg) {
    LunaProto* proto = compiler_current_proto(C);

    luna_proto_emit_op_A(proto, OP_NEW_LIST, (uint8_t)reg);
    for (LunaNodeList* l = node->as.list.elements; l; l = l->next) {
        compile_expr(C, l->node, reg + 1);
        luna_proto_emit_op_ABC(proto, OP_LIST_APPEND, (uint8_t)reg,
                               (uint8_t)(reg + 1), 0);
    }
}

static void compile_table(LunaCompiler* C, LunaNode* node, int reg) {
    LunaProto* proto = compiler_current_proto(C);

    luna_proto_emit_op_A(proto, OP_NEW_TABLE, (uint8_t)reg);
    for (LunaTableEntry* e = node->as.table.entries; e; e = e->next) {
        uint16_t key = luna_proto_add_const_string(proto, e->key, strlen(e->key));
        luna_proto_emit_op_ABx(proto, OP_LOAD_CONST, (uint8_t)(reg + 1), key);
        compile_expr(C, e->value, reg + 2);
        luna_proto_emit_op_ABC(proto, OP_TABLE_SET, (uint8_t)reg,
                               (uint8_t)(reg + 1), (uint8_t)(reg + 2));
    }
}

void compile_expr(LunaCompiler* C, LunaNode* node, int reg) {
    if (C->failed || !node) return;

    switch (node->type) {
        case NODE_NIL:
        case NODE_BOOL:
        case NODE_INT:
        case NODE_FLOAT:
        case NODE_STRING:
            compile_literal(C, node, reg);
            break;
        case NODE_IDENT:
            compile_ident(C, node, reg);
            break;
        case NODE_BINARY:
            compile_binary(C, node, reg);
            break;
        case NODE_UNARY:
            compile_unary(C, node, reg);
            break;
        case NODE_CALL:
            compile_call(C, node, reg);
            break;
        case NODE_FIELD:
            compile_field(C, node, reg);
            break;
        case NODE_INDEX:
            compile_index(C, node, reg);
            break;
        case NODE_LIST:
            compile_list(C, node, reg);
            break;
        case NODE_TABLE:
            compile_table(C, node, reg);
            break;
        default:
            compiler_error_at(C, node, "unsupported expression node");
    }
}

/* ---------- Compilación de sentencias ---------- */
static void patch_jump_AsBx(LunaProto* proto, size_t pos) {
    int16_t off = (int16_t)(proto->code_len - (pos + 4));
    uint16_t bx = (uint16_t)(off + 32767);
    proto->code[pos + 2] = (uint8_t)(bx & 0xFF);
    proto->code[pos + 3] = (uint8_t)((bx >> 8) & 0xFF);
}

static void patch_jump_sBx(LunaProto* proto, size_t pos) {
    int16_t off = (int16_t)(proto->code_len - (pos + 3));
    uint16_t bx = (uint16_t)(off + 32767);
    proto->code[pos + 1] = (uint8_t)(bx & 0xFF);
    proto->code[pos + 2] = (uint8_t)((bx >> 8) & 0xFF);
}

static void compile_let(LunaCompiler* C, LunaNode* node) {
    LunaScope* scope = &C->scope_stack[C->scope_depth - 1];
    int reg = expr_base(C);

    compile_expr(C, node->as.var.init, reg);

    int slot = scope_add_local(scope, node->as.var.name, false);
    if (slot < 0) {
        compiler_error_at(C, node, "too many local variables");
        return;
    }
    if (slot != reg) {
        luna_proto_emit_op_AB(compiler_current_proto(C), OP_MOVE,
                              (uint8_t)slot, (uint8_t)reg);
    }
    if (node->as.var.is_pub) emit_export(C, node->as.var.name, slot);
}

static void compile_const(LunaCompiler* C, LunaNode* node) {
    LunaScope* scope = &C->scope_stack[C->scope_depth - 1];
    int reg = expr_base(C);

    compile_expr(C, node->as.var.init, reg);

    int slot = scope_add_local(scope, node->as.var.name, true);
    if (slot < 0) {
        compiler_error_at(C, node, "too many local variables");
        return;
    }
    if (slot != reg) {
        luna_proto_emit_op_AB(compiler_current_proto(C), OP_MOVE,
                              (uint8_t)slot, (uint8_t)reg);
    }
    if (node->as.var.is_pub) emit_export(C, node->as.var.name, slot);
}

static void compile_import(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    LunaScope* scope = &C->scope_stack[C->scope_depth - 1];
    LunaNodeList* path = node->as.import_stmt.path;

    size_t len = 0;
    for (LunaNodeList* l = path; l; l = l->next) {
        if (!l->node || l->node->type != NODE_IDENT) {
            compiler_error_at(C, node, "invalid import path");
            return;
        }
        len += strlen(l->node->as.string.data);
        if (l->next) len++;
    }
    if (len == 0) {
        compiler_error_at(C, node, "empty import path");
        return;
    }

    char* name = (char*)malloc(len + 1);
    if (!name) {
        compiler_error_at(C, node, "out of memory compiling import");
        return;
    }

    char* p = name;
    const char* binding = NULL;
    for (LunaNodeList* l = path; l; l = l->next) {
        const char* part = l->node->as.string.data;
        size_t n = strlen(part);
        memcpy(p, part, n);
        p += n;
        binding = part;
        if (l->next) *p++ = '.';
    }
    *p = '\0';

    int reg = expr_base(C);
    uint16_t idx = luna_proto_add_const_string(proto, name, len);
    free(name);

    luna_proto_emit_op_ABx(proto, OP_IMPORT, (uint8_t)reg, idx);

    int slot = scope_add_local(scope, binding, true);
    if (slot < 0) {
        compiler_error_at(C, node, "too many local variables");
        return;
    }
    if (slot != reg) {
        luna_proto_emit_op_AB(proto, OP_MOVE, (uint8_t)slot, (uint8_t)reg);
    }
}

static int compound_opcode(LunaTokenType token, LunaOpcode* op) {
    switch (token) {
        case LUNA_TOK_PLUS_ASSIGN:  *op = OP_ADD; return 1;
        case LUNA_TOK_MINUS_ASSIGN: *op = OP_SUB; return 1;
        case LUNA_TOK_STAR_ASSIGN:  *op = OP_MUL; return 1;
        case LUNA_TOK_SLASH_ASSIGN: *op = OP_DIV; return 1;
        default: return 0;
    }
}

static void store_ident(LunaCompiler* C, LunaNode* node, const char* name, int reg) {
    LunaProto* proto = compiler_current_proto(C);

    if (variable_is_const(C, name)) {
        compiler_error_at(C, node, "cannot reassign 'const' variable");
        return;
    }

    int result = resolve_variable(C, name);

    if (result >= 0) {
        if (result != reg) {
            luna_proto_emit_op_AB(proto, OP_MOVE, (uint8_t)result, (uint8_t)reg);
        }
    } else if (result <= -3) {
        luna_proto_emit_op_AB(proto, OP_SET_UPVALUE, (uint8_t)reg,
                              (uint8_t)(-3 - result));
    } else if (result == -2) {
        uint16_t idx = luna_proto_add_const_string(proto, name, strlen(name));
        luna_proto_emit_op_ABx(proto, OP_SET_GLOBAL, (uint8_t)reg, idx);
    } else {
        compiler_error_at(C, node, "variable resolution failed");
    }
}

static void compile_assign(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    LunaNode* target = node->as.assign.target;
    LunaOpcode compound = OP_ADD;
    bool is_compound = node->as.assign.op != LUNA_TOK_ASSIGN;

    if (is_compound && !compound_opcode(node->as.assign.op, &compound)) {
        compiler_error_at(C, node, "unsupported compound assignment");
        return;
    }

    int reg = expr_base(C);

    if (target->type == NODE_IDENT) {
        int value_reg = reg;
        if (is_compound) {
            compile_ident(C, target, value_reg);
            compile_expr(C, node->as.assign.value, reg + 1);
            luna_proto_emit_op_ABC(proto, compound, (uint8_t)value_reg,
                                   (uint8_t)value_reg, (uint8_t)(reg + 1));
        } else {
            compile_expr(C, node->as.assign.value, value_reg);
        }
        store_ident(C, node, target->as.string.data, value_reg);
        return;
    }

    if (target->type == NODE_INDEX) {
        int object_reg = reg;
        int key_reg = reg + 1;
        int value_reg = reg + 2;
        int rhs_reg = reg + 3;

        compile_expr(C, target->as.index.object, object_reg);
        compile_expr(C, target->as.index.index, key_reg);
        if (is_compound) {
            luna_proto_emit_op_ABC(proto, OP_GET_INDEX, (uint8_t)value_reg,
                                   (uint8_t)object_reg, (uint8_t)key_reg);
            compile_expr(C, node->as.assign.value, rhs_reg);
            luna_proto_emit_op_ABC(proto, compound, (uint8_t)value_reg,
                                   (uint8_t)value_reg, (uint8_t)rhs_reg);
        } else {
            compile_expr(C, node->as.assign.value, value_reg);
        }
        luna_proto_emit_op_ABC(proto, OP_SET_INDEX, (uint8_t)object_reg,
                               (uint8_t)key_reg, (uint8_t)value_reg);
        return;
    }

    if (target->type == NODE_FIELD) {
        int object_reg = reg;
        int value_reg = reg + 1;
        int rhs_reg = reg + 2;
        uint16_t key = luna_proto_add_const_string(proto, target->as.field.name,
                                                   strlen(target->as.field.name));
        if (key > 255) {
            compiler_error_at(C, node, "too many constants for field assignment");
            return;
        }

        compile_expr(C, target->as.field.object, object_reg);
        if (is_compound) {
            luna_proto_emit_op_ABC(proto, OP_GET_FIELD, (uint8_t)value_reg,
                                   (uint8_t)object_reg, (uint8_t)key);
            compile_expr(C, node->as.assign.value, rhs_reg);
            luna_proto_emit_op_ABC(proto, compound, (uint8_t)value_reg,
                                   (uint8_t)value_reg, (uint8_t)rhs_reg);
        } else {
            compile_expr(C, node->as.assign.value, value_reg);
        }
        luna_proto_emit_op_ABC(proto, OP_SET_FIELD, (uint8_t)object_reg,
                               (uint8_t)value_reg, (uint8_t)key);
        return;
    }

    compiler_error_at(C, target, "invalid assignment target");
}

static void compile_return(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);

    if (node->as.return_value) {
        int reg = expr_base(C);
        compile_expr(C, node->as.return_value, reg);
        luna_proto_emit_op_AB(proto, OP_RETURN, (uint8_t)reg, 2);
    } else {
        luna_proto_emit_op_AB(proto, OP_RETURN, 0, 0);
    }
}

static void compile_expr_stmt(LunaCompiler* C, LunaNode* node) {
    compile_expr(C, node->as.expr, expr_base(C));
}

static void compile_try(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    LunaScope* scope = &C->scope_stack[C->scope_depth - 1];
    const LunaNode* body = node->as.try_stmt.body;
    const LunaNode* catch_body = node->as.try_stmt.catch_body;
    const char* catch_name = node->as.try_stmt.catch_name;

    if (!body || body->type != NODE_BLOCK || !catch_body ||
        catch_body->type != NODE_BLOCK || !catch_name) {
        compiler_error_at(C, node, "malformed try/catch statement");
        return;
    }

    size_t setup_pos = proto->code_len;
    luna_proto_emit_op_AsBx(proto, OP_SETUP_EXC, 0, 0);

    compile_block(C, (LunaNode*)body);
    if (C->failed) return;

    luna_proto_emit_byte(proto, OP_POP_EXC);

    size_t skip_pos = proto->code_len;
    luna_proto_emit_op_sBx(proto, OP_JUMP, 0);
    patch_jump_AsBx(proto, setup_pos);

    int reg = expr_base(C);
    int slot = scope_add_local(scope, catch_name, false);
    if (slot < 0) {
        compiler_error_at(C, node, "too many local variables");
        return;
    }

    luna_proto_emit_op_A(proto, OP_GET_EXC, (uint8_t)reg);
    if (slot != reg) {
        luna_proto_emit_op_AB(proto, OP_MOVE, (uint8_t)slot, (uint8_t)reg);
    }

    compile_block(C, (LunaNode*)catch_body);
    if (C->failed) return;

    patch_jump_sBx(proto, skip_pos);
    scope_hide_local(scope, slot);
}

static void compile_throw(LunaCompiler* C, LunaNode* node) {
    if (node->as.throw_value) {
        compile_expr(C, node->as.throw_value, expr_base(C));
    } else {
        luna_proto_emit_op_A(compiler_current_proto(C), OP_LOAD_NIL, expr_base(C));
    }
    luna_proto_emit_op_A(compiler_current_proto(C), OP_THROW, expr_base(C));
}

static void compile_if(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);

    compile_expr(C, node->as.if_stmt.cond, expr_base(C));

    size_t jump_pos = proto->code_len;
    luna_proto_emit_op_AsBx(proto, OP_JUMP_IF_FALSE, expr_base(C), 0);

    compile_block(C, node->as.if_stmt.then_block);

    size_t end_jump_pos = proto->code_len;
    luna_proto_emit_op_sBx(proto, OP_JUMP, 0);

    patch_jump_AsBx(proto, jump_pos);

    if (node->as.if_stmt.else_block) {
        compile_block(C, node->as.if_stmt.else_block);
    }

    patch_jump_sBx(proto, end_jump_pos);
}

static void compile_while(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);

    size_t loop_start = proto->code_len;

    compile_expr(C, node->as.while_stmt.cond, expr_base(C));

    size_t exit_jump_pos = proto->code_len;
    luna_proto_emit_op_AsBx(proto, OP_JUMP_IF_FALSE, expr_base(C), 0);

    compile_block(C, node->as.while_stmt.body);

    int16_t back = (int16_t)(loop_start - (int16_t)(proto->code_len + 3));
    luna_proto_emit_op_sBx(proto, OP_JUMP, back);

    patch_jump_AsBx(proto, exit_jump_pos);
}

static void compile_for(LunaCompiler* C, LunaNode* node) {
    (void)C;
    compiler_error_at(C, node, "for loops not yet implemented");
}

static void compile_fn_decl(LunaCompiler* C, LunaNode* node) {
    LunaProto* parent_proto = compiler_current_proto(C);
    LunaScope* parent_scope = &C->scope_stack[C->scope_depth - 1];

    int slot = scope_add_local(parent_scope, node->as.fn.name, false);
    if (slot < 0) {
        compiler_error_at(C, node, "too many local variables");
        return;
    }

    compiler_push_proto(C, node->as.fn.name);
    LunaProto* fn_proto = compiler_current_proto(C);
    LunaScope* scope = &C->scope_stack[C->scope_depth - 1];

    int param_idx = 0;
    for (LunaNodeList* l = node->as.fn.params; l; l = l->next) {
        scope_add_local(scope, l->node->as.string.data, false);
        param_idx++;
    }
    fn_proto->num_params = (uint8_t)param_idx;

    compile_block(C, node->as.fn.body);
    luna_proto_emit_op_AB(fn_proto, OP_RETURN, 0, 0);

    LunaProto* completed_fn = compiler_pop_proto(C);
    uint16_t proto_idx = luna_proto_add_proto(parent_proto, completed_fn);

    luna_proto_emit_op_ABx(parent_proto, OP_CLOSURE, (uint8_t)slot, proto_idx);
    if (node->as.fn.is_pub) emit_export(C, node->as.fn.name, slot);
}

static void compile_block(LunaCompiler* C, LunaNode* node) {
    if (!node || node->type != NODE_BLOCK) return;

    for (LunaNodeList* l = node->as.block.stmts; l; l = l->next) {
        compile_stmt(C, l->node);
        if (C->failed) return;
    }
}

void compile_stmt(LunaCompiler* C, LunaNode* node) {
    if (C->failed || !node) return;

    switch (node->type) {
        case NODE_LET:        compile_let(C, node); break;
        case NODE_CONST:      compile_const(C, node); break;
        case NODE_FN:         compile_fn_decl(C, node); break;
        case NODE_RETURN:     compile_return(C, node); break;
        case NODE_IF:         compile_if(C, node); break;
        case NODE_WHILE:      compile_while(C, node); break;
        case NODE_FOR:        compile_for(C, node); break;
        case NODE_ASSIGN:     compile_assign(C, node); break;
        case NODE_IMPORT:     compile_import(C, node); break;
        case NODE_MOD:        break;
        case NODE_THROW:      compile_throw(C, node); break;
        case NODE_TRY:        compile_try(C, node); break;
        case NODE_EXPR_STMT:  compile_expr_stmt(C, node); break;
        default:
            compiler_error_at(C, node, "unsupported statement node");
    }
}

static void compile_program(LunaCompiler* C, LunaNode* node) {
    if (!node || node->type != NODE_PROGRAM) {
        compiler_error(C, 0, 0, "expected PROGRAM node");
        return;
    }

    if (program_needs_exports(node)) ensure_exports_local(C);
    if (C->failed) return;

    for (LunaNodeList* l = node->as.program.stmts; l; l = l->next) {
        compile_stmt(C, l->node);
        if (C->failed) return;
    }
}

/* ---------- API pública ---------- */
static LunaProto* compile_program_detailed(LunaNode* program, char* message,
                                           size_t message_size, int* line_out,
                                           int* col_out) {
    LunaCompiler C;
    memset(&C, 0, sizeof(C));

    /* Crear proto raíz */
    compiler_push_proto(&C, "<main>");

    /* Compilar */
    compile_program(&C, program);

    if (C.failed) {
        if (message) {
            snprintf(message, message_size, "%s", C.error_message);
        }
        if (line_out) *line_out = C.error_line;
        if (col_out) *col_out = C.error_column;
        while (C.proto_depth > 0) {
            LunaProto* p = compiler_pop_proto(&C);
            luna_proto_free(p);
        }
        return NULL;
    }

    LunaProto* root = compiler_current_proto(&C);
    if (C.has_exports) {
        int slot = scope_find_local(&C.scope_stack[0], LUNA_EXPORTS_LOCAL);
        uint16_t idx = luna_proto_add_const_string(root, LUNA_EXPORTS_GLOBAL,
                                                   strlen(LUNA_EXPORTS_GLOBAL));
        luna_proto_emit_op_ABx(root, OP_SET_GLOBAL, (uint8_t)slot, idx);
    }

    /* Emitir RETURN implícito */
    luna_proto_emit_op_AB(root, OP_RETURN, 0, 0);

    return compiler_pop_proto(&C);
}

LunaProto* luna_compile(LunaNode* program) {
    return compile_program_detailed(program, NULL, 0, NULL, NULL);
}

const char* luna_compiler_error(LunaCompiler* C) {
    return C->failed ? C->error_message : NULL;
}

struct LunaProto* luna_compile_source(struct LunaState* L,
                                      const char* source, size_t length,
                                      const char* filename,
                                       char* error_out, size_t error_size) {
    if (L) luna_clear_error(L);

    LunaParser P;
    LunaNode* prog = luna_parse_named(source, length, filename, &P);
    if (!prog) {
        luna_parser_render_diagnostics(&P, stderr, luna_diag_color_enabled());
        const char* msg = luna_parser_error(&P);
        if (!msg) msg = "parse error";
        if (error_out) {
            snprintf(error_out, error_size, "%s:%d:%d: %s",
                     filename ? filename : "<string>",
                     P.error_line, P.error_column, msg);
        }
        if (L) {
            luna_error_set(L, LUNA_ERR_SYNTAX, filename,
                           P.error_line, P.error_column, "%s", msg);
        }
        luna_parser_destroy(&P);
        return NULL;
    }
    luna_parser_destroy(&P);

    char compile_message[256];
    int compile_line = 0;
    int compile_column = 0;
    LunaProto* proto = compile_program_detailed(prog, compile_message,
                                                 sizeof(compile_message),
                                                 &compile_line, &compile_column);
    luna_ast_free(prog);

    if (!proto) {
        if (error_out) {
            snprintf(error_out, error_size, "%s:%d:%d: %s",
                     filename ? filename : "<string>",
                     compile_line, compile_column,
                     compile_message[0] ? compile_message : "compilation failed");
        }
        if (L) {
            luna_error_set(L, LUNA_ERR_RUNTIME, filename,
                           compile_line, compile_column, "%s",
                           compile_message[0] ? compile_message : "compilation failed");
        }
    }
    return proto;
}