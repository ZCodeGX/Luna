#include "modules_internal.h"
#include "../runtime/runtime_internal.h"
#include "../bytecode/proto.h"
#include <stdlib.h>
#include <string.h>

/* FNV-1a 32-bit para el hash del cache */
static uint32_t hash_name(const char* name) {
    uint32_t h = 2166136261u;
    for (const char* p = name; *p; p++) {
        h ^= (uint8_t)*p;
        h *= 16777619u;
    }
    return h;
}

void modules_cache_init(LunaModuleCache* C) {
    memset(C, 0, sizeof(*C));
}

void modules_cache_free(LunaModuleCache* C) {
    for (size_t i = 0; i < LUNA_MODULE_CACHE_SIZE; i++) {
        LunaModuleEntry* e = C->buckets[i];
        while (e) {
            LunaModuleEntry* next = e->next;
            free(e->name);
            free(e->module.source_path);
            /* Nota: proto y exports los libera el GC o el shutdown del módulo */
            free(e);
            e = next;
        }
        C->buckets[i] = NULL;
    }
    C->count = 0;
}

LunaModuleInternal* modules_cache_get(LunaModules* M, const char* name) {
    uint32_t h = hash_name(name) % LUNA_MODULE_CACHE_SIZE;
    LunaModuleEntry* e = M->cache.buckets[h];
    while (e) {
        if (strcmp(e->name, name) == 0) {
            return &e->module;
        }
        e = e->next;
    }
    return NULL;
}

int modules_cache_put(LunaModules* M, const char* name, LunaModuleInternal* mod) {
    /* Verificar que no exista ya */
    if (modules_cache_get(M, name)) return -1;
    
    LunaModuleEntry* e = (LunaModuleEntry*)calloc(1, sizeof(LunaModuleEntry));
    if (!e) return -1;
    
    size_t len = strlen(name);
    e->name = (char*)malloc(len + 1);
    if (!e->name) {
        free(e);
        return -1;
    }
    memcpy(e->name, name, len + 1);
    
    e->module = *mod;
    
    uint32_t h = hash_name(name) % LUNA_MODULE_CACHE_SIZE;
    e->next = M->cache.buckets[h];
    M->cache.buckets[h] = e;
    M->cache.count++;
    
    return 0;
}

void modules_cache_remove(LunaState* L, const char* name) {
    uint32_t h = hash_name(name) % LUNA_MODULE_CACHE_SIZE;
    LunaModuleEntry** link = &L->modules.cache.buckets[h];
    while (*link) {
        LunaModuleEntry* e = *link;
        if (strcmp(e->name, name) == 0) {
            *link = e->next;
            L->modules.cache.count--;

            if (e->module.shutdown_fn) e->module.shutdown_fn(L);
            luna_proto_release(e->module.proto);
            free(e->name);
            free(e->module.source_path);
            free(e);
            return;
        }
        link = &e->next;
    }
}