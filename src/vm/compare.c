#include "vm_internal.h"
#include <string.h>

/* ============================================================
 * Comparaciones
 * ============================================================ */

/*
 * Compara dos valores según el opcode.
 * Devuelve 0 en éxito (resultado en *out), -1 en error.
 *
 * Reglas:
 *   - EQ/NEQ: cualquier par de valores (usa luna_value_equal)
 *   - LT/LE/GT/GE: solo números (con coerción) o strings
 */
int vm_compare(LunaState* L, LunaValue a, LunaValue b, LunaOpcode op, int* out) {
    if (op == OP_EQ) {
        *out = luna_value_equal(a, b) ? 1 : 0;
        return 0;
    }
    if (op == OP_NEQ) {
        *out = luna_value_equal(a, b) ? 0 : 1;
        return 0;
    }

    /* Para LT/LE/GT/GE: ambos deben ser del mismo tipo numérico o strings */
    if (a.type == LUNA_TINTEGER && b.type == LUNA_TINTEGER) {
        int64_t ai = a.as.integer, bi = b.as.integer;
        int r;
        switch (op) {
            case OP_LT: r = ai < bi; break;
            case OP_LE: r = ai <= bi; break;
            case OP_GT: r = ai > bi; break;
            case OP_GE: r = ai >= bi; break;
            default: return -1;
        }
        *out = r;
        return 0;
    }

    if ((a.type == LUNA_TINTEGER || a.type == LUNA_TFLOAT) &&
        (b.type == LUNA_TINTEGER || b.type == LUNA_TFLOAT)) {
        double af = (a.type == LUNA_TFLOAT) ? a.as.floating : (double)a.as.integer;
        double bf = (b.type == LUNA_TFLOAT) ? b.as.floating : (double)b.as.integer;
        int r;
        switch (op) {
            case OP_LT: r = af < bf; break;
            case OP_LE: r = af <= bf; break;
            case OP_GT: r = af > bf; break;
            case OP_GE: r = af >= bf; break;
            default: return -1;
        }
        *out = r;
        return 0;
    }

    if (a.type == LUNA_TSTRING && b.type == LUNA_TSTRING) {
        LunaString* as = luna_val_string(a);
        LunaString* bs = luna_val_string(b);
        size_t min_len = as->len < bs->len ? as->len : bs->len;
        int cmp = memcmp(as->data, bs->data, min_len);
        int r;
        if (cmp == 0) {
            if (as->len < bs->len) cmp = -1;
            else if (as->len > bs->len) cmp = 1;
        }
        switch (op) {
            case OP_LT: r = cmp < 0; break;
            case OP_LE: r = cmp <= 0; break;
            case OP_GT: r = cmp > 0; break;
            case OP_GE: r = cmp >= 0; break;
            default: return -1;
        }
        *out = r;
        return 0;
    }

    luna_error_set(L, LUNA_ERR_TYPE, NULL, 0, 0,
                   "attempt to compare %s with %s",
                   luna_type_name(a.type), luna_type_name(b.type));
    return -1;
}