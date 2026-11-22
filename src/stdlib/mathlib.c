#include "stdlib_internal.h"

#include <math.h>
#include <stdlib.h>

#define LUNA_M_PI 3.14159265358979323846
#define LUNA_M_E  2.71828182845904523536

static int math_num(LunaState* L, int index, double* out) {
    LunaValue v = lib_arg(L, index);
    if (v.type == LUNA_TINTEGER) {
        *out = (double)luna_val_int(v);
        return 0;
    }
    if (v.type == LUNA_TFLOAT) {
        *out = luna_val_float(v);
        return 0;
    }
    return lib_error(L, "expected number, got %s", luna_type_name(v.type));
}

static int math_unary(LunaState* L, double (*fn)(double)) {
    double x;
    if (math_num(L, 0, &x) != 0) return -1;
    luna_pushfloat(L, fn(x));
    return 1;
}

static int m_abs(LunaState* L) {
    LunaValue v = lib_arg(L, 0);
    if (v.type == LUNA_TINTEGER) {
        int64_t n = luna_val_int(v);
        if (n == INT64_MIN) {
            luna_pushfloat(L, -(double)n);
            return 1;
        }
        luna_pushinteger(L, n < 0 ? -n : n);
        return 1;
    }
    return math_unary(L, fabs);
}

static int m_sqrt(LunaState* L) { return math_unary(L, sqrt); }
static int m_cbrt(LunaState* L) { return math_unary(L, cbrt); }
static int m_floor(LunaState* L) { return math_unary(L, floor); }
static int m_ceil(LunaState* L) { return math_unary(L, ceil); }
static int m_round(LunaState* L) { return math_unary(L, round); }
static int m_trunc(LunaState* L) { return math_unary(L, trunc); }
static int m_sin(LunaState* L) { return math_unary(L, sin); }
static int m_cos(LunaState* L) { return math_unary(L, cos); }
static int m_tan(LunaState* L) { return math_unary(L, tan); }
static int m_asin(LunaState* L) { return math_unary(L, asin); }
static int m_acos(LunaState* L) { return math_unary(L, acos); }
static int m_atan(LunaState* L) { return math_unary(L, atan); }
static int m_sinh(LunaState* L) { return math_unary(L, sinh); }
static int m_cosh(LunaState* L) { return math_unary(L, cosh); }
static int m_tanh(LunaState* L) { return math_unary(L, tanh); }
static int m_exp(LunaState* L) { return math_unary(L, exp); }
static int m_log(LunaState* L) { return math_unary(L, log); }
static int m_log10(LunaState* L) { return math_unary(L, log10); }
static int m_log2(LunaState* L) { return math_unary(L, log2); }

static int m_atan2(LunaState* L) {
    double y, x;
    if (math_num(L, 0, &y) != 0 || math_num(L, 1, &x) != 0) return -1;
    luna_pushfloat(L, atan2(y, x));
    return 1;
}

static int m_pow(LunaState* L) {
    double a, b;
    if (math_num(L, 0, &a) != 0 || math_num(L, 1, &b) != 0) return -1;
    luna_pushfloat(L, pow(a, b));
    return 1;
}

static int m_minmax(LunaState* L, int want_max) {
    int n = lib_nargs(L);
    if (n == 0) return lib_error(L, "expected at least one number");

    double best;
    if (math_num(L, 0, &best) != 0) return -1;
    for (int i = 1; i < n; i++) {
        double x;
        if (math_num(L, i, &x) != 0) return -1;
        if (want_max ? x > best : x < best) best = x;
    }
    luna_pushfloat(L, best);
    return 1;
}

static int m_min(LunaState* L) { return m_minmax(L, 0); }
static int m_max(LunaState* L) { return m_minmax(L, 1); }

static int m_sign(LunaState* L) {
    double x;
    if (math_num(L, 0, &x) != 0) return -1;
    luna_pushinteger(L, x > 0 ? 1 : (x < 0 ? -1 : 0));
    return 1;
}

