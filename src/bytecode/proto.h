#ifndef LUNA_PROTO_H
#define LUNA_PROTO_H

#include "opcodes.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Un LunaProto es la unidad de compilación: representa el bytecode
 * de una función (incluyendo el chunk principal, que es una función anónima).
 *
 * Los protos pueden anidarse: cada proto tiene una lista de protos hijos
 * (las funciones/lambdas declaradas dentro de él).
 */

typedef enum {
    LUNA_CONST_NIL,
    LUNA_CONST_BOOL,
    LUNA_CONST_INT,
    LUNA_CONST_FLOAT,
    LUNA_CONST_STRING
} LunaConstType;

typedef struct {
    LunaConstType type;
    union {
        int         boolean;
        int64_t     integer;
        double      floating;
        struct { char* data; size_t len; } string;
    } as;
} LunaConst;

/* Describe cómo una closure captura una variable. */
typedef struct {
    uint8_t in_stack;   /* 1 si captura local del padre inmediato, 0 si es upvalue del padre */
    uint8_t index;      /* índice del local o del upvalue del padre */
} LunaUpvalueDesc;

typedef struct LunaProto LunaProto;

struct LunaProto {
    char*           name;           /* nombre de la función (o "<main>" para el chunk) */
    uint8_t         num_params;     /* número de parámetros declarados */
    uint8_t         num_locals;     /* máximo de slots locales necesarios */
    uint8_t         stack_size;     /* tamaño máximo de stack que necesita */
    bool            is_vararg;      /* ¿acepta ...? (reservado, no usado en 1.0) */

    /* Bytecode */
    uint8_t*        code;           /* array de bytes (instrucciones de ancho variable) */
    size_t          code_len;
    size_t          code_cap;

    /* Mapa de líneas: para cada instrucción (por índice de instrucción, no byte), su línea fuente */
    int*            lines;          /* array paralelo a "instructions" (ver más abajo) */
    size_t          lines_cap;

    /* Constantes */
    LunaConst*      constants;
    size_t          num_constants;
    size_t          cap_constants;

    /* Upvalues que esta función captura */
    LunaUpvalueDesc* upvalues;
    size_t           num_upvalues;
    size_t           cap_upvalues;

    /* Protos hijos (funciones anidadas) */
    LunaProto**     protos;
    size_t          num_protos;
    size_t          cap_protos;

    /* Contador de instrucciones (para el array de líneas) */
    size_t          num_instructions;

    size_t          refcount;
    int             current_line;
};

/* Crear y destruir */
LunaProto* luna_proto_new(const char* name);
void       luna_proto_retain(LunaProto* p);
void       luna_proto_release(LunaProto* p);
void       luna_proto_free(LunaProto* p);

/* Emisión de bytecode (helpers) */
void luna_proto_set_current_line(LunaProto* p, int line);
void luna_proto_emit_byte(LunaProto* p, uint8_t b);
void luna_proto_emit_word(LunaProto* p, uint16_t w);    /* little-endian */
void luna_proto_emit_op_NONE(LunaProto* p, LunaOpcode op);
void luna_proto_emit_op_A(LunaProto* p, LunaOpcode op, uint8_t a);
void luna_proto_emit_op_AB(LunaProto* p, LunaOpcode op, uint8_t a, uint8_t b);
void luna_proto_emit_op_ABC(LunaProto* p, LunaOpcode op, uint8_t a, uint8_t b, uint8_t c);
void luna_proto_emit_op_ABx(LunaProto* p, LunaOpcode op, uint8_t a, uint16_t bx);
void luna_proto_emit_op_AsBx(LunaProto* p, LunaOpcode op, uint8_t a, int16_t sbx);
void luna_proto_emit_op_sBx(LunaProto* p, LunaOpcode op, int16_t sbx);

/* Registrar línea fuente para la última instrucción emitida */
void luna_proto_set_last_line(LunaProto* p, int line);

/* Gestión de constantes (devuelven índice; añaden si no existen para string/int/float) */
uint16_t luna_proto_add_const_nil(LunaProto* p);
uint16_t luna_proto_add_const_bool(LunaProto* p, int v);
uint16_t luna_proto_add_const_int(LunaProto* p, int64_t v);
uint16_t luna_proto_add_const_float(LunaProto* p, double v);
uint16_t luna_proto_add_const_string(LunaProto* p, const char* s, size_t len);

/* Gestión de upvalues */
uint8_t luna_proto_add_upvalue(LunaProto* p, int in_stack, uint8_t index);

/* Añadir un proto hijo; devuelve su índice */
uint16_t luna_proto_add_proto(LunaProto* p, LunaProto* child);

/* Validación de bytecode cargado desde disco */
int luna_proto_validate(const LunaProto* root, char* error_out, size_t error_size);

#endif /* LUNA_PROTO_H */