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

#define STRING_POOL_INITIAL 256

void luna_string_pool_init(LunaState* L) {
    L->string_buckets = NULL;
    L->string_bucket_count = 0;
    L->string_pool_count = 0;

    LunaString** buckets = (LunaString**)luna_alloc(
        L, NULL, 0, STRING_POOL_INITIAL * sizeof(LunaString*));
    if (!buckets) return;

    for (uint32_t i = 0; i < STRING_POOL_INITIAL; i++) buckets[i] = NULL;
    L->string_buckets = buckets;
    L->string_bucket_count = STRING_POOL_INITIAL;
}

static int string_pool_grow(LunaState* L) {
    uint32_t new_count = L->string_bucket_count * 2;
    LunaString** buckets = (LunaString**)luna_alloc(
        L, NULL, 0, new_count * sizeof(LunaString*));
    if (!buckets) return 0;

    for (uint32_t i = 0; i < new_count; i++) buckets[i] = NULL;

    for (uint32_t i = 0; i < L->string_bucket_count; i++) {
        LunaString* s = L->string_buckets[i];
        while (s) {
            LunaString* next = s->pool_next;
            uint32_t idx = (uint32_t)(s->hash & (new_count - 1));
            s->pool_next = buckets[idx];
            buckets[idx] = s;
            s = next;
        }
    }

    luna_alloc(L, L->string_buckets,
               L->string_bucket_count * sizeof(LunaString*), 0);
    L->string_buckets = buckets;
    L->string_bucket_count = new_count;
    return 1;
}

LunaString* luna_string_new(LunaState* L, const char* data, size_t len) {
    uint64_t hash = fnv1a(data, len);

    if (L && L->string_buckets) {
        uint32_t idx = (uint32_t)(hash & (L->string_bucket_count - 1));
        for (LunaString* cur = L->string_buckets[idx]; cur; cur = cur->pool_next) {
            if (cur->hash == hash && cur->len == len &&
                memcmp(cur->data, data, len) == 0) {
                return cur;
            }
        }
    }

    LunaString* s = (LunaString*)luna_alloc(L, NULL, 0, sizeof(LunaString) + len + 1);
    if (!s) return NULL;

    s->header.type = LUNA_TSTRING;
    s->header.marked = 0;
    s->header.gc_next = L->all_objects;
    L->all_objects = &s->header;

    s->hash = hash;
    s->len = len;
    s->pool_next = NULL;
    memcpy(s->data, data, len);
    s->data[len] = '\0';

    if (L->string_buckets) {
        if (L->string_pool_count >= L->string_bucket_count * 3 / 4) {
            string_pool_grow(L);
        }
        uint32_t idx = (uint32_t)(hash & (L->string_bucket_count - 1));
        s->pool_next = L->string_buckets[idx];
        L->string_buckets[idx] = s;
        L->string_pool_count++;
    }

    return s;
}

LunaString* luna_string_newz(LunaState* L, const char* s) {
    return luna_string_new(L, s, strlen(s));
}

void luna_string_pool_purge(LunaState* L) {
    if (!L->string_buckets) return;

    for (uint32_t i = 0; i < L->string_bucket_count; i++) {
        LunaString** link = &L->string_buckets[i];
        while (*link) {
            LunaString* s = *link;
            if (s->header.marked == LUNA_GC_MARK_WHITE) {
                *link = s->pool_next;
                L->string_pool_count--;
            } else {
                link = &s->pool_next;
            }
        }
    }
}

void luna_string_pool_free(LunaState* L) {
    if (!L->string_buckets) return;
    luna_alloc(L, L->string_buckets,
               L->string_bucket_count * sizeof(LunaString*), 0);
    L->string_buckets = NULL;
    L->string_bucket_count = 0;
    L->string_pool_count = 0;
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