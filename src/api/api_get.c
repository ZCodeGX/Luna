#include "api_internal.h"

/* ============================================================
 * Consulta de tipo
 * ============================================================ */

LunaType luna_type(LunaState* L, int idx) {
    LunaValue v = api_get(L, idx);
    return v.type;
}

const char* luna_typename(LunaType t) {
    return luna_type_name(t);
}

int luna_isnil(LunaState* L, int idx)      { return api_get(L, idx).type == LUNA_TNIL; }
int luna_isboolean(LunaState* L, int idx)  { return api_get(L, idx).type == LUNA_TBOOLEAN; }
int luna_isinteger(LunaState* L, int idx)  { return api_get(L, idx).type == LUNA_TINTEGER; }
int luna_isfloat(LunaState* L, int idx)    { return api_get(L, idx).type == LUNA_TFLOAT; }
int luna_isstring(LunaState* L, int idx)   { return api_get(L, idx).type == LUNA_TSTRING; }
int luna_islist(LunaState* L, int idx)     { return api_get(L, idx).type == LUNA_TLIST; }
int luna_istable(LunaState* L, int idx)    { return api_get(L, idx).type == LUNA_TTABLE; }
int luna_isfunction(LunaState* L, int idx) { return api_get(L, idx).type == LUNA_TFUNCTION; }
int luna_isuserdata(LunaState* L, int idx) { return api_get(L, idx).type == LUNA_TUSERDATA; }

int luna_isnumber(LunaState* L, int idx) {
    LunaType t = api_get(L, idx).type;
    return (t == LUNA_TINTEGER || t == LUNA_TFLOAT) ? 1 : 0;
}

/* ============================================================
 * Conversión (no destructiva, sin error)
 * ============================================================ */

int luna_toboolean(LunaState* L, int idx) {
    return luna_value_truthy(api_get(L, idx)) ? 1 : 0;
}

int64_t luna_tointeger(LunaState* L, int idx, int* ok) {
    LunaValue v = api_get(L, idx);
    int64_t out;
    int r = luna_value_to_integer(v, &out);
    if (ok) *ok = r;
    return out;
}

double luna_tonumber(LunaState* L, int idx, int* ok) {
    LunaValue v = api_get(L, idx);
    double out;
    int r = luna_value_to_number(v, &out);
    if (ok) *ok = r;
    return out;
}

const char* luna_tostring(LunaState* L, int idx, size_t* len) {
    LunaValue v = api_get(L, idx);
    if (v.type != LUNA_TSTRING) {
        if (len) *len = 0;
        return NULL;
    }
    LunaString* s = luna_val_string(v);
    if (len) *len = s->len;
    return s->data;
}

void* luna_touserdata(LunaState* L, int idx) {
    LunaValue v = api_get(L, idx);
    if (v.type != LUNA_TUSERDATA) return NULL;
    return luna_val_userdata(v)->data;
}

/* ============================================================
 * Check (con error si tipo incorrecto)
 * ============================================================ */

int64_t luna_checkinteger(LunaState* L, int idx) {
    int ok = 0;
    int64_t v = luna_tointeger(L, idx, &ok);
    if (!ok) {
        api_type_error(L, idx, "integer");
    }
    return v;
}

double luna_checknumber(LunaState* L, int idx) {
    int ok = 0;
    double v = luna_tonumber(L, idx, &ok);
    if (!ok) {
        api_type_error(L, idx, "number");
    }
    return v;
}

const char* luna_checkstring(LunaState* L, int idx, size_t* len) {
    const char* s = luna_tostring(L, idx, len);
    if (!s) {
        api_type_error(L, idx, "string");
        return "";
    }
    return s;
}

void* luna_checkuserdata(LunaState* L, int idx) {
    void* p = luna_touserdata(L, idx);
    if (!p) {
        api_type_error(L, idx, "userdata");
    }
    return p;
}

int luna_checkboolean(LunaState* L, int idx) {
    LunaValue v = api_get(L, idx);
    if (v.type != LUNA_TBOOLEAN) {
        api_type_error(L, idx, "boolean");
        return 0;
    }
    return v.as.boolean;
}