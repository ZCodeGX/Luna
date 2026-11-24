#include "proto.h"
#include <stdlib.h>
#include <string.h>

LunaProto* luna_proto_new(const char* name) {
    LunaProto* p = (LunaProto*)calloc(1, sizeof(LunaProto));
    if (!p) return NULL;
    
    const char* src = name ? name : "<anonymous>";
    size_t len = strlen(src);
    p->name = (char*)malloc(len + 1);
    if (p->name) {
        memcpy(p->name, src, len + 1);
    }
    p->refcount = 1;

    return p;
}

void luna_proto_retain(LunaProto* p) {
    if (p) p->refcount++;
}

static void proto_destroy(LunaProto* p) {
    if (!p) return;

    free(p->name);
    free(p->code);
    free(p->lines);
    
    for (size_t i = 0; i < p->num_constants; i++) {
        if (p->constants[i].type == LUNA_CONST_STRING) {
            free(p->constants[i].as.string.data);
        }
    }
    free(p->constants);
    
    free(p->upvalues);
    
    for (size_t i = 0; i < p->num_protos; i++) {
        proto_destroy(p->protos[i]);
    }
    free(p->protos);

    free(p);
}

void luna_proto_release(LunaProto* p) {
    if (!p) return;
    if (p->refcount > 0 && --p->refcount == 0) {
        proto_destroy(p);
    }
}

void luna_proto_free(LunaProto* p) {
    luna_proto_release(p);
}

void luna_proto_set_current_line(LunaProto* p, int line) {
    if (p) p->current_line = line;
}

void luna_proto_emit_byte(LunaProto* p, uint8_t b) {
    if (p->code_len >= p->code_cap) {
        size_t new_cap = p->code_cap ? p->code_cap * 2 : 256;
        uint8_t* new_code = (uint8_t*)realloc(p->code, new_cap);
        if (!new_code) return;
        p->code = new_code;
        p->code_cap = new_cap;
    }
    p->code[p->code_len++] = b;
}

void luna_proto_emit_word(LunaProto* p, uint16_t w) {
    luna_proto_emit_byte(p, w & 0xFF);
    luna_proto_emit_byte(p, (w >> 8) & 0xFF);
}

void luna_proto_emit_op_NONE(LunaProto* p, LunaOpcode op) {
    luna_proto_emit_byte(p, (uint8_t)op);
    luna_proto_set_last_line(p, p->current_line);
}

void luna_proto_emit_op_A(LunaProto* p, LunaOpcode op, uint8_t a) {
    luna_proto_emit_byte(p, (uint8_t)op);
    luna_proto_emit_byte(p, a);
    luna_proto_set_last_line(p, p->current_line);
}

void luna_proto_emit_op_AB(LunaProto* p, LunaOpcode op, uint8_t a, uint8_t b) {
    luna_proto_emit_byte(p, (uint8_t)op);
    luna_proto_emit_byte(p, a);
    luna_proto_emit_byte(p, b);
    luna_proto_set_last_line(p, p->current_line);
}

void luna_proto_emit_op_ABC(LunaProto* p, LunaOpcode op, uint8_t a, uint8_t b, uint8_t c) {
    luna_proto_emit_byte(p, (uint8_t)op);
    luna_proto_emit_byte(p, a);
    luna_proto_emit_byte(p, b);
    luna_proto_emit_byte(p, c);
    luna_proto_set_last_line(p, p->current_line);
}

void luna_proto_emit_op_ABx(LunaProto* p, LunaOpcode op, uint8_t a, uint16_t bx) {
    luna_proto_emit_byte(p, (uint8_t)op);
    luna_proto_emit_byte(p, a);
    luna_proto_emit_word(p, bx);
    luna_proto_set_last_line(p, p->current_line);
}

void luna_proto_emit_op_AsBx(LunaProto* p, LunaOpcode op, uint8_t a, int16_t sbx) {
    uint16_t bx = (uint16_t)(sbx + 32767);
    luna_proto_emit_op_ABx(p, op, a, bx);
}

