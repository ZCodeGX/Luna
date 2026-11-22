#ifndef LUNA_VALUE_H
#define LUNA_VALUE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../memory/gcobject.h"   /* LunaGCObject vive aquí */
#include "../../include/luna/luna_types.h"

/* ---------- Forward declarations ---------- */
typedef struct LunaString   LunaString;
typedef struct LunaTable    LunaTable;
typedef struct LunaList     LunaList;
typedef struct LunaFunction LunaFunction;
typedef struct LunaUserdata LunaUserdata;

/* ---------- Valor tagged ---------- */
struct LunaValue {
    LunaType type;
    union {
        int           boolean;
        int64_t       integer;
        double        floating;
        LunaGCObject* gcobj;
    } as;
};
typedef struct LunaValue LunaValue;

/* ---------- Constructores (primitivos, inline) ---------- */
static inline LunaValue luna_nil(void) {
    LunaValue v; v.type = LUNA_TNIL; v.as.integer = 0; return v;
}
static inline LunaValue luna_bool(int b) {
    LunaValue v; v.type = LUNA_TBOOLEAN; v.as.boolean = b ? 1 : 0; return v;
}
static inline LunaValue luna_int(int64_t i) {
    LunaValue v; v.type = LUNA_TINTEGER; v.as.integer = i; return v;
}
static inline LunaValue luna_float(double f) {
    LunaValue v; v.type = LUNA_TFLOAT; v.as.floating = f; return v;
}

/* Constructores para objetos GC (implementados en value.c) */
LunaValue luna_string_val(LunaString* s);
LunaValue luna_table_val(LunaTable* t);
LunaValue luna_list_val(LunaList* l);
LunaValue luna_function_val(LunaFunction* f);
LunaValue luna_userdata_val(LunaUserdata* u);

/* ---------- Accessors (inline) ---------- */
static inline int           luna_val_bool(LunaValue v)     { return v.as.boolean; }
static inline int64_t       luna_val_int(LunaValue v)      { return v.as.integer; }
static inline double        luna_val_float(LunaValue v)    { return v.as.floating; }
static inline LunaString*   luna_val_string(LunaValue v)   { return (LunaString*)v.as.gcobj; }
static inline LunaTable*    luna_val_table(LunaValue v)    { return (LunaTable*)v.as.gcobj; }
static inline LunaList*     luna_val_list(LunaValue v)     { return (LunaList*)v.as.gcobj; }
static inline LunaFunction* luna_val_function(LunaValue v) { return (LunaFunction*)v.as.gcobj; }
static inline LunaUserdata* luna_val_userdata(LunaValue v) { return (LunaUserdata*)v.as.gcobj; }

/* ---------- Operaciones sobre valores ---------- */
bool        luna_value_equal(LunaValue a, LunaValue b);
static inline bool luna_value_truthy(LunaValue v) {
    switch (v.type) {
        case LUNA_TNIL:     return false;
        case LUNA_TBOOLEAN: return v.as.boolean != 0;
        default:            return true;
    }
}
uint64_t    luna_value_hash(LunaValue v);
const char* luna_type_name(LunaType t);

/* ---------- Conversiones numéricas ---------- */
bool luna_value_to_number(LunaValue v, double* out);
bool luna_value_to_integer(LunaValue v, int64_t* out);

/* ---------- Incluir lstring.h al final ---------- */
/*
 * lstring.h necesita LunaState y LunaGCObject (definidos arriba),
 * y value.h necesita LunaString para los constructores/accessors.
 * Al incluirlo al final rompemos la dependencia circular.
 */
#include "lstring.h"

#endif /* LUNA_VALUE_H */