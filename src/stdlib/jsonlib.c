#include "stdlib_internal.h"
#include "../runtime/value.h"
#include "../runtime/list.h"
#include "../runtime/table.h"
#include "../runtime/lstring.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JSON_MAX_DEPTH 128
#define JSON_MAX_INDENT 16

typedef struct {
    char*  data;
    size_t len;
    size_t cap;
    int    failed;
} JsonBuf;

typedef struct {
    JsonBuf* buf;
    int      pretty;
    int      indent;
} JsonWriter;

typedef struct {
    const char* s;
    size_t      len;
    size_t      pos;
} JsonParser;

static int json_buf_reserve(JsonBuf* b, size_t extra) {
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

static void json_buf_write(JsonBuf* b, const char* s, size_t n) {
    if (json_buf_reserve(b, n) != 0) return;
    if (n) memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static void json_buf_char(JsonBuf* b, char c) {
    json_buf_write(b, &c, 1);
}

static void json_buf_text(JsonBuf* b, const char* s) {
    json_buf_write(b, s, strlen(s));
}

static void json_buf_int(JsonBuf* b, int64_t n) {
    char tmp[32];
    int k = snprintf(tmp, sizeof(tmp), "%lld", (long long)n);
    if (k > 0) json_buf_write(b, tmp, (size_t)k);
}

static void json_buf_float(JsonBuf* b, double d) {
    char tmp[64];
    int k = snprintf(tmp, sizeof(tmp), "%.17g", d);
    if (k > 0) json_buf_write(b, tmp, (size_t)k);
}

static void json_buf_string(JsonBuf* b, const char* s, size_t n) {
    static const char hex[] = "0123456789abcdef";
    json_buf_char(b, '"');
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '"':  json_buf_text(b, "\\\""); break;
            case '\\': json_buf_text(b, "\\\\"); break;
            case '\b': json_buf_text(b, "\\b"); break;
            case '\f': json_buf_text(b, "\\f"); break;
            case '\n': json_buf_text(b, "\\n"); break;
            case '\r': json_buf_text(b, "\\r"); break;
            case '\t': json_buf_text(b, "\\t"); break;
            default:
                if (c < 0x20) {
                    char esc[6];
                    esc[0] = '\\';
                    esc[1] = 'u';
                    esc[2] = '0';
                    esc[3] = '0';
                    esc[4] = hex[(c >> 4) & 0x0f];
                    esc[5] = hex[c & 0x0f];
                    json_buf_write(b, esc, sizeof(esc));
                } else {
                    json_buf_char(b, (char)c);
                }
                break;
        }
    }
    json_buf_char(b, '"');
}

static void json_indent(JsonWriter* w, int depth) {
    if (!w->pretty) return;
    json_buf_char(w->buf, '\n');
    int spaces = depth * w->indent;
    for (int i = 0; i < spaces; i++) json_buf_char(w->buf, ' ');
}

static int json_write_value(JsonWriter* w, LunaValue v, int depth);

static int json_write_list(JsonWriter* w, LunaList* list, int depth) {
    size_t n = luna_list_len(list);
    if (n == 0) {
        json_buf_text(w->buf, "[]");
        return 0;
    }
    json_buf_char(w->buf, '[');
    for (size_t i = 0; i < n; i++) {
        if (i) json_buf_char(w->buf, ',');
        json_indent(w, depth);
        LunaValue item = luna_nil();
        if (!luna_list_get(list, i, &item)) return -1;
        if (json_write_value(w, item, depth) != 0) return -1;
    }
    json_indent(w, depth - 1);
    json_buf_char(w->buf, ']');
    return 0;
}

static int json_write_table(JsonWriter* w, LunaTable* table, int depth) {
    uint32_t entries = 0;
    for (uint32_t i = 0; i < table->size; i++) {
        if (table->nodes[i].key.type == LUNA_TSTRING) entries++;
    }
    if (entries == 0) {
        json_buf_text(w->buf, "{}");
        return 0;
    }
    json_buf_char(w->buf, '{');
    uint32_t written = 0;
    for (uint32_t i = 0; i < table->size; i++) {
        LunaTableNode* node = &table->nodes[i];
        if (node->key.type != LUNA_TSTRING) continue;
        if (written) json_buf_char(w->buf, ',');
        json_indent(w, depth);
        LunaString* key = luna_val_string(node->key);
        json_buf_string(w->buf, luna_string_data(key), luna_string_len(key));
        json_buf_char(w->buf, ':');
        if (w->pretty) json_buf_char(w->buf, ' ');
        if (json_write_value(w, node->value, depth) != 0) return -1;
        written++;
    }
    json_indent(w, depth - 1);
    json_buf_char(w->buf, '}');
    return 0;
}