static int m_rad(LunaState* L) {
    double x;
    if (math_num(L, 0, &x) != 0) return -1;
    luna_pushfloat(L, x * LUNA_M_PI / 180.0);
    return 1;
}

static int m_deg(LunaState* L) {
    double x;
    if (math_num(L, 0, &x) != 0) return -1;
    luna_pushfloat(L, x * 180.0 / LUNA_M_PI);
    return 1;
}

static int m_isnan(LunaState* L) {
    double x;
    if (math_num(L, 0, &x) != 0) return -1;
    luna_pushboolean(L, isnan(x));
    return 1;
}

static int m_isinf(LunaState* L) {
    double x;
    if (math_num(L, 0, &x) != 0) return -1;
    luna_pushboolean(L, isinf(x));
    return 1;
}

static int m_seed(LunaState* L) {
    double x;
    if (math_num(L, 0, &x) != 0) return -1;
    srand((unsigned int)x);
    luna_pushnil(L);
    return 1;
}

static int m_random(LunaState* L) {
    double lo = 0.0, hi = 1.0;
    if (lib_nargs(L) > 0 && math_num(L, 0, &lo) != 0) return -1;
    if (lib_nargs(L) > 1 && math_num(L, 1, &hi) != 0) return -1;
    if (hi < lo) {
        double tmp = lo;
        lo = hi;
        hi = tmp;
    }
    double r = (double)rand() / ((double)RAND_MAX + 1.0);
    luna_pushfloat(L, lo + r * (hi - lo));
    return 1;
}

static void register_functions(LunaState* L, LunaTable* exports) {
    lib_export_float(L, exports, "pi", LUNA_M_PI);
    lib_export_float(L, exports, "e", LUNA_M_E);
    lib_export_float(L, exports, "inf", INFINITY);
    lib_export_float(L, exports, "nan", NAN);

    lib_export_function(L, exports, "abs", m_abs);
    lib_export_function(L, exports, "sqrt", m_sqrt);
    lib_export_function(L, exports, "cbrt", m_cbrt);
    lib_export_function(L, exports, "floor", m_floor);
    lib_export_function(L, exports, "ceil", m_ceil);
    lib_export_function(L, exports, "round", m_round);
    lib_export_function(L, exports, "trunc", m_trunc);
    lib_export_function(L, exports, "sin", m_sin);
    lib_export_function(L, exports, "cos", m_cos);
    lib_export_function(L, exports, "tan", m_tan);
    lib_export_function(L, exports, "asin", m_asin);
    lib_export_function(L, exports, "acos", m_acos);
    lib_export_function(L, exports, "atan", m_atan);
    lib_export_function(L, exports, "atan2", m_atan2);
    lib_export_function(L, exports, "sinh", m_sinh);
    lib_export_function(L, exports, "cosh", m_cosh);
    lib_export_function(L, exports, "tanh", m_tanh);
    lib_export_function(L, exports, "exp", m_exp);
    lib_export_function(L, exports, "log", m_log);
    lib_export_function(L, exports, "log10", m_log10);
    lib_export_function(L, exports, "log2", m_log2);
    lib_export_function(L, exports, "pow", m_pow);
    lib_export_function(L, exports, "min", m_min);
    lib_export_function(L, exports, "max", m_max);
    lib_export_function(L, exports, "sign", m_sign);
    lib_export_function(L, exports, "rad", m_rad);
    lib_export_function(L, exports, "deg", m_deg);
    lib_export_function(L, exports, "isnan", m_isnan);
    lib_export_function(L, exports, "isinf", m_isinf);
    lib_export_function(L, exports, "seed", m_seed);
    lib_export_function(L, exports, "random", m_random);
}

static const LunaModule g_module = { "math_native", register_functions, NULL };

void luna_open_math_lib(LunaState* L) {
    luna_register_module(L, &g_module);
}
