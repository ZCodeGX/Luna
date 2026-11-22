#ifndef LUNA_LSTRING_H
#define LUNA_LSTRING_H

#include "value.h"
#include <stddef.h>

struct LunaString {
    LunaGCObject header;
    uint64_t     hash;
    size_t       len;
    char         data[];
};

LunaString* luna_string_new(LunaState* L, const char* data, size_t len);
LunaString* luna_string_newz(LunaState* L, const char* s);
int luna_string_equal(LunaString* a, LunaString* b);
uint64_t luna_string_hash(LunaString* s);

static inline size_t luna_string_len(LunaString* s) { return s->len; }
static inline const char* luna_string_data(LunaString* s) { return s->data; }

#endif