#include "lstring.h"          /* <-- cambia de "string.h" a "lstring.h" */
#include "runtime_internal.h"
#include <stdlib.h>
#include <string.h>

static uint64_t fnv1a(const char* data, size_t len) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint8_t)data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

LunaString* luna_string_new(LunaState* L, const char* data, size_t len) {
    LunaString* s = (LunaString*)luna_alloc(L, NULL, 0, sizeof(LunaString) + len + 1);
    if (!s) return NULL;
    
    s->header.type = LUNA_TSTRING;
    s->header.marked = 0;
    s->header.gc_next = L->all_objects;
    L->all_objects = &s->header;
    
    s->hash = fnv1a(data, len);
    s->len = len;
    memcpy(s->data, data, len);
    s->data[len] = '\0';
    
    return s;
}

LunaString* luna_string_newz(LunaState* L, const char* s) {
    return luna_string_new(L, s, strlen(s));
}

int luna_string_equal(LunaString* a, LunaString* b) {
    if (a == b) return 1;
    if (a->len != b->len) return 0;
    if (a->hash != b->hash) return 0;
    return memcmp(a->data, b->data, a->len) == 0;
}

uint64_t luna_string_hash(LunaString* s) {
    return s->hash;
}