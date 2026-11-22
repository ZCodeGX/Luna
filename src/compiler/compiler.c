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
static void compile_literal(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    
    switch (node->type) {
        case NODE_NIL:
            luna_proto_emit_op_A(proto, OP_LOAD_NIL, 0);
            break;
        case NODE_BOOL:
            luna_proto_emit_op_A(proto, node->as.boolean ? OP_LOAD_TRUE : OP_LOAD_FALSE, 0);
            break;
        case NODE_INT: {
            int64_t val = node->as.integer;
            if (val >= -32767 && val <= 32767) {
                /* Usar LOAD_INT para valores pequeños */
                luna_proto_emit_op_AsBx(proto, OP_LOAD_INT, 0, (int16_t)(val + 32767));
            } else {
                /* Usar constante */
                uint16_t idx = luna_proto_add_const_int(proto, val);
                luna_proto_emit_op_ABx(proto, OP_LOAD_CONST, 0, idx);
            }
            break;
        }
        case NODE_FLOAT: {
            uint16_t idx = luna_proto_add_const_float(proto, node->as.floating);
            luna_proto_emit_op_ABx(proto, OP_LOAD_CONST, 0, idx);
            break;
        }
        case NODE_STRING: {
            uint16_t idx = luna_proto_add_const_string(proto, node->as.string.data, node->as.string.len);
            luna_proto_emit_op_ABx(proto, OP_LOAD_CONST, 0, idx);
            break;
        }
        default:
            compiler_error_at(C, node, "invalid literal node");
    }
}

static void compile_ident(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    const char* name = node->as.string.data;
    
    int result = resolve_variable(C, name);
    
    if (result >= 0) {
        /* Es upvalue */
        luna_proto_emit_op_AB(proto, OP_GET_UPVALUE, 0, (uint8_t)result);
    } else if (result == -2) {
        /* Es global */
        uint16_t idx = luna_proto_add_const_string(proto, name, strlen(name));
        luna_proto_emit_op_ABx(proto, OP_GET_GLOBAL, 0, idx);
    } else {
        compiler_error_at(C, node, "variable resolution failed");
    }
}

static void compile_binary(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    
    /* Compilar operandos (dejan resultado en slot 0 y 1) */
    compile_expr(C, node->as.binary.left);
    compile_expr(C, node->as.binary.right);
    
    /* Emitir operación */
    LunaOpcode op;
    switch (node->as.binary.op) {
        case LUNA_TOK_PLUS:     op = OP_ADD; break;
        case LUNA_TOK_MINUS:    op = OP_SUB; break;
        case LUNA_TOK_STAR:     op = OP_MUL; break;
        case LUNA_TOK_SLASH:    op = OP_DIV; break;
        case LUNA_TOK_PERCENT:  op = OP_MOD; break;
        case LUNA_TOK_STAR_STAR: op = OP_POW; break;
        case LUNA_TOK_EQ:       op = OP_EQ; break;
        case LUNA_TOK_NEQ:      op = OP_NEQ; break;
        case LUNA_TOK_LT:       op = OP_LT; break;
        case LUNA_TOK_LE:       op = OP_LE; break;
        case LUNA_TOK_GT:       op = OP_GT; break;
        case LUNA_TOK_GE:       op = OP_GE; break;
        case LUNA_TOK_AND:      op = OP_AND; break;
        case LUNA_TOK_OR:       op = OP_OR; break;
        default:
            compiler_error_at(C, node, "unknown binary operator");
            return;
    }
    
    luna_proto_emit_op_ABC(proto, op, 0, 0, 1);
}

static void compile_unary(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    
    compile_expr(C, node->as.unary.operand);
    
    LunaOpcode op;
    switch (node->as.unary.op) {
        case LUNA_TOK_MINUS: op = OP_NEG; break;
        case LUNA_TOK_NOT:   op = OP_NOT; break;
        default:
            compiler_error_at(C, node, "unknown unary operator");
            return;
    }
    
    luna_proto_emit_op_AB(proto, op, 0, 0);
}

static void compile_call(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    
    /* Compilar callee */
    compile_expr(C, node->as.call.callee);
    
    /* Compilar argumentos */
    int nargs = 0;
    for (LunaNodeList* l = node->as.call.args; l; l = l->next) {
        compile_expr(C, l->node);
        nargs++;
    }
    
    /* Emitir CALL: slot 0 = callee, slots 1..nargs = args, resultado en slot 0 */
    luna_proto_emit_op_ABC(proto, OP_CALL, 0, (uint8_t)nargs, 1);
}

void compile_expr(LunaCompiler* C, LunaNode* node) {
    if (C->failed || !node) return;
    
    switch (node->type) {
        case NODE_NIL:
        case NODE_BOOL:
        case NODE_INT:
        case NODE_FLOAT:
        case NODE_STRING:
            compile_literal(C, node);
            break;
        case NODE_IDENT:
            compile_ident(C, node);
            break;
        case NODE_BINARY:
            compile_binary(C, node);
            break;
        case NODE_UNARY:
            compile_unary(C, node);
            break;
        case NODE_CALL:
            compile_call(C, node);
            break;
        default:
            compiler_error_at(C, node, "unsupported expression node");
    }
}

/* ---------- Compilación de sentencias ---------- */
static void compile_let(LunaCompiler* C, LunaNode* node) {
    LunaScope* scope = &C->scope_stack[C->scope_depth - 1];
    
    /* Compilar inicializador */
    compile_expr(C, node->as.var.init);
    
    /* Añadir local */
    int slot = scope_add_local(scope, node->as.var.name, false);
    if (slot < 0) {
        compiler_error_at(C, node, "too many local variables");
    }
}

