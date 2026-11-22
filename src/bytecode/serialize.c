#include "proto.h"
#include "version.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* ---------- helpers de escritura (little-endian) ---------- */

static int write_bytes(FILE* f, const void* data, size_t n) {
    return fwrite(data, 1, n, f) == n ? 0 : -1;
}

static int write_u8(FILE* f, uint8_t v) {
    return write_bytes(f, &v, 1);
}

static int write_u16(FILE* f, uint16_t v) {
    uint8_t buf[2] = { (uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF) };
    return write_bytes(f, buf, 2);
}

static int write_u32(FILE* f, uint32_t v) {
    uint8_t buf[4] = {
        (uint8_t)(v & 0xFF),
        (uint8_t)((v >> 8) & 0xFF),
        (uint8_t)((v >> 16) & 0xFF),
        (uint8_t)((v >> 24) & 0xFF)
    };
    return write_bytes(f, buf, 4);
}

static int write_i32(FILE* f, int32_t v) {
    return write_u32(f, (uint32_t)v);
}

static int write_i64(FILE* f, int64_t v) {
    uint64_t uv = (uint64_t)v;
    uint8_t buf[8];
    for (int i = 0; i < 8; i++) {
        buf[i] = (uint8_t)(uv & 0xFF);
        uv >>= 8;
    }
    return write_bytes(f, buf, 8);
}

static int write_double(FILE* f, double v) {
    uint64_t uv;
    memcpy(&uv, &v, sizeof(uv));
    return write_i64(f, (int64_t)uv);
}

static int write_string(FILE* f, const char* s, size_t len) {
    if (write_u32(f, (uint32_t)len) != 0) return -1;
    if (len > 0 && write_bytes(f, s, len) != 0) return -1;
    return 0;
}

/* ---------- serialización de constantes ---------- */

static int write_const(FILE* f, const LunaConst* c) {
    if (write_u8(f, (uint8_t)c->type) != 0) return -1;
    switch (c->type) {
        case LUNA_CONST_NIL:
            return 0;
        case LUNA_CONST_BOOL:
            return write_u8(f, c->as.boolean ? 1 : 0);
        case LUNA_CONST_INT:
            return write_i64(f, c->as.integer);
        case LUNA_CONST_FLOAT:
            return write_double(f, c->as.floating);
        case LUNA_CONST_STRING:
            return write_string(f, c->as.string.data, c->as.string.len);
    }
    return -1;
}

/* ---------- serialización de proto (recursiva) ---------- */

static int write_proto(FILE* f, const LunaProto* p) {
    /* name */
    size_t name_len = p->name ? strlen(p->name) : 0;
    if (write_string(f, p->name ? p->name : "", name_len) != 0) return -1;

    /* metadata */
    if (write_u8(f, p->num_params) != 0) return -1;
    if (write_u8(f, p->num_locals) != 0) return -1;
    if (write_u8(f, p->stack_size) != 0) return -1;
    if (write_u8(f, p->is_vararg ? 1 : 0) != 0) return -1;

    /* code */
    if (write_u32(f, (uint32_t)p->code_len) != 0) return -1;
    if (p->code_len > 0 && write_bytes(f, p->code, p->code_len) != 0) return -1;

    /* lines */
    if (write_u32(f, (uint32_t)p->num_instructions) != 0) return -1;
    for (size_t i = 0; i < p->num_instructions; i++) {
        if (write_i32(f, p->lines[i]) != 0) return -1;
    }

    /* constants */
    if (write_u16(f, (uint16_t)p->num_constants) != 0) return -1;
    for (size_t i = 0; i < p->num_constants; i++) {
        if (write_const(f, &p->constants[i]) != 0) return -1;
    }

    /* upvalues */
    if (write_u16(f, (uint16_t)p->num_upvalues) != 0) return -1;
    for (size_t i = 0; i < p->num_upvalues; i++) {
        if (write_u8(f, p->upvalues[i].in_stack) != 0) return -1;
        if (write_u8(f, p->upvalues[i].index) != 0) return -1;
    }

    /* protos hijos */
    if (write_u16(f, (uint16_t)p->num_protos) != 0) return -1;
    for (size_t i = 0; i < p->num_protos; i++) {
        if (write_proto(f, p->protos[i]) != 0) return -1;
    }

    return 0;
}

/* ---------- API pública ---------- */

int luna_proto_save(const LunaProto* root, const char* path) {
    if (!root || !path) return -1;

    FILE* f = fopen(path, "wb");
    if (!f) return -1;

    /* Header */
    uint8_t magic[4] = {
        LUNA_BYTECODE_MAGIC_0,
        LUNA_BYTECODE_MAGIC_1,
        LUNA_BYTECODE_MAGIC_2,
        LUNA_BYTECODE_MAGIC_3
    };
    if (write_bytes(f, magic, 4) != 0) goto fail;
    if (write_u8(f, LUNA_BYTECODE_VERSION_MAJOR) != 0) goto fail;
    if (write_u8(f, LUNA_BYTECODE_VERSION_MINOR) != 0) goto fail;

    /* Proto raíz */
    if (write_proto(f, root) != 0) goto fail;

    fclose(f);
    return 0;

fail:
    fclose(f);
    remove(path);
    return -1;
}