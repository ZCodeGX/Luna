#include "vm_internal.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Implementación de opcodes individuales
 * ============================================================ */

/*
 * Ejecuta una instrucción. Devuelve 0 si continuar, -1 si error,
 * 1 si se hizo una llamada (el caller debe reanudar el dispatch).
 * Actualiza frame->pc a la siguiente instrucción.
 */
int vm_execute_instruction(LunaState* L, CallFrame* frame) {
    LunaVM* vm = state_vm(L);
    const uint8_t* code = frame->proto->code;
    size_t pc = frame->pc;
    LunaOpcode op = (LunaOpcode)decode_op(code, pc);

    LunaValue* base = frame->base;
    uint8_t A = decode_A(code, pc);

    switch (op) {
        /* ---------- Constantes y literales ---------- */
        case OP_NOP:
            frame->pc = advance_pc(code, pc);
            return 0;

        case OP_LOAD_NIL:
            base[A + 1] = luna_nil();
            frame->pc = advance_pc(code, pc);
            return 0;

        case OP_LOAD_TRUE:
            base[A + 1] = luna_bool(1);
            frame->pc = advance_pc(code, pc);
            return 0;

        case OP_LOAD_FALSE:
            base[A + 1] = luna_bool(0);
            frame->pc = advance_pc(code, pc);
            return 0;

        case OP_LOAD_CONST: {
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
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_LOAD_INT: {
            int16_t sbx = decode_sBx(code, pc);
            base[A + 1] = luna_int((int64_t)sbx);
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        /* ---------- Locales y upvalues ---------- */
        case OP_MOVE: {
            uint8_t B = decode_B(code, pc);
            base[A + 1] = base[B + 1];
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_GET_UPVALUE: {
            uint8_t B = decode_B(code, pc);
            if (B >= frame->num_ups || !frame->ups[B]) {
                luna_error_set_simple(L, LUNA_ERR_RUNTIME, "invalid upvalue");
                return -1;
            }
            base[A + 1] = *frame->ups[B]->location;
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_SET_UPVALUE: {
            uint8_t B = decode_B(code, pc);
            if (B >= frame->num_ups || !frame->ups[B]) {
                luna_error_set_simple(L, LUNA_ERR_RUNTIME, "invalid upvalue");
                return -1;
            }
            *frame->ups[B]->location = base[A + 1];
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        /* ---------- Globales y campos ---------- */
        case OP_GET_GLOBAL: {
            uint16_t bx = decode_Bx(code, pc);
            LunaConst* c = &frame->proto->constants[bx];
            LunaString* key = luna_string_newz(L, c->as.string.data);
            if (!key) return -1;
            LunaValue v;
            if (!luna_table_get(L->globals, luna_string_val(key), &v)) {
                v = luna_nil();
            }
            base[A + 1] = v;
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_SET_GLOBAL: {
            uint16_t bx = decode_Bx(code, pc);
            LunaConst* c = &frame->proto->constants[bx];
            LunaString* key = luna_string_newz(L, c->as.string.data);
            if (!key) return -1;
            luna_table_set(L, L->globals, luna_string_val(key), base[A + 1]);
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_GET_FIELD: {
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
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_SET_FIELD: {
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
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_GET_INDEX: {
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
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_SET_INDEX: {
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
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        /* ---------- Tablas y listas ---------- */
        case OP_NEW_TABLE: {
            LunaTable* t = luna_table_new(L);
            if (!t) return -1;
            base[A + 1] = luna_table_val(t);
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_NEW_LIST: {
            LunaList* list = luna_list_new(L);
            if (!list) return -1;
            base[A + 1] = luna_list_val(list);
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_LIST_APPEND: {
            uint8_t B = decode_B(code, pc);
            LunaValue list_val = base[A + 1];
            LunaValue val = base[B + 1];
            if (list_val.type != LUNA_TLIST) {
                luna_error_set_simple(L, LUNA_ERR_TYPE, "list_append on non-list");
                return -1;
            }
            luna_list_push_value(L, luna_val_list(list_val), val);
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_TABLE_SET: {
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
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        /* ---------- Aritmética ---------- */
        case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV:
        case OP_MOD: case OP_POW: {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            LunaValue result;
            if (vm_arith(L, base[B + 1], base[C + 1], &result, op) != 0) return -1;
            base[A + 1] = result;
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_NEG: {
            uint8_t B = decode_B(code, pc);
            LunaValue result;
            if (vm_neg(L, base[B + 1], &result) != 0) return -1;
            base[A + 1] = result;
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        /* ---------- Comparaciones ---------- */
        case OP_EQ: case OP_NEQ: case OP_LT: case OP_LE:
        case OP_GT: case OP_GE: {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            int result;
            if (vm_compare(L, base[B + 1], base[C + 1], op, &result) != 0) return -1;
            base[A + 1] = luna_bool(result);
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        /* ---------- Lógica ---------- */
        case OP_NOT: {
            uint8_t B = decode_B(code, pc);
            base[A + 1] = luna_bool(!luna_value_truthy(base[B + 1]));
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_AND: {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            base[A + 1] = luna_value_truthy(base[B + 1]) ? base[C + 1] : base[B + 1];
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_OR: {
            uint8_t B = decode_B(code, pc);
            uint8_t C = decode_C(code, pc);
            base[A + 1] = luna_value_truthy(base[B + 1]) ? base[B + 1] : base[C + 1];
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        /* ---------- Concatenación y longitud ---------- */
        case OP_CONCAT: {
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
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_LEN: {
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
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        /* ---------- Control de flujo ---------- */
        case OP_JUMP: {
            uint16_t bx = (uint16_t)code[pc + 1] | ((uint16_t)code[pc + 2] << 8);
            int16_t sbx = (int16_t)(bx - 32767);
            frame->pc = (size_t)((int64_t)advance_pc(code, pc) + sbx);
            return 0;
        }

        case OP_JUMP_IF_TRUE: {
            int16_t sbx = decode_sBx(code, pc);
            size_t next = advance_pc(code, pc);
            if (luna_value_truthy(base[A + 1])) {
                frame->pc = (size_t)((int64_t)next + sbx);
            } else {
                frame->pc = next;
            }
            return 0;
        }

        case OP_JUMP_IF_FALSE: {
            int16_t sbx = decode_sBx(code, pc);
            size_t next = advance_pc(code, pc);
            if (!luna_value_truthy(base[A + 1])) {
                frame->pc = (size_t)((int64_t)next + sbx);
            } else {
                frame->pc = next;
            }
            return 0;
        }

        /* ---------- Llamar y retornar ---------- */
        case OP_CALL: {
            uint8_t B = decode_B(code, pc);   /* nargs */
            uint8_t C = decode_C(code, pc);   /* nresults */
            int func_idx = (int)(A + 1);
            LunaVM* vm2 = state_vm(L);
            int old_top = vm2->frame_top;

            int result = vm_call_function(L, (int)(frame->base - L->stack) + func_idx,
                                          B, C == 0 ? -1 : C);
            if (result != 0) return -1;

            if (vm2->frame_top > old_top) {
                return 1;  /* llamada Luna: reanudar con el nuevo frame */
            }

            frame->pc = advance_pc(code, pc);
            return 0;
        }

        case OP_RETURN: {
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

            if (vm->frame_top == 0) {
                /* Retorno del chunk principal: terminar */
                L->stack_top = ret_start;
                return 2;  /* señal: terminar ejecución */
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
            return 1;  /* señal: reanudar con frame anterior */
        }

        /* ---------- Closures ---------- */
        case OP_CLOSURE: {
            uint16_t bx = decode_Bx(code, pc);
            LunaProto* child = frame->proto->protos[bx];

            LunaFunction* fn = luna_function_new(L, child);
            if (!fn) return -1;

            /* Capturar upvalues */
            for (size_t i = 0; i < child->num_upvalues; i++) {
                LunaUpvalueDesc* desc = &child->upvalues[i];
                LunaUpvalue* uv;
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
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        /* ---------- Iteración (for) ---------- */
        case OP_FOR_INIT:
        case OP_FOR_STEP:
            /* TODO: implementar for loops */
            luna_error_set_simple(L, LUNA_ERR_RUNTIME, "for loops not yet implemented");
            return -1;

        /* ---------- Excepciones ---------- */
        case OP_THROW: {
            LunaValue v = base[A + 1];
            if (vm_throw(L, v) != 0) return -1;
            /* Si hay handler, frame->pc ya está actualizado */
            return 0;
        }

        case OP_SETUP_EXC: {
            int16_t sbx = decode_sBx(code, pc);
            size_t next = advance_pc(code, pc);
            frame->handler_pc = (int)((int64_t)next + sbx);
            frame->pc = next;
            return 0;
        }

        case OP_POP_EXC: {
            frame->handler_pc = -1;
            frame->pc = advance_pc(code, pc);
            return 0;
        }

        default:
            luna_error_set(L, LUNA_ERR_RUNTIME, NULL, 0, 0,
                           "invalid opcode %d", (int)op);
            return -1;
    }
}