static void compile_const(LunaCompiler* C, LunaNode* node) {
    LunaScope* scope = &C->scope_stack[C->scope_depth - 1];
    
    compile_expr(C, node->as.var.init);
    
    int slot = scope_add_local(scope, node->as.var.name, true);
    if (slot < 0) {
        compiler_error_at(C, node, "too many local variables");
    }
}

static void compile_assign(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    
    /* Compilar valor */
    compile_expr(C, node->as.assign.value);
    
    /* Resolver target */
    if (node->as.assign.target->type == NODE_IDENT) {
        const char* name = node->as.assign.target->as.string.data;
        int result = resolve_variable(C, name);
        
        if (result >= 0) {
            luna_proto_emit_op_AB(proto, OP_SET_UPVALUE, 0, (uint8_t)result);
        } else if (result == -2) {
            uint16_t idx = luna_proto_add_const_string(proto, name, strlen(name));
            luna_proto_emit_op_ABx(proto, OP_SET_GLOBAL, 0, idx);
        } else {
            compiler_error_at(C, node, "variable resolution failed");
        }
    } else {
        compiler_error_at(C, node, "complex assignment targets not yet supported");
    }
}

static void compile_return(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    
    if (node->as.return_value) {
        compile_expr(C, node->as.return_value);
        luna_proto_emit_op_AB(proto, OP_RETURN, 0, 1);
    } else {
        luna_proto_emit_op_AB(proto, OP_RETURN, 0, 0);
    }
}

static void compile_expr_stmt(LunaCompiler* C, LunaNode* node) {
    compile_expr(C, node->as.expr);
}

static void compile_if(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    
    /* Compilar condición */
    compile_expr(C, node->as.if_stmt.cond);
    
    /* JUMP_IF_FALSE al else (placeholder) */
    size_t jump_pos = proto->code_len;
    luna_proto_emit_op_AsBx(proto, OP_JUMP_IF_FALSE, 0, 0);
    
    /* Compilar then block */
    compile_block(C, node->as.if_stmt.then_block);
    
    /* JUMP al final (placeholder) */
    size_t end_jump_pos = proto->code_len;
    luna_proto_emit_op_sBx(proto, OP_JUMP, 0);
    
    /* Patchear JUMP_IF_FALSE */
    int16_t offset = (int16_t)(proto->code_len - jump_pos);
    proto->code[jump_pos + 1] = (offset >> 8) & 0xFF;
    proto->code[jump_pos + 2] = offset & 0xFF;
    
    /* Compilar elifs y else */
    /* TODO: implementar elifs */
    
    if (node->as.if_stmt.else_block) {
        compile_block(C, node->as.if_stmt.else_block);
    }
    
    /* Patchear JUMP final */
    offset = (int16_t)(proto->code_len - end_jump_pos);
    proto->code[end_jump_pos + 1] = (offset >> 8) & 0xFF;
    proto->code[end_jump_pos + 2] = offset & 0xFF;
}

static void compile_while(LunaCompiler* C, LunaNode* node) {
    LunaProto* proto = compiler_current_proto(C);
    
    size_t loop_start = proto->code_len;
    
    /* Compilar condición */
    compile_expr(C, node->as.while_stmt.cond);
    
    /* JUMP_IF_FALSE al final */
    size_t exit_jump_pos = proto->code_len;
    luna_proto_emit_op_AsBx(proto, OP_JUMP_IF_FALSE, 0, 0);
    
    /* Compilar body */
    compile_block(C, node->as.while_stmt.body);
    
    /* JUMP al inicio */
    int16_t offset = (int16_t)(loop_start - proto->code_len);
    luna_proto_emit_op_sBx(proto, OP_JUMP, offset);
    
    /* Patchear JUMP_IF_FALSE */
    offset = (int16_t)(proto->code_len - exit_jump_pos);
    proto->code[exit_jump_pos + 1] = (offset >> 8) & 0xFF;
    proto->code[exit_jump_pos + 2] = offset & 0xFF;
}

static void compile_for(LunaCompiler* C, LunaNode* node) {
    /* TODO: implementar for loops */
    compiler_error_at(C, node, "for loops not yet implemented");
}

static void compile_fn_decl(LunaCompiler* C, LunaNode* node) {
    LunaProto* parent_proto = compiler_current_proto(C);
    
    /* Crear nuevo proto para la función */
    compiler_push_proto(C, node->as.fn.name);
    LunaProto* fn_proto = compiler_current_proto(C);
    
    /* Añadir parámetros como locales */
    LunaScope* scope = &C->scope_stack[C->scope_depth - 1];
    int param_idx = 0;
    for (LunaNodeList* l = node->as.fn.params; l; l = l->next) {
        const char* param_name = l->node->as.string.data;
        scope_add_local(scope, param_name, false);
        param_idx++;
    }
    fn_proto->num_params = (uint8_t)param_idx;
    
    /* Compilar body */
    compile_block(C, node->as.fn.body);
    
    /* Emitir RETURN implícito si no hay return explícito */
    luna_proto_emit_op_AB(fn_proto, OP_RETURN, 0, 0);
    
    /* Pop proto */
    LunaProto* completed_fn = compiler_pop_proto(C);
    
    /* Añadir como proto hijo del padre */
    uint16_t proto_idx = luna_proto_add_proto(parent_proto, completed_fn);
    
    /* Emitir CLOSURE en el padre */
    luna_proto_emit_op_ABx(parent_proto, OP_CLOSURE, 0, proto_idx);
    
    /* Si es declaración (no lambda), añadir como local o global */
    if (node->as.fn.name) {
        int slot = scope_add_local(scope, node->as.fn.name, false);
        if (slot < 0) {
            compiler_error_at(C, node, "too many local variables");
        }
    }
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
    
    compile_block(C, node);
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