void luna_proto_emit_op_sBx(LunaProto* p, LunaOpcode op, int16_t sbx) {
    uint16_t bx = (uint16_t)(sbx + 32767);
    luna_proto_emit_byte(p, (uint8_t)op);
    luna_proto_emit_word(p, bx);
    luna_proto_set_last_line(p, p->current_line);
}

void luna_proto_set_last_line(LunaProto* p, int line) {
    if (p->num_instructions >= p->lines_cap) {
        size_t new_cap = p->lines_cap ? p->lines_cap * 2 : 256;
        int* new_lines = (int*)realloc(p->lines, new_cap * sizeof(int));
        if (!new_lines) return;
        p->lines = new_lines;
        p->lines_cap = new_cap;
    }
    p->lines[p->num_instructions++] = line;
}

static uint16_t add_const(LunaProto* p, LunaConstType type) {
    if (p->num_constants >= p->cap_constants) {
        size_t new_cap = p->cap_constants ? p->cap_constants * 2 : 64;
        LunaConst* new_consts = (LunaConst*)realloc(p->constants, new_cap * sizeof(LunaConst));
        if (!new_consts) return 0;
        p->constants = new_consts;
        p->cap_constants = new_cap;
    }
    uint16_t idx = (uint16_t)p->num_constants;
    p->constants[idx].type = type;
    p->num_constants++;
    return idx;
}

uint16_t luna_proto_add_const_nil(LunaProto* p) {
    return add_const(p, LUNA_CONST_NIL);
}

uint16_t luna_proto_add_const_bool(LunaProto* p, int v) {
    uint16_t idx = add_const(p, LUNA_CONST_BOOL);
    p->constants[idx].as.boolean = v ? 1 : 0;
    return idx;
}

uint16_t luna_proto_add_const_int(LunaProto* p, int64_t v) {
    uint16_t idx = add_const(p, LUNA_CONST_INT);
    p->constants[idx].as.integer = v;
    return idx;
}

uint16_t luna_proto_add_const_float(LunaProto* p, double v) {
    uint16_t idx = add_const(p, LUNA_CONST_FLOAT);
    p->constants[idx].as.floating = v;
    return idx;
}

uint16_t luna_proto_add_const_string(LunaProto* p, const char* s, size_t len) {
    uint16_t idx = add_const(p, LUNA_CONST_STRING);
    p->constants[idx].as.string.data = (char*)malloc(len + 1);
    if (p->constants[idx].as.string.data) {
        memcpy(p->constants[idx].as.string.data, s, len);
        p->constants[idx].as.string.data[len] = 0;
        p->constants[idx].as.string.len = len;
    }
    return idx;
}

uint8_t luna_proto_add_upvalue(LunaProto* p, int in_stack, uint8_t index) {
    if (p->num_upvalues >= p->cap_upvalues) {
        size_t new_cap = p->cap_upvalues ? p->cap_upvalues * 2 : 16;
        LunaUpvalueDesc* new_ups = (LunaUpvalueDesc*)realloc(p->upvalues, new_cap * sizeof(LunaUpvalueDesc));
        if (!new_ups) return 0;
        p->upvalues = new_ups;
        p->cap_upvalues = new_cap;
    }
    uint8_t idx = (uint8_t)p->num_upvalues;
    p->upvalues[idx].in_stack = in_stack ? 1 : 0;
    p->upvalues[idx].index = index;
    p->num_upvalues++;
    return idx;
}

uint16_t luna_proto_add_proto(LunaProto* p, LunaProto* child) {
    if (p->num_protos >= p->cap_protos) {
        size_t new_cap = p->cap_protos ? p->cap_protos * 2 : 8;
        LunaProto** new_protos = (LunaProto**)realloc(p->protos, new_cap * sizeof(LunaProto*));
        if (!new_protos) return 0;
        p->protos = new_protos;
        p->cap_protos = new_cap;
    }
    uint16_t idx = (uint16_t)p->num_protos;
    p->protos[idx] = child;
    p->num_protos++;
    return idx;
}