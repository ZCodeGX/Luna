#ifndef LUNA_FUNCTION_H
#define LUNA_FUNCTION_H

#include "runtime_internal.h"
#include "../bytecode/proto.h"

typedef struct LunaUpvalue {
    LunaValue* location;   /* puntero al slot del stack o al valor cerrado */
    LunaValue  closed;     /* valor cuando el upvalue está cerrado */
    struct LunaUpvalue* next;  /* lista de upvalues abiertos */
    int refcount;          /* referencias: lista abierta + closures */
} LunaUpvalue;

struct LunaFunction {
    LunaGCObject  header;
    LunaProto*    proto;       /* NULL para funciones C */
    LunaUpvalue** upvalues;
    size_t        num_upvalues;
    
    /* Para funciones C */
    int (*c_function)(struct LunaState* L);
};

/* Upvalues: propiedad compartida entre la lista abierta y los closures */
LunaUpvalue* luna_upvalue_new(LunaState* L, LunaValue* location);
void luna_upvalue_retain(LunaUpvalue* uv);
void luna_upvalue_release(LunaState* L, LunaUpvalue* uv);

/* Crear función desde proto */
LunaFunction* luna_function_new(LunaState* L, LunaProto* proto);

/* Crear función C */
LunaFunction* luna_function_new_c(LunaState* L, int (*fn)(struct LunaState* L));

#endif /* LUNA_FUNCTION_H */