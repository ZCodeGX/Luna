#include "vm_internal.h"
#include "../modules/module.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern int luna_report_instruction_limit(LunaState* L);

#if defined(__GNUC__) || defined(__clang__)
#define LUNA_THREADED_DISPATCH 1
#define VM_CASE(op) label_##op:
#define VM_SWITCH_BEGIN
#define VM_SWITCH_END
#define VM_DISPATCH_NEXT() goto *dispatch_table[op]
#define VM_END_INSTR() VM_NEXT()
#define VM_FRAME_CHANGED() do { VM_RELOAD_FRAME(); VM_NEXT(); } while (0)
#else
#define VM_CASE(op) case op:
#define VM_SWITCH_BEGIN vm_switch: switch (op) {
#define VM_SWITCH_END }
#define VM_DISPATCH_NEXT() ((void)0)
#define VM_END_INSTR() break
#define VM_FRAME_CHANGED() do { VM_RELOAD_FRAME(); goto vm_done; } while (0)
#endif

#define VM_ADVANCE() do { pc = advance_pc(code, pc); frame->pc = pc; } while (0)
#define VM_SET_PC(new_pc) do { pc = (size_t)(new_pc); frame->pc = pc; } while (0)
#define VM_SYNC_PC() (pc = frame->pc)
#define VM_SYNC_BASE() (base = frame->base)
#define VM_FINISHED() return 0
#define VM_RELOAD_FRAME() do { \
    frame = &vm->frames[vm->frame_top - 1]; \
    code = frame->proto->code; \
    pc = frame->pc; \
    base = frame->base; \
} while (0)
#define VM_NEXT() do { \
    L->instructions_executed++; \
    if (--budget == 0) { \
        budget = LUNA_INSTRUCTION_BUDGET; \
        if (L->instruction_limit != 0) { \
            uint64_t remaining = L->instructions_executed <= L->instruction_limit \
                ? L->instruction_limit - L->instructions_executed + 1 : 0; \
            if (remaining == 0) goto vm_limit; \
            if (remaining < budget) budget = remaining; \
        } \
    } \
    op = decode_op(code, pc); \
    A = decode_A(code, pc); \
    if ((unsigned)op >= OP_COUNT) goto vm_invalid; \
    VM_DISPATCH_NEXT(); \
} while (0)

/* ============================================================
 * Bucle de ejecución con dispatch
 * ============================================================ */

