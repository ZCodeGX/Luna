#include "stdlib_internal.h"
#include "../runtime/value.h"
#include "../runtime/list.h"
#include "../runtime/table.h"
#include "../runtime/lstring.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char*  data;
    size_t len;
    size_t cap;
    int    failed;
} CsvBuf;

typedef struct {
    const char* s;
    size_t      len;
    size_t      pos;
    char        delim;
} CsvParser;

static int csv_buf_reserve(CsvBuf* b, size_t extra) {
    if (b->failed) return -1;
    if (extra > SIZE_MAX - b->len - 1) {
        b->failed = 1;
        return -1;
    }
    size_t need = b->len + extra + 1;
    if (need <= b->cap) return 0;
    size_t cap = b->cap ? b->cap : 128;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) {
            b->failed = 1;
            return -1;
        }
        cap *= 2;
    }
    char* p = (char*)realloc(b->data, cap);
    if (!p) {
        b->failed = 1;
        return -1;
    }
    b->data = p;
    b->cap = cap;
    return 0;
}

static void csv_buf_write(CsvBuf* b, const char* s, size_t n) {
    if (csv_buf_reserve(b, n) != 0) return;
    if (n) memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static void csv_buf_char(CsvBuf* b, char c) {
    csv_buf_write(b, &c, 1);
}

static void csv_buf_text(CsvBuf* b, const char* s) {
    csv_buf_write(b, s, strlen(s));
}

static void csv_buf_reset(CsvBuf* b) {
    b->len = 0;
    b->failed = 0;
    if (b->data) b->data[0] = '\0';
}

static int csv_keep(LunaState* L, LunaValue v) {
    int top = luna_gettop(L);
    lib_push_value(L, v);
    return luna_gettop(L) == top + 1 ? 0 : -1;
}

static int csv_get_delimiter(LunaState* L, int index, char* out) {
    LunaValue v = lib_arg(L, index);
    if (v.type == LUNA_TNIL) return 0;
    if (v.type != LUNA_TSTRING) return lib_error(L, "csv: delimiter must be a string");
    LunaString* s = luna_val_string(v);
    if (luna_string_len(s) != 1) return lib_error(L, "csv: delimiter must be one character");
    *out = luna_string_data(s)[0];
    return 0;
}

static int csv_parse_field(CsvParser* p, CsvBuf* b) {
    csv_buf_reset(b);
    if (p->pos < p->len && p->s[p->pos] == '"') {
        p->pos++;
        int closed = 0;
        while (p->pos < p->len) {
            char c = p->s[p->pos++];
            if (c == '"') {
                if (p->pos < p->len && p->s[p->pos] == '"') {
                    p->pos++;
                    csv_buf_char(b, '"');
                } else {
                    closed = 1;
                    break;
                }
            } else {
                csv_buf_char(b, c);
            }
            if (b->failed) return -1;
        }
        if (!closed) return -1;
        if (p->pos < p->len) {
            char c = p->s[p->pos];
            if (c != p->delim && c != '\n' && c != '\r') return -1;
        }
        return b->failed ? -1 : 0;
    }
    while (p->pos < p->len) {
        char c = p->s[p->pos];
        if (c == p->delim || c == '\n' || c == '\r') break;
        if (c == '"') return -1;
        csv_buf_char(b, c);
        p->pos++;
        if (b->failed) return -1;
    }
    return 0;
}

static void csv_consume_newline(CsvParser* p) {
    if (p->pos < p->len && p->s[p->pos] == '\r') {
        p->pos++;
        if (p->pos < p->len && p->s[p->pos] == '\n') p->pos++;
    } else if (p->pos < p->len && p->s[p->pos] == '\n') {
        p->pos++;
    }
}

static int csv_parse_text(LunaState* L, const char* text, size_t len, char delim) {
    int base = luna_gettop(L);
    CsvParser p;
    p.s = text ? text : "";
    p.len = len;
    p.pos = 0;
    p.delim = delim;
    LunaList* rows = luna_list_new(L);
    if (!rows) {
        luna_settop(L, base);
        return -1;
    }
    LunaValue rowsv = luna_list_val(rows);
    if (csv_keep(L, rowsv) != 0) {
        luna_settop(L, base);
        return -1;
    }
    CsvBuf field = { NULL, 0, 0, 0 };
    while (p.pos < p.len) {
        LunaList* row = luna_list_new(L);
        if (!row) goto fail;
        LunaValue rowv = luna_list_val(row);
        if (csv_keep(L, rowv) != 0) goto fail;
        for (;;) {
            if (csv_parse_field(&p, &field) != 0) goto fail;
            LunaString* s = luna_string_new(L, field.data ? field.data : "", field.len);
            if (!s) goto fail;
            LunaValue fv = luna_string_val(s);
            if (csv_keep(L, fv) != 0) goto fail;
            luna_list_push_value(L, row, fv);
            if (p.pos >= p.len) break;
            if (p.s[p.pos] == p.delim) {
                p.pos++;
                continue;
            }
            if (p.s[p.pos] == '\n' || p.s[p.pos] == '\r') {
                csv_consume_newline(&p);
                break;
            }
            goto fail;
        }
        luna_list_push_value(L, rows, rowv);
    }
    free(field.data);
    luna_settop(L, base);
    lib_push_value(L, rowsv);
    return luna_gettop(L) == base + 1 ? 0 : -1;
fail:
    free(field.data);
    luna_settop(L, base);
    return -1;
}

static int csv_write_value(CsvBuf* b, LunaValue v) {
    char tmp[64];
    int n;
    switch (v.type) {
        case LUNA_TNIL:
            break;
        case LUNA_TBOOLEAN:
            csv_buf_text(b, luna_val_bool(v) ? "true" : "false");
            break;
        case LUNA_TINTEGER:
            n = snprintf(tmp, sizeof(tmp), "%lld", (long long)luna_val_int(v));
            if (n > 0) csv_buf_write(b, tmp, (size_t)n);
            break;
        case LUNA_TFLOAT:
            n = snprintf(tmp, sizeof(tmp), "%.14g", luna_val_float(v));
            if (n > 0) csv_buf_write(b, tmp, (size_t)n);
            break;
        case LUNA_TSTRING: {
            LunaString* s = luna_val_string(v);
            csv_buf_write(b, luna_string_data(s), luna_string_len(s));
            break;
        }
        default:
            csv_buf_text(b, luna_type_name(v.type));
            break;
    }
    return b->failed ? -1 : 0;
}

static void csv_write_escaped(CsvBuf* out, const char* data, size_t len, char delim) {
    size_t i;
    int quote = 0;
    for (i = 0; i < len; i++) {
        if (data[i] == delim || data[i] == '"' || data[i] == '\r' || data[i] == '\n') {
            quote = 1;
            break;
        }
    }
    if (!quote) {
        csv_buf_write(out, data, len);
        return;
    }
    csv_buf_char(out, '"');
    for (i = 0; i < len; i++) {
        if (data[i] == '"') csv_buf_char(out, '"');
        csv_buf_char(out, data[i]);
    }
    csv_buf_char(out, '"');
}

static int csv_append_value(CsvBuf* out, LunaValue v, char delim) {
    CsvBuf tmp = { NULL, 0, 0, 0 };
    if (csv_write_value(&tmp, v) != 0) {
        free(tmp.data);
        return -1;
    }
    csv_write_escaped(out, tmp.data ? tmp.data : "", tmp.len, delim);
    free(tmp.data);
    return out->failed ? -1 : 0;
}

static int csv_encode_row(CsvBuf* out, LunaValue row, char delim) {
    if (row.type == LUNA_TLIST) {
        LunaList* list = luna_val_list(row);
        size_t n = luna_list_len(list);
        for (size_t i = 0; i < n; i++) {
            LunaValue v = luna_nil();
            if (!luna_list_get(list, i, &v)) return -1;
            if (i) csv_buf_char(out, delim);
            if (out->failed || csv_append_value(out, v, delim) != 0) return -1;
        }
        return out->failed ? -1 : 0;
    }
    if (row.type == LUNA_TTABLE) {
        LunaTable* table = luna_val_table(row);
        uint32_t written = 0;
        for (uint32_t i = 0; i < table->size; i++) {
            LunaTableNode* node = &table->nodes[i];
            if (node->key.type == LUNA_TNIL) continue;
            if (written) csv_buf_char(out, delim);
            if (out->failed || csv_append_value(out, node->value, delim) != 0) return -1;
            written++;
        }
        return out->failed ? -1 : 0;
    }
    return -1;
}

static int csv_parse_fn(LunaState* L) {
    LunaValue v = lib_arg(L, 0);
    if (v.type != LUNA_TSTRING) return lib_error(L, "csv: expected string");
    char delim = ',';
    if (csv_get_delimiter(L, 1, &delim) != 0) return -1;
    LunaString* s = luna_val_string(v);
    if (csv_parse_text(L, luna_string_data(s), luna_string_len(s), delim) != 0) {
        return lib_error(L, "csv: invalid input");
    }
    return 1;
}

static int csv_encode_fn(LunaState* L) {
    LunaValue rows = lib_arg(L, 0);
    if (rows.type != LUNA_TLIST) return lib_error(L, "csv: expected list of rows");
    char delim = ',';
    if (csv_get_delimiter(L, 1, &delim) != 0) return -1;
    LunaList* list = luna_val_list(rows);
    size_t n = luna_list_len(list);
    CsvBuf out = { NULL, 0, 0, 0 };
    for (size_t i = 0; i < n; i++) {
        LunaValue row = luna_nil();
        if (!luna_list_get(list, i, &row)) goto fail;
        if (csv_encode_row(&out, row, delim) != 0) goto fail;
        csv_buf_char(&out, '\n');
    }
    if (out.failed) goto fail;
    luna_pushlstring(L, out.data ? out.data : "", out.len);
    free(out.data);
    return 1;
fail:
    free(out.data);
    return lib_error(L, "csv: cannot encode rows");
}

static void register_csv(LunaState* L, LunaTable* exports) {
    lib_export_function(L, exports, "parse", csv_parse_fn);
    lib_export_function(L, exports, "decode", csv_parse_fn);
    lib_export_function(L, exports, "encode", csv_encode_fn);
    lib_export_function(L, exports, "stringify", csv_encode_fn);
}

static const LunaModule g_csv_module = { "csv_native", register_csv, NULL };

void luna_open_csv_lib(LunaState* L) {
    luna_register_module(L, &g_csv_module);
}
