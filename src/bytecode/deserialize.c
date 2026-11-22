#include "proto.h"
#include "version.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* ---------- helpers de lectura (little-endian) ---------- */

typedef struct {
    FILE* f;
    int   error;
    char  error_msg[256];
} Reader;

static int read_bytes(Reader* R, void* out, size_t n) {
    if (R->error) return -1;
    if (fread(out, 1, n, R->f) != n) {
        R->error = 1;
        snprintf(R->error_msg, sizeof(R->error_msg), "unexpected end of file");
        return -1;
    }
    return 0;
}

static uint8_t read_u8(Reader* R) {
    uint8_t v = 0;
    read_bytes(R, &v, 1);
    return v;
}

static uint16_t read_u16(Reader* R) {
    uint8_t buf[2];
    if (read_bytes(R, buf, 2) != 0) return 0;
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

static uint32_t read_u32(Reader* R) {
    uint8_t buf[4];
    if (read_bytes(R, buf, 4) != 0) return 0;
    return (uint32_t)buf[0]
         | ((uint32_t)buf[1] << 8)
         | ((uint32_t)buf[2] << 16)
         | ((uint32_t)buf[3] << 24);
}

static int32_t read_i32(Reader* R) {
    return (int32_t)read_u32(R);
}

static int64_t read_i64(Reader* R) {
    uint8_t buf[8];
    if (read_bytes(R, buf, 8) != 0) return 0;
    uint64_t uv = 0;
    for (int i = 7; i >= 0; i--) {
        uv = (uv << 8) | buf[i];
    }
    return (int64_t)uv;
}

static double read_double(Reader* R) {
    int64_t iv = read_i64(R);
    uint64_t uv = (uint64_t)iv;
    double v;
    memcpy(&v, &uv, sizeof(v));
    return v;
}

static char* read_string(Reader* R, size_t* out_len) {
    uint32_t len = read_u32(R);
    if (R->error) return NULL;
    if (len > 10 * 1024 * 1024) {  /* límite de seguridad: 10 MB */
        R->error = 1;
        snprintf(R->error_msg, sizeof(R->error_msg), "string too long (%u bytes)", len);
        return NULL;
    }
    char* s = (char*)malloc(len + 1);
    if (!s) {
        R->error = 1;
        snprintf(R->error_msg, sizeof(R->error_msg), "out of memory");
        return NULL;
    }
    if (len > 0 && read_bytes(R, s, len) != 0) {
        free(s);
        return NULL;
    }
    s[len] = 0;
    if (out_len) *out_len = len;
    return s;
}

/* ---------- deserialización de constantes ---------- */

static int read_const(Reader* R, LunaConst* c) {
    c->type = (LunaConstType)read_u8(R);
    if (R->error) return -1;

    switch (c->type) {
        case LUNA_CONST_NIL:
            return 0;
        case LUNA_CONST_BOOL:
            c->as.boolean = read_u8(R) ? 1 : 0;
            return 0;
        case LUNA_CONST_INT:
            c->as.integer = read_i64(R);
            return 0;
        case LUNA_CONST_FLOAT:
            c->as.floating = read_double(R);
            return 0;
        case LUNA_CONST_STRING: {
            size_t len = 0;
            c->as.string.data = read_string(R, &len);
            if (!c->as.string.data) return -1;
            c->as.string.len = len;
            return 0;
        }
        default:
            R->error = 1;
            snprintf(R->error_msg, sizeof(R->error_msg), "unknown constant type %d", c->type);
            return -1;
    }
}

/* ---------- deserialización de proto (recursiva) ---------- */

static LunaProto* read_proto(Reader* R);

static LunaProto* read_proto(Reader* R) {
    if (R->error) return NULL;

    LunaProto* p = (LunaProto*)calloc(1, sizeof(LunaProto));
    if (!p) {
        R->error = 1;
        snprintf(R->error_msg, sizeof(R->error_msg), "out of memory");
        return NULL;
    }

    /* name */
    size_t name_len = 0;
    p->name = read_string(R, &name_len);
    if (!p->name && R->error) goto fail;

    /* metadata */
    p->num_params = read_u8(R);
    p->num_locals = read_u8(R);
    p->stack_size = read_u8(R);
    p->is_vararg  = read_u8(R) ? true : false;

    /* code */
    uint32_t code_len = read_u32(R);
    if (code_len > 0) {
        p->code = (uint8_t*)malloc(code_len);
        if (!p->code) { R->error = 1; goto fail; }
        if (read_bytes(R, p->code, code_len) != 0) goto fail;
        p->code_len = code_len;
        p->code_cap = code_len;
    }

    /* lines */
    uint32_t num_inst = read_u32(R);
    if (num_inst > 0) {
        p->lines = (int*)malloc(num_inst * sizeof(int));
        if (!p->lines) { R->error = 1; goto fail; }
        for (uint32_t i = 0; i < num_inst; i++) {
            p->lines[i] = read_i32(R);
        }
        p->num_instructions = num_inst;
        p->lines_cap = num_inst;
    }

    /* constants */
    uint16_t num_consts = read_u16(R);
    if (num_consts > 0) {
        p->constants = (LunaConst*)calloc(num_consts, sizeof(LunaConst));
        if (!p->constants) { R->error = 1; goto fail; }
        p->num_constants = num_consts;
        p->cap_constants = num_consts;
        for (uint16_t i = 0; i < num_consts; i++) {
            if (read_const(R, &p->constants[i]) != 0) goto fail;
        }
    }

    /* upvalues */
    uint16_t num_ups = read_u16(R);
    if (num_ups > 0) {
        p->upvalues = (LunaUpvalueDesc*)calloc(num_ups, sizeof(LunaUpvalueDesc));
        if (!p->upvalues) { R->error = 1; goto fail; }
        p->num_upvalues = num_ups;
        p->cap_upvalues = num_ups;
        for (uint16_t i = 0; i < num_ups; i++) {
            p->upvalues[i].in_stack = read_u8(R);
            p->upvalues[i].index    = read_u8(R);
        }
    }

    /* protos hijos */
    uint16_t num_protos = read_u16(R);
    if (num_protos > 0) {
        p->protos = (LunaProto**)calloc(num_protos, sizeof(LunaProto*));
        if (!p->protos) { R->error = 1; goto fail; }
        p->num_protos = num_protos;
        p->cap_protos = num_protos;
        for (uint16_t i = 0; i < num_protos; i++) {
            p->protos[i] = read_proto(R);
            if (!p->protos[i]) goto fail;
        }
    }

    return p;

fail:
    luna_proto_free(p);
    return NULL;
}

/* ---------- API pública ---------- */

LunaProto* luna_proto_load(const char* path, char* error_out, size_t error_size) {
    if (!path) return NULL;

    FILE* f = fopen(path, "rb");
    if (!f) {
        if (error_out) snprintf(error_out, error_size, "cannot open file");
        return NULL;
    }

    Reader R = { .f = f, .error = 0, .error_msg = {0} };

    /* Magic */
    uint8_t magic[4];
    if (read_bytes(&R, magic, 4) != 0) goto fail;
    if (magic[0] != LUNA_BYTECODE_MAGIC_0 ||
        magic[1] != LUNA_BYTECODE_MAGIC_1 ||
        magic[2] != LUNA_BYTECODE_MAGIC_2 ||
        magic[3] != LUNA_BYTECODE_MAGIC_3) {
        snprintf(R.error_msg, sizeof(R.error_msg), "bad magic number");
        goto fail;
    }

    /* Version */
    uint8_t major = read_u8(&R);
    uint8_t minor = read_u8(&R);
    if (major != LUNA_BYTECODE_VERSION_MAJOR) {
        snprintf(R.error_msg, sizeof(R.error_msg),
                 "incompatible version %d.%d (expected %d.x)",
                 major, minor, LUNA_BYTECODE_VERSION_MAJOR);
        goto fail;
    }

    LunaProto* root = read_proto(&R);
    fclose(f);

    if (!root) {
        if (error_out) snprintf(error_out, error_size, "%s", R.error_msg);
        return NULL;
    }

    return root;

fail:
    fclose(f);
    if (error_out) snprintf(error_out, error_size, "%s", R.error_msg);
    return NULL;
}