int vm_execute_loop(LunaState* L) {
    LunaVM* vm = state_vm(L);
    CallFrame* frame;
    const uint8_t* code;
    LunaValue* base;
    size_t pc;
    uint8_t op;
    uint8_t A;
    uint64_t budget = vm->instruction_budget ? vm->instruction_budget
                                             : LUNA_INSTRUCTION_BUDGET;

    if (L->instruction_limit != 0 && L->instruction_limit < budget - 1) {
        budget = L->instruction_limit + 1;
    }

#ifdef LUNA_THREADED_DISPATCH
    static void* const dispatch_table[OP_COUNT] = {
        [OP_NOP] = &&label_OP_NOP,
        [OP_LOAD_NIL] = &&label_OP_LOAD_NIL,
        [OP_LOAD_TRUE] = &&label_OP_LOAD_TRUE,
        [OP_LOAD_FALSE] = &&label_OP_LOAD_FALSE,
        [OP_LOAD_CONST] = &&label_OP_LOAD_CONST,
        [OP_LOAD_INT] = &&label_OP_LOAD_INT,
        [OP_MOVE] = &&label_OP_MOVE,
        [OP_GET_UPVALUE] = &&label_OP_GET_UPVALUE,
        [OP_SET_UPVALUE] = &&label_OP_SET_UPVALUE,
        [OP_GET_GLOBAL] = &&label_OP_GET_GLOBAL,
        [OP_SET_GLOBAL] = &&label_OP_SET_GLOBAL,
        [OP_GET_FIELD] = &&label_OP_GET_FIELD,
        [OP_SET_FIELD] = &&label_OP_SET_FIELD,
        [OP_GET_INDEX] = &&label_OP_GET_INDEX,
        [OP_SET_INDEX] = &&label_OP_SET_INDEX,
        [OP_NEW_TABLE] = &&label_OP_NEW_TABLE,
        [OP_NEW_LIST] = &&label_OP_NEW_LIST,
        [OP_LIST_APPEND] = &&label_OP_LIST_APPEND,
        [OP_TABLE_SET] = &&label_OP_TABLE_SET,
        [OP_ADD] = &&label_OP_ADD,
        [OP_SUB] = &&label_OP_SUB,
        [OP_MUL] = &&label_OP_MUL,
        [OP_DIV] = &&label_OP_DIV,
        [OP_MOD] = &&label_OP_MOD,
        [OP_POW] = &&label_OP_POW,
        [OP_NEG] = &&label_OP_NEG,
        [OP_EQ] = &&label_OP_EQ,
        [OP_NEQ] = &&label_OP_NEQ,
        [OP_LT] = &&label_OP_LT,
        [OP_LE] = &&label_OP_LE,
        [OP_GT] = &&label_OP_GT,
        [OP_GE] = &&label_OP_GE,
        [OP_NOT] = &&label_OP_NOT,
        [OP_AND] = &&label_OP_AND,
        [OP_OR] = &&label_OP_OR,
        [OP_CONCAT] = &&label_OP_CONCAT,
        [OP_LEN] = &&label_OP_LEN,
        [OP_JUMP] = &&label_OP_JUMP,
        [OP_JUMP_IF_TRUE] = &&label_OP_JUMP_IF_TRUE,
        [OP_JUMP_IF_FALSE] = &&label_OP_JUMP_IF_FALSE,
        [OP_CALL] = &&label_OP_CALL,
        [OP_RETURN] = &&label_OP_RETURN,
        [OP_CLOSURE] = &&label_OP_CLOSURE,
        [OP_FOR_INIT] = &&label_OP_FOR_INIT,
        [OP_FOR_STEP] = &&label_OP_FOR_STEP,
        [OP_THROW] = &&label_OP_THROW,
        [OP_SETUP_EXC] = &&label_OP_SETUP_EXC,
        [OP_POP_EXC] = &&label_OP_POP_EXC,
        [OP_IMPORT] = &&label_OP_IMPORT
    };
#endif

    if (vm->frame_top <= vm->return_level) return 0;
    VM_RELOAD_FRAME();

    for (;;) {
        VM_NEXT();

VM_SWITCH_BEGIN
        /* ---------- Constantes y literales ---------- */
        VM_CASE(OP_NOP)
            VM_ADVANCE();
            VM_END_INSTR();

        VM_CASE(OP_LOAD_NIL)
            base[A + 1] = luna_nil();
            VM_ADVANCE();
            VM_END_INSTR();

        VM_CASE(OP_LOAD_TRUE)
            base[A + 1] = luna_bool(1);
            VM_ADVANCE();
            VM_END_INSTR();

        VM_CASE(OP_LOAD_FALSE)
            base[A + 1] = luna_bool(0);
            VM_ADVANCE();
            VM_END_INSTR();

        VM_CASE(OP_LOAD_CONST) {
            uint16_t bx = decode_Bx(code, pc);
            LunaConst* c = &frame->proto->constants[bx];
            LunaValue v;
            switch (c->type) {
                case LUNA_CONST_NIL:   v = luna_nil(); break;
                case LUNA_CONST_BOOL:  v = luna_bool(c->as.boolean); break;
                case LUNA_CONST_INT:   v = luna_int(c->as.integer); break;
                case LUNA_CONST_FLOAT: v = luna_float(c->as.floating); break;
                case LUNA_CONST_STRING: {
                    LunaString* s = luna_string_new(L, c->as.string.data, c->as.string.len);
                    if (!s) return -1;
                    v = luna_string_val(s);
                    break;
                }
            }
            base[A + 1] = v;
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_LOAD_INT) {
            int16_t sbx = decode_sBx(code, pc);
            base[A + 1] = luna_int((int64_t)sbx);
            VM_ADVANCE();
            VM_END_INSTR();
        }

        /* ---------- Locales y upvalues ---------- */
        VM_CASE(OP_MOVE) {
            uint8_t B = decode_B(code, pc);
            base[A + 1] = base[B + 1];
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_GET_UPVALUE) {
            uint8_t B = decode_B(code, pc);
            if (B >= frame->num_ups || !frame->ups[B]) {
                luna_error_set_simple(L, LUNA_ERR_RUNTIME, "invalid upvalue");
                return -1;
            }
            base[A + 1] = *frame->ups[B]->location;
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_SET_UPVALUE) {
            uint8_t B = decode_B(code, pc);
            if (B >= frame->num_ups || !frame->ups[B]) {
                luna_error_set_simple(L, LUNA_ERR_RUNTIME, "invalid upvalue");
                return -1;
            }
            *frame->ups[B]->location = base[A + 1];
            VM_ADVANCE();
            VM_END_INSTR();
        }

        /* ---------- Globales y campos ---------- */
        VM_CASE(OP_GET_GLOBAL) {
            uint16_t bx = decode_Bx(code, pc);
            LunaConst* c = &frame->proto->constants[bx];
            LunaString* key = luna_string_newz(L, c->as.string.data);
            if (!key) return -1;
            LunaValue v;
            if (!luna_table_get(L->globals, luna_string_val(key), &v)) {
                v = luna_nil();
            }
            base[A + 1] = v;
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_SET_GLOBAL) {
            uint16_t bx = decode_Bx(code, pc);
            LunaConst* c = &frame->proto->constants[bx];
            LunaString* key = luna_string_newz(L, c->as.string.data);
            if (!key) return -1;
            luna_table_set(L, L->globals, luna_string_val(key), base[A + 1]);
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_IMPORT) {
            uint16_t bx = decode_Bx(code, pc);
            LunaConst* c = &frame->proto->constants[bx];
            LunaTable* exports = luna_import_module(L, c->as.string.data);
            if (!exports) return -1;
            base[A + 1] = luna_table_val(exports);
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_GET_FIELD) {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            LunaConst* c = &frame->proto->constants[C];
            LunaString* key = luna_string_newz(L, c->as.string.data);
            if (!key) return -1;
            LunaValue obj = base[B + 1];
            if (obj.type != LUNA_TTABLE) {
                luna_error_set(L, LUNA_ERR_TYPE, NULL, 0, 0,
                               "attempt to index a %s value", luna_type_name(obj.type));
                return -1;
            }
            LunaValue v;
            if (!luna_table_get(luna_val_table(obj), luna_string_val(key), &v)) {
                v = luna_nil();
            }
            base[A + 1] = v;
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_SET_FIELD) {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            LunaConst* c = &frame->proto->constants[C];
            LunaString* key = luna_string_newz(L, c->as.string.data);
            if (!key) return -1;
            LunaValue obj = base[A + 1];
            if (obj.type != LUNA_TTABLE) {
                luna_error_set(L, LUNA_ERR_TYPE, NULL, 0, 0,
                               "attempt to index a %s value", luna_type_name(obj.type));
                return -1;
            }
            luna_table_set(L, luna_val_table(obj), luna_string_val(key), base[B + 1]);
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_GET_INDEX) {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            LunaValue obj = base[B + 1];
            LunaValue key = base[C + 1];
            if (obj.type == LUNA_TLIST) {
                LunaList* list = luna_val_list(obj);
                if (key.type != LUNA_TINTEGER) {
                    luna_error_set_simple(L, LUNA_ERR_TYPE, "list index must be integer");
                    return -1;
                }
                int64_t idx = key.as.integer;
                if (idx < 0 || (size_t)idx >= list->length) {
                    luna_error_set(L, LUNA_ERR_INDEX, NULL, 0, 0,
                                   "list index %lld out of range (len %zu)",
                                   (long long)idx, list->length);
                    return -1;
                }
                base[A + 1] = list->items[idx];
            } else if (obj.type == LUNA_TTABLE) {
                LunaValue v;
                if (!luna_table_get(luna_val_table(obj), key, &v)) {
                    v = luna_nil();
                }
                base[A + 1] = v;
            } else {
                luna_error_set(L, LUNA_ERR_TYPE, NULL, 0, 0,
                               "attempt to index a %s value", luna_type_name(obj.type));
                return -1;
            }
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_SET_INDEX) {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            LunaValue obj = base[A + 1];
            LunaValue key = base[B + 1];
            LunaValue val = base[C + 1];
            if (obj.type == LUNA_TLIST) {
                LunaList* list = luna_val_list(obj);
                if (key.type != LUNA_TINTEGER) {
                    luna_error_set_simple(L, LUNA_ERR_TYPE, "list index must be integer");
                    return -1;
                }
                int64_t idx = key.as.integer;
                if (idx < 0 || (size_t)idx >= list->length) {
                    luna_error_set(L, LUNA_ERR_INDEX, NULL, 0, 0,
                                   "list index %lld out of range", (long long)idx);
                    return -1;
                }
                list->items[idx] = val;
            } else if (obj.type == LUNA_TTABLE) {
                luna_table_set(L, luna_val_table(obj), key, val);
            } else {
                luna_error_set(L, LUNA_ERR_TYPE, NULL, 0, 0,
                               "attempt to index a %s value", luna_type_name(obj.type));
                return -1;
            }
            VM_ADVANCE();
            VM_END_INSTR();
        }

        /* ---------- Tablas y listas ---------- */
        VM_CASE(OP_NEW_TABLE) {
            LunaTable* t = luna_table_new(L);
            if (!t) return -1;
            base[A + 1] = luna_table_val(t);
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_NEW_LIST) {
            LunaList* list = luna_list_new(L);
            if (!list) return -1;
            base[A + 1] = luna_list_val(list);
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_LIST_APPEND) {
            uint8_t B = decode_B(code, pc);
            LunaValue list_val = base[A + 1];
            LunaValue val = base[B + 1];
            if (list_val.type != LUNA_TLIST) {
                luna_error_set_simple(L, LUNA_ERR_TYPE, "list_append on non-list");
                return -1;
            }
            luna_list_push_value(L, luna_val_list(list_val), val);
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_TABLE_SET) {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            LunaValue tbl = base[A + 1];
            LunaValue key = base[B + 1];
            LunaValue val = base[C + 1];
            if (tbl.type != LUNA_TTABLE) {
                luna_error_set_simple(L, LUNA_ERR_TYPE, "table_set on non-table");
                return -1;
            }
            luna_table_set(L, luna_val_table(tbl), key, val);
            VM_ADVANCE();
            VM_END_INSTR();
        }

        /* ---------- Aritmética ---------- */
        VM_CASE(OP_ADD) VM_CASE(OP_SUB) VM_CASE(OP_MUL) VM_CASE(OP_DIV)
        VM_CASE(OP_MOD) VM_CASE(OP_POW) {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            LunaValue av = base[B + 1];
            LunaValue bv = base[C + 1];
            LunaValue result;

            if (av.type == LUNA_TINTEGER && bv.type == LUNA_TINTEGER) {
                int64_t ai = av.as.integer;
                int64_t bi = bv.as.integer;
                switch (op) {
                    case OP_ADD: result = luna_int(ai + bi); break;
                    case OP_SUB: result = luna_int(ai - bi); break;
                    case OP_MUL: result = luna_int(ai * bi); break;
                    case OP_DIV:
                        if (bi == 0) {
                            luna_error_set_simple(L, LUNA_ERR_DIVZERO, "division by zero");
                            return -1;
                        }
                        result = luna_float((double)ai / (double)bi);
                        break;
                    case OP_MOD:
                        if (bi == 0) {
                            luna_error_set_simple(L, LUNA_ERR_DIVZERO, "modulo by zero");
                            return -1;
                        }
                        result = luna_int(ai % bi);
                        break;
                    case OP_POW: result = luna_float(pow((double)ai, (double)bi)); break;
                    default: return -1;
                }
            } else if (vm_arith(L, av, bv, &result, op) != 0) {
                return -1;
            }

            base[A + 1] = result;
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_NEG) {
            uint8_t B = decode_B(code, pc);
            LunaValue result;
            if (vm_neg(L, base[B + 1], &result) != 0) return -1;
            base[A + 1] = result;
            VM_ADVANCE();
            VM_END_INSTR();
        }

        /* ---------- Comparaciones ---------- */
        VM_CASE(OP_EQ) VM_CASE(OP_NEQ) VM_CASE(OP_LT) VM_CASE(OP_LE)
        VM_CASE(OP_GT) VM_CASE(OP_GE) {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            LunaValue av = base[B + 1];
            LunaValue bv = base[C + 1];
            int result;

            if (av.type == LUNA_TINTEGER && bv.type == LUNA_TINTEGER) {
                int64_t ai = av.as.integer;
                int64_t bi = bv.as.integer;
                switch (op) {
                    case OP_EQ:  result = ai == bi; break;
                    case OP_NEQ: result = ai != bi; break;
                    case OP_LT:  result = ai <  bi; break;
                    case OP_LE:  result = ai <= bi; break;
                    case OP_GT:  result = ai >  bi; break;
                    case OP_GE:  result = ai >= bi; break;
                    default: return -1;
                }
            } else if (vm_compare(L, av, bv, op, &result) != 0) {
                return -1;
            }

            base[A + 1] = luna_bool(result);
            VM_ADVANCE();
            VM_END_INSTR();
        }

        /* ---------- Lógica ---------- */
        VM_CASE(OP_NOT) {
            uint8_t B = decode_B(code, pc);
            base[A + 1] = luna_bool(!luna_value_truthy(base[B + 1]));
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_AND) {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            base[A + 1] = luna_value_truthy(base[B + 1]) ? base[C + 1] : base[B + 1];
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_OR) {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            base[A + 1] = luna_value_truthy(base[B + 1]) ? base[B + 1] : base[C + 1];
            VM_ADVANCE();
            VM_END_INSTR();
        }

        /* ---------- Concatenación y longitud ---------- */
        VM_CASE(OP_CONCAT) {
            /* Simplificado: concatenar strings de B a C */
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            /* Calcular longitud total */
            size_t total = 0;
            for (uint8_t i = B; i <= C; i++) {
                LunaValue v = base[i + 1];
                if (v.type != LUNA_TSTRING) {
                    luna_error_set_simple(L, LUNA_ERR_TYPE, "concat requires strings");
                    return -1;
                }
                total += luna_val_string(v)->len;
            }
            char* buf = (char*)luna_alloc(L, NULL, 0, total + 1);
            if (!buf) return -1;
            size_t pos = 0;
            for (uint8_t i = B; i <= C; i++) {
                LunaString* s = luna_val_string(base[i + 1]);
                memcpy(buf + pos, s->data, s->len);
                pos += s->len;
            }
            buf[total] = '\0';
            LunaString* result = luna_string_new(L, buf, total);
            luna_alloc(L, buf, total + 1, 0);
            if (!result) return -1;
            base[A + 1] = luna_string_val(result);
            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_LEN) {
            uint8_t B = decode_B(code, pc);
            LunaValue v = base[B + 1];
            int64_t len;
            switch (v.type) {
                case LUNA_TSTRING: len = (int64_t)luna_val_string(v)->len; break;
                case LUNA_TLIST:   len = (int64_t)luna_val_list(v)->length; break;
                case LUNA_TTABLE:  len = (int64_t)luna_table_count(luna_val_table(v)); break;
                default:
                    luna_error_set(L, LUNA_ERR_TYPE, NULL, 0, 0,
                                   "attempt to get length of a %s value",
                                   luna_type_name(v.type));
                    return -1;
            }
            base[A + 1] = luna_int(len);
            VM_ADVANCE();
            VM_END_INSTR();
        }

        /* ---------- Control de flujo ---------- */
        VM_CASE(OP_JUMP) {
            uint16_t bx = (uint16_t)code[pc + 1] | ((uint16_t)code[pc + 2] << 8);
            int16_t sbx = (int16_t)(bx - 32767);
            VM_SET_PC((size_t)((int64_t)advance_pc(code, pc) + sbx));
            VM_END_INSTR();
        }

        VM_CASE(OP_JUMP_IF_TRUE) {
            int16_t sbx = decode_sBx(code, pc);
            size_t next = advance_pc(code, pc);
            if (luna_value_truthy(base[A + 1])) {
                VM_SET_PC((size_t)((int64_t)next + sbx));
            } else {
                VM_SET_PC(next);
            }
            VM_END_INSTR();
        }

        VM_CASE(OP_JUMP_IF_FALSE) {
            int16_t sbx = decode_sBx(code, pc);
            size_t next = advance_pc(code, pc);
            if (!luna_value_truthy(base[A + 1])) {
                VM_SET_PC((size_t)((int64_t)next + sbx));
            } else {
                VM_SET_PC(next);
            }
            VM_END_INSTR();
        }

        /* ---------- Llamar y retornar ---------- */
        VM_CASE(OP_CALL) {
            uint8_t B = decode_B(code, pc);   /* nargs */
            uint8_t C = decode_C(code, pc);   /* nresults */
            int func_idx = (int)(A + 1);
            LunaVM* vm2 = state_vm(L);
            int old_top = vm2->frame_top;

            int result = vm_call_function(L, (int)(frame->base - L->stack) + func_idx,
                                          B, C == 0 ? -1 : C);
            if (result != 0) return -1;
            VM_SYNC_BASE();

            if (vm2->frame_top > old_top) {
                VM_FRAME_CHANGED();
            }

            VM_ADVANCE();
            VM_END_INSTR();
        }

        VM_CASE(OP_RETURN) {
            uint8_t B = decode_B(code, pc);  /* número de resultados */
            int nresults = B == 0 ? 0 : B - 1;  /* B=1 → 0 resultados, B=2 → 1 resultado */
            /* B=0 en nuestra convención = return sin valor */

            /* Cerrar upvalues */
            vm_close_upvalues(L, frame->base);

            /* Copiar resultados a la posición de la función */
            LunaValue* ret_base = frame->base;
            size_t ret_start = (size_t)(ret_base - L->stack) + (size_t)A + 1;

            /* Pop del frame */
            vm->frame_top--;

            if (vm->frame_top <= vm->return_level) {
                /* Retorno del chunk principal: terminar */
                L->stack_top = ret_start;
                VM_FINISHED();
            }

            /* Volver al frame anterior */
            CallFrame* prev = &vm->frames[vm->frame_top - 1];

            /* Mover resultados a la posición de la función en el frame anterior */
            size_t func_idx = (size_t)(ret_base - L->stack);
            int expected = prev->num_results;

            for (int i = 0; i < nresults; i++) {
                L->stack[func_idx + (size_t)i] = L->stack[ret_start + (size_t)i];
            }
            if (expected >= 0) {
                for (int i = nresults; i < expected; i++) {
                    L->stack[func_idx + (size_t)i] = luna_nil();
                }
                L->stack_top = func_idx + (size_t)(expected > 0 ? expected : 0);
            } else {
                L->stack_top = func_idx + (size_t)nresults;
            }

            /* Continuar en el frame anterior */
            prev->pc = advance_pc(prev->proto->code, prev->pc);
            VM_FRAME_CHANGED();
        }

        /* ---------- Closures ---------- */
        VM_CASE(OP_CLOSURE) {
            uint16_t bx = decode_Bx(code, pc);
            LunaProto* child = frame->proto->protos[bx];

            LunaFunction* fn = luna_function_new(L, child);
            if (!fn) return -1;

            /* Capturar upvalues */
            for (size_t i = 0; i < child->num_upvalues; i++) {
                LunaUpvalueDesc* desc = &child->upvalues[i];
                LunaUpvalue* uv = NULL;
                if (desc->in_stack) {
                    uv = vm_find_upvalue(L, frame, &frame->base[desc->index + 1]);
                } else if (desc->index < frame->num_ups) {
                    uv = frame->ups[desc->index];
                }
                if (!uv) {
                    luna_error_set_simple(L, LUNA_ERR_RUNTIME, "invalid upvalue capture");
                    return -1;
                }
                fn->upvalues[i] = uv;
            }

            base[A + 1] = luna_function_val(fn);
            VM_ADVANCE();
            VM_END_INSTR();
        }

        /* ---------- Iteración (for) ---------- */
        VM_CASE(OP_FOR_INIT)
        VM_CASE(OP_FOR_STEP)
            /* TODO: implementar for loops */
            luna_error_set_simple(L, LUNA_ERR_RUNTIME, "for loops not yet implemented");
            return -1;

        /* ---------- Excepciones ---------- */
        VM_CASE(OP_THROW) {
            LunaValue v = base[A + 1];
            if (vm_throw(L, v) != 0) return -1;
            VM_SYNC_PC();
            VM_END_INSTR();
        }

        VM_CASE(OP_SETUP_EXC) {
            int16_t sbx = decode_sBx(code, pc);
            size_t next = advance_pc(code, pc);
            frame->handler_pc = (int)((int64_t)next + sbx);
            VM_SET_PC(next);
            VM_END_INSTR();
        }

        VM_CASE(OP_POP_EXC) {
            frame->handler_pc = -1;
            VM_ADVANCE();
            VM_END_INSTR();
        }

VM_SWITCH_END

#if !defined(LUNA_THREADED_DISPATCH)
vm_done:
        ;
#endif
    }

vm_limit:
    vm->instruction_budget = budget;
    luna_report_instruction_limit(L);
    return -1;

vm_invalid:
    luna_error_set(L, LUNA_ERR_RUNTIME, NULL, 0, 0,
                   "invalid opcode %d", (int)op);
    return -1;
}