static int json_write_value(JsonWriter* w, LunaValue v, int depth) {
    if (depth > JSON_MAX_DEPTH || w->buf->failed) return -1;
    switch (v.type) {
        case LUNA_TNIL:
            json_buf_text(w->buf, "null");
            break;
        case LUNA_TBOOLEAN:
            json_buf_text(w->buf, luna_val_bool(v) ? "true" : "false");
            break;
        case LUNA_TINTEGER:
            json_buf_int(w->buf, luna_val_int(v));
            break;
        case LUNA_TFLOAT: {
            double d = luna_val_float(v);
            if (isfinite(d)) json_buf_float(w->buf, d);
            else json_buf_text(w->buf, "null");
            break;
        }
        case LUNA_TSTRING: {
            LunaString* s = luna_val_string(v);
            json_buf_string(w->buf, luna_string_data(s), luna_string_len(s));
            break;
        }
        case LUNA_TLIST:
            if (json_write_list(w, luna_val_list(v), depth + 1) != 0) return -1;
            break;
        case LUNA_TTABLE:
            if (json_write_table(w, luna_val_table(v), depth + 1) != 0) return -1;
            break;
        default:
            json_buf_text(w->buf, "null");
            break;
    }
    return w->buf->failed ? -1 : 0;
}

static void json_skip_ws(JsonParser* p) {
    while (p->pos < p->len) {
        char c = p->s[p->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') p->pos++;
        else break;
    }
}

static int json_keep(LunaState* L, LunaValue v) {
    int top = luna_gettop(L);
    lib_push_value(L, v);
    return luna_gettop(L) == top + 1 ? 0 : -1;
}

static void json_utf8(JsonBuf* b, unsigned cp) {
    if (cp <= 0x7f) {
        json_buf_char(b, (char)cp);
    } else if (cp <= 0x7ff) {
        json_buf_char(b, (char)(0xc0 | (cp >> 6)));
        json_buf_char(b, (char)(0x80 | (cp & 0x3f)));
    } else if (cp <= 0xffff) {
        json_buf_char(b, (char)(0xe0 | (cp >> 12)));
        json_buf_char(b, (char)(0x80 | ((cp >> 6) & 0x3f)));
        json_buf_char(b, (char)(0x80 | (cp & 0x3f)));
    } else {
        json_buf_char(b, (char)(0xf0 | (cp >> 18)));
        json_buf_char(b, (char)(0x80 | ((cp >> 12) & 0x3f)));
        json_buf_char(b, (char)(0x80 | ((cp >> 6) & 0x3f)));
        json_buf_char(b, (char)(0x80 | (cp & 0x3f)));
    }
}

static int json_hex4(JsonParser* p, unsigned* out) {
    if (p->pos + 4 > p->len) return -1;
    unsigned v = 0;
    for (int i = 0; i < 4; i++) {
        unsigned char c = (unsigned char)p->s[p->pos++];
        unsigned d;
        if (c >= '0' && c <= '9') d = (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') d = (unsigned)(c - 'a') + 10;
        else if (c >= 'A' && c <= 'F') d = (unsigned)(c - 'A') + 10;
        else return -1;
        v = (v << 4) | d;
    }
    *out = v;
    return 0;
}

static int json_parse_value(LunaState* L, JsonParser* p, LunaValue* out, int depth);

static int json_parse_string(LunaState* L, JsonParser* p, LunaValue* out) {
    if (p->pos >= p->len || p->s[p->pos] != '"') return -1;
    p->pos++;
    JsonBuf b;
    b.data = NULL;
    b.len = 0;
    b.cap = 0;
    b.failed = 0;
    while (p->pos < p->len) {
        unsigned char c = (unsigned char)p->s[p->pos++];
        if (c == '"') {
            LunaString* s = luna_string_new(L, b.data ? b.data : "", b.len);
            free(b.data);
            if (!s) return -1;
            LunaValue v = luna_string_val(s);
            if (json_keep(L, v) != 0) return -1;
            *out = v;
            return 0;
        }
        if (c == '\\') {
            if (p->pos >= p->len) goto fail;
            char e = p->s[p->pos++];
            if (e == 'u') {
                unsigned cp = 0;
                if (json_hex4(p, &cp) != 0) goto fail;
                if (cp >= 0xd800 && cp <= 0xdbff) {
                    unsigned lo = 0;
                    if (p->pos + 6 <= p->len && p->s[p->pos] == '\\' && p->s[p->pos + 1] == 'u') {
                        p->pos += 2;
                        if (json_hex4(p, &lo) != 0 || lo < 0xdc00 || lo > 0xdfff) {
                            cp = 0xfffd;
                        } else {
                            cp = 0x10000 + ((cp - 0xd800) << 10) + (lo - 0xdc00);
                        }
                    } else {
                        cp = 0xfffd;
                    }
                } else if (cp >= 0xdc00 && cp <= 0xdfff) {
                    cp = 0xfffd;
                }
                json_utf8(&b, cp);
            } else {
                char decoded;
                switch (e) {
                    case '"': decoded = '"'; break;
                    case '\\': decoded = '\\'; break;
                    case '/': decoded = '/'; break;
                    case 'b': decoded = '\b'; break;
                    case 'f': decoded = '\f'; break;
                    case 'n': decoded = '\n'; break;
                    case 'r': decoded = '\r'; break;
                    case 't': decoded = '\t'; break;
                    default: goto fail;
                }
                json_buf_char(&b, decoded);
            }
            if (b.failed) goto fail;
        } else if (c < 0x20) {
            goto fail;
        } else {
            json_buf_char(&b, (char)c);
            if (b.failed) goto fail;
        }
    }
fail:
    free(b.data);
    return -1;
}

static int json_parse_literal(JsonParser* p, const char* lit, LunaValue v, LunaValue* out) {
    size_t n = strlen(lit);
    if (p->pos + n > p->len) return -1;
    if (memcmp(p->s + p->pos, lit, n) != 0) return -1;
    p->pos += n;
    *out = v;
    return 0;
}

static int json_parse_number(JsonParser* p, LunaValue* out) {
    size_t start = p->pos;
    int floating = 0;
    if (p->pos < p->len && p->s[p->pos] == '-') p->pos++;
    if (p->pos >= p->len) return -1;
    if (p->s[p->pos] == '0') {
        p->pos++;
    } else if (p->s[p->pos] >= '1' && p->s[p->pos] <= '9') {
        while (p->pos < p->len && isdigit((unsigned char)p->s[p->pos])) p->pos++;
    } else {
        return -1;
    }
    if (p->pos < p->len && p->s[p->pos] == '.') {
        floating = 1;
        p->pos++;
        if (p->pos >= p->len || !isdigit((unsigned char)p->s[p->pos])) return -1;
        while (p->pos < p->len && isdigit((unsigned char)p->s[p->pos])) p->pos++;
    }
    if (p->pos < p->len && (p->s[p->pos] == 'e' || p->s[p->pos] == 'E')) {
        floating = 1;
        p->pos++;
        if (p->pos < p->len && (p->s[p->pos] == '+' || p->s[p->pos] == '-')) p->pos++;
        if (p->pos >= p->len || !isdigit((unsigned char)p->s[p->pos])) return -1;
        while (p->pos < p->len && isdigit((unsigned char)p->s[p->pos])) p->pos++;
    }
    size_t n = p->pos - start;
    char* text = (char*)malloc(n + 1);
    if (!text) return -1;
    memcpy(text, p->s + start, n);
    text[n] = '\0';
    if (!floating) {
        char* end = NULL;
        errno = 0;
        long long iv = strtoll(text, &end, 10);
        if (errno == 0 && end == text + n && iv >= (long long)INT64_MIN && iv <= (long long)INT64_MAX) {
            free(text);
            *out = luna_int((int64_t)iv);
            return 0;
        }
    }
    char* end = NULL;
    errno = 0;
    double d = strtod(text, &end);
    int ok = (end == text + n);
    free(text);
    if (!ok || !isfinite(d)) return -1;
    *out = luna_float(d);
    return 0;
}

static int json_parse_array(LunaState* L, JsonParser* p, LunaValue* out, int depth) {
    p->pos++;
    LunaList* list = luna_list_new(L);
    if (!list) return -1;
    LunaValue arr = luna_list_val(list);
    if (json_keep(L, arr) != 0) return -1;
    json_skip_ws(p);
    if (p->pos < p->len && p->s[p->pos] == ']') {
        p->pos++;
        *out = arr;
        return 0;
    }
    for (;;) {
        LunaValue item = luna_nil();
        if (json_parse_value(L, p, &item, depth) != 0) return -1;
        luna_list_push_value(L, list, item);
        json_skip_ws(p);
        if (p->pos < p->len && p->s[p->pos] == ',') {
            p->pos++;
            continue;
        }
        if (p->pos < p->len && p->s[p->pos] == ']') {
            p->pos++;
            *out = arr;
            return 0;
        }
        return -1;
    }
}

static int json_parse_object(LunaState* L, JsonParser* p, LunaValue* out, int depth) {
    p->pos++;
    LunaTable* table = luna_table_new(L);
    if (!table) return -1;
    LunaValue obj = luna_table_val(table);
    if (json_keep(L, obj) != 0) return -1;
    json_skip_ws(p);
    if (p->pos < p->len && p->s[p->pos] == '}') {
        p->pos++;
        *out = obj;
        return 0;
    }
    for (;;) {
        json_skip_ws(p);
        LunaValue key = luna_nil();
        if (json_parse_string(L, p, &key) != 0) return -1;
        json_skip_ws(p);
        if (p->pos >= p->len || p->s[p->pos] != ':') return -1;
        p->pos++;
        LunaValue value = luna_nil();
        if (json_parse_value(L, p, &value, depth) != 0) return -1;
        luna_table_set(L, table, key, value);
        json_skip_ws(p);
        if (p->pos < p->len && p->s[p->pos] == ',') {
            p->pos++;
            continue;
        }
        if (p->pos < p->len && p->s[p->pos] == '}') {
            p->pos++;
            *out = obj;
            return 0;
        }
        return -1;
    }
}

static int json_parse_value(LunaState* L, JsonParser* p, LunaValue* out, int depth) {
    if (depth > JSON_MAX_DEPTH) return -1;
    json_skip_ws(p);
    if (p->pos >= p->len) return -1;
    char c = p->s[p->pos];
    switch (c) {
        case '{': return json_parse_object(L, p, out, depth + 1);
        case '[': return json_parse_array(L, p, out, depth + 1);
        case '"': return json_parse_string(L, p, out);
        case 't': return json_parse_literal(p, "true", luna_bool(1), out);
        case 'f': return json_parse_literal(p, "false", luna_bool(0), out);
        case 'n': return json_parse_literal(p, "null", luna_nil(), out);
        default:
            if (c == '-' || isdigit((unsigned char)c)) return json_parse_number(p, out);
            return -1;
    }
}

int luna_json_parse_text(LunaState* L, const char* text, size_t len) {
    if (!L || (!text && len)) return -1;
    int base = luna_gettop(L);
    JsonParser p;
    p.s = text ? text : "";
    p.len = len;
    p.pos = 0;
    LunaValue value = luna_nil();
    json_skip_ws(&p);
    if (json_parse_value(L, &p, &value, 0) != 0) {
        luna_settop(L, base);
        return -1;
    }
    json_skip_ws(&p);
    if (p.pos != p.len) {
        luna_settop(L, base);
        return -1;
    }
    luna_settop(L, base);
    lib_push_value(L, value);
    if (luna_gettop(L) != base + 1) return -1;
    return 0;
}

static int json_encode_fn(LunaState* L) {
    JsonBuf buf;
    buf.data = NULL;
    buf.len = 0;
    buf.cap = 0;
    buf.failed = 0;
    JsonWriter w;
    w.buf = &buf;
    w.pretty = 0;
    w.indent = 2;
    if (json_write_value(&w, lib_arg(L, 0), 0) != 0) {
        free(buf.data);
        return lib_error(L, "json: cannot encode value");
    }
    luna_pushlstring(L, buf.data ? buf.data : "", buf.len);
    free(buf.data);
    return 1;
}

static int json_decode_fn(LunaState* L) {
    LunaValue v = lib_arg(L, 0);
    if (v.type != LUNA_TSTRING) return lib_error(L, "json: expected string");
    LunaString* s = luna_val_string(v);
    if (luna_json_parse_text(L, luna_string_data(s), luna_string_len(s)) != 0) {
        return lib_error(L, "invalid JSON");
    }
    return 1;
}

static int json_pretty_fn(LunaState* L) {
    int indent = 2;
    if (lib_nargs(L) > 1) {
        LunaValue arg = lib_arg(L, 1);
        double n = 0;
        if (arg.type == LUNA_TINTEGER) {
            n = (double)luna_val_int(arg);
        } else if (arg.type == LUNA_TFLOAT) {
            n = luna_val_float(arg);
        } else if (arg.type != LUNA_TNIL) {
            return lib_error(L, "json indent must be an integer");
        } else {
            n = 2;
        }
        if (n < 0) n = 0;
        if (n > JSON_MAX_INDENT) n = JSON_MAX_INDENT;
        indent = (int)n;
    }
    JsonBuf buf;
    buf.data = NULL;
    buf.len = 0;
    buf.cap = 0;
    buf.failed = 0;
    JsonWriter w;
    w.buf = &buf;
    w.pretty = 1;
    w.indent = indent;
    if (json_write_value(&w, lib_arg(L, 0), 0) != 0) {
        free(buf.data);
        return lib_error(L, "json: cannot encode value");
    }
    luna_pushlstring(L, buf.data ? buf.data : "", buf.len);
    free(buf.data);
    return 1;
}

static void register_json(LunaState* L, LunaTable* exports) {
    lib_export_function(L, exports, "encode", json_encode_fn);
    lib_export_function(L, exports, "stringify", json_encode_fn);
    lib_export_function(L, exports, "decode", json_decode_fn);
    lib_export_function(L, exports, "parse", json_decode_fn);
    lib_export_function(L, exports, "pretty", json_pretty_fn);
}

static const LunaModule g_json_module = { "json_native", register_json, NULL };

void luna_open_json_lib(LunaState* L) {
    luna_register_module(L, &g_json_module);
}
