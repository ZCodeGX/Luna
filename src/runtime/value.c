#include "value.h"
#include "lstring.h"          /* <-- cambia de "string.h" a "lstring.h" */
#include "runtime_internal.h"
#include <string.h>
#include <math.h>

LunaValue luna_string_val(LunaString* s) {
    LunaValue v; v.type = LUNA_TSTRING; v.as.gcobj = (LunaGCObject*)s; return v;
}
LunaValue luna_table_val(LunaTable* t) {
    LunaValue v; v.type = LUNA_TTABLE; v.as.gcobj = (LunaGCObject*)t; return v;
}
LunaValue luna_list_val(LunaList* l) {
    LunaValue v; v.type = LUNA_TLIST; v.as.gcobj = (LunaGCObject*)l; return v;
}
LunaValue luna_function_val(LunaFunction* f) {
    LunaValue v; v.type = LUNA_TFUNCTION; v.as.gcobj = (LunaGCObject*)f; return v;
}
LunaValue luna_userdata_val(LunaUserdata* u) {
    LunaValue v; v.type = LUNA_TUSERDATA; v.as.gcobj = (LunaGCObject*)u; return v;
}

bool luna_value_equal(LunaValue a, LunaValue b) {
    if (a.type != b.type) {
        /* int/float son comparables */
        if (a.type == LUNA_TINTEGER && b.type == LUNA_TFLOAT)
            return (double)a.as.integer == b.as.floating;
        if (a.type == LUNA_TFLOAT && b.type == LUNA_TINTEGER)
            return a.as.floating == (double)b.as.integer;
        return false;
    }
    switch (a.type) {
        case LUNA_TNIL:      return true;
        case LUNA_TBOOLEAN:  return a.as.boolean == b.as.boolean;
        case LUNA_TINTEGER:  return a.as.integer == b.as.integer;
        case LUNA_TFLOAT:    return a.as.floating == b.as.floating;
        case LUNA_TSTRING:   return luna_string_equal(luna_val_string(a), luna_val_string(b));
        default:             return a.as.gcobj == b.as.gcobj;  /* identidad */
    }
}

/* FNV-1a 64-bit */
static uint64_t hash_bytes(const void* data, size_t len) {
    const uint8_t* p = (const uint8_t*)data;
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static uint64_t hash_number(double d) {
    if (d == 0.0) d = 0.0;
    return hash_bytes(&d, sizeof(d));
}

uint64_t luna_value_hash(LunaValue v) {
    switch (v.type) {
        case LUNA_TNIL:     return 0;
        case LUNA_TBOOLEAN: return v.as.boolean ? 1 : 0;
        case LUNA_TINTEGER: return hash_number((double)v.as.integer);
        case LUNA_TFLOAT:   return hash_number(v.as.floating);
        case LUNA_TSTRING:  return luna_string_hash(luna_val_string(v));
        default:            return hash_bytes(&v.as.gcobj, sizeof(v.as.gcobj));
    }
}

const char* luna_type_name(LunaType t) {
    switch (t) {
        case LUNA_TNIL:      return "nil";
        case LUNA_TBOOLEAN:  return "boolean";
        case LUNA_TINTEGER:  return "integer";
        case LUNA_TFLOAT:    return "float";
        case LUNA_TSTRING:   return "string";
        case LUNA_TLIST:     return "list";
        case LUNA_TTABLE:    return "table";
        case LUNA_TFUNCTION: return "function";
        case LUNA_TUSERDATA: return "userdata";
    }
    return "?";
}

bool luna_value_to_number(LunaValue v, double* out) {
    switch (v.type) {
        case LUNA_TINTEGER: *out = (double)v.as.integer; return true;
        case LUNA_TFLOAT:   *out = v.as.floating; return true;
        case LUNA_TSTRING:  return false;  /* TODO: parse */
        default: return false;
    }
}

bool luna_value_to_integer(LunaValue v, int64_t* out) {
    switch (v.type) {
        case LUNA_TINTEGER: *out = v.as.integer; return true;
        case LUNA_TFLOAT: {
            double d = v.as.floating;
            int64_t i = (int64_t)d;
            if ((double)i == d) { *out = i; return true; }
            return false;
        }
        default: return false;
    }
}