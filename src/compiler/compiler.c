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

/* Extern de closures.c */
extern int resolve_variable(LunaCompiler* C, const char* name);

#define LUNA_TEMP_SLOTS 16

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

static void compile_binary(LunaCompiler* C, LunaNode* node, int reg) {
    LunaProto* proto = compiler_current_proto(C);

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
}

static void compile_assign(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    int reg = expr_base(C);

    compile_expr(C, node->as.assign.value, reg);

    if (node->as.assign.target->type != NODE_IDENT) {
        compiler_error_at(C, node, "complex assignment targets not yet supported");
        return;
    }

    const char* name = node->as.assign.target->as.string.data;
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
        case NODE_THROW:      compile_throw(C, node); break;
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

    for (LunaNodeList* l = node->as.program.stmts; l; l = l->next) {
        compile_stmt(C, l->node);
        if (C->failed) return;
    }
}

/* ---------- API pública ---------- */
LunaProto* luna_compile(LunaNode* program) {
    LunaCompiler C;
    memset(&C, 0, sizeof(C));
    
    /* Crear proto raíz */
    compiler_push_proto(&C, "<main>");
    
    /* Compilar */
    compile_program(&C, program);
    
    if (C.failed) {
        /* Limpiar protos */
        while (C.proto_depth > 0) {
            LunaProto* p = compiler_pop_proto(&C);
            luna_proto_free(p);
        }
        return NULL;
    }
    
    /* Emitir RETURN implícito */
    LunaProto* root = compiler_current_proto(&C);
    luna_proto_emit_op_AB(root, OP_RETURN, 0, 0);
    
    return compiler_pop_proto(&C);
}

const char* luna_compiler_error(LunaCompiler* C) {
    return C->failed ? C->error_message : NULL;
}

struct LunaProto* luna_compile_source(struct LunaState* L,
                                      const char* source, size_t length,
                                      const char* filename,
                                      char* error_out, size_t error_size) {
    LunaParser P;
    LunaNode* prog = luna_parse(source, length, &P);
    if (!prog) {
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
        return NULL;
    }

    LunaProto* proto = luna_compile(prog);
    luna_ast_free(prog);

    if (!proto && error_out) {
        snprintf(error_out, error_size, "compilation failed in '%s'",
                 filename ? filename : "<string>");
    }
    return proto;
}