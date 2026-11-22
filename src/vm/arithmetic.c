#include "vm_internal.h"
#include <math.h>

/* ============================================================
 * Coerción numérica
 * ============================================================ */

/*
 * Intenta convertir un valor a float.
 * Devuelve 1 si OK, 0 si no es convertible.
 */
static int to_float(LunaValue v, double* out) {
    switch (v.type) {
        case LUNA_TFLOAT:   *out = v.as.floating; return 1;
        case LUNA_TINTEGER: *out = (double)v.as.integer; return 1;
        default: return 0;
    }
}

/* ============================================================
 * Operaciones aritméticas
 * ============================================================ */

/*
 * Ejecuta una operación aritmética binaria.
 * Devuelve 0 en éxito, -1 en error de tipo.
 *
 * Reglas de coerción:
 *   - Si ambos son int → resultado int (excepto división y potencia)
 *   - Si alguno es float → resultado float
 *   - Si ninguno es numérico → error
 */
int vm_arith(LunaState* L, LunaValue a, LunaValue b, LunaValue* out, LunaOpcode op) {
    int a_is_int = (a.type == LUNA_TINTEGER);
    int b_is_int = (b.type == LUNA_TINTEGER);

    /* Caso rápido: ambos enteros */
    if (a_is_int && b_is_int) {
        int64_t ai = a.as.integer, bi = b.as.integer;
        int64_t r;
        switch (op) {
            case OP_ADD: r = ai + bi; break;
            case OP_SUB: r = ai - bi; break;
            case OP_MUL: r = ai * bi; break;
            case OP_DIV:
                /* División entera → float (como Python 3) */
                if (bi == 0) {
                    luna_error_set_simple(L, LUNA_ERR_DIVZERO, "division by zero");
                    return -1;
                }
                *out = luna_float((double)ai / (double)bi);
                return 0;
            case OP_MOD:
                if (bi == 0) {
                    luna_error_set_simple(L, LUNA_ERR_DIVZERO, "modulo by zero");
                    return -1;
                }
                r = ai % bi;
                break;
            case OP_POW:
                /* Potencia → float */
                *out = luna_float(pow((double)ai, (double)bi));
                return 0;
            default:
                luna_error_set_simple(L, LUNA_ERR_RUNTIME, "invalid arithmetic opcode");
                return -1;
        }
        *out = luna_int(r);
        return 0;
    }

    /* Caso: al menos uno es float */
    double af, bf;
    if (!to_float(a, &af) || !to_float(b, &bf)) {
        luna_error_set(L, LUNA_ERR_TYPE, NULL, 0, 0,
                       "attempt to perform arithmetic on a %s value",
                       luna_type_name(a.type == LUNA_TNIL || a.type == LUNA_TBOOLEAN ||
                                      a.type == LUNA_TSTRING || a.type == LUNA_TTABLE ||
                                      a.type == LUNA_TLIST || a.type == LUNA_TFUNCTION ||
                                      a.type == LUNA_TUSERDATA ? a.type : b.type));
        return -1;
    }

    double r;
    switch (op) {
        case OP_ADD: r = af + bf; break;
        case OP_SUB: r = af - bf; break;
        case OP_MUL: r = af * bf; break;
        case OP_DIV:
            if (bf == 0.0) {
                luna_error_set_simple(L, LUNA_ERR_DIVZERO, "division by zero");
                return -1;
            }
            r = af / bf;
            break;
        case OP_MOD:
            if (bf == 0.0) {
                luna_error_set_simple(L, LUNA_ERR_DIVZERO, "modulo by zero");
                return -1;
            }
            r = fmod(af, bf);
            break;
        case OP_POW: r = pow(af, bf); break;
        default:
            luna_error_set_simple(L, LUNA_ERR_RUNTIME, "invalid arithmetic opcode");
            return -1;
    }
    *out = luna_float(r);
    return 0;
}

/*
 * Negación unaria.
 */
int vm_neg(LunaState* L, LunaValue a, LunaValue* out) {
    switch (a.type) {
        case LUNA_TINTEGER: *out = luna_int(-a.as.integer); return 0;
        case LUNA_TFLOAT:   *out = luna_float(-a.as.floating); return 0;
        default:
            luna_error_set(L, LUNA_ERR_TYPE, NULL, 0, 0,
                           "attempt to negate a %s value",
                           luna_type_name(a.type));
            return -1;
    }
}