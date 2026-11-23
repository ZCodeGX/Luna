#include "modules_internal.h"
#include "../bytecode/proto.h"
#include <stdlib.h>
#include <string.h>

void modules_init(LunaModules* M) {
    memset(M, 0, sizeof(*M));
    memset(&M->cache, 0, sizeof(M->cache));
    memset(&M->loading, 0, sizeof(M->loading));
    M->current_dir[0] = '\0';
}

void modules_free(struct LunaState* L, LunaModules* M) {
    /* Liberar search paths */
    for (int i = 0; i < M->num_paths; i++) {
        free(M->paths[i]);
    }
    M->num_paths = 0;
    
    /* Liberar cache */
    for (size_t i = 0; i < LUNA_MODULE_CACHE_SIZE; i++) {
        LunaModuleEntry* e = M->cache.buckets[i];
        while (e) {
            LunaModuleEntry* next = e->next;
            if (e->module.shutdown_fn) e->module.shutdown_fn(L);
            if (e->module.proto) luna_proto_free(e->module.proto);
            free(e->name);
            free(e->module.source_path);
            /* exports es un objeto GC y se libera con el estado */
            free(e);
            e = next;
        }
        M->cache.buckets[i] = NULL;
    }
    M->cache.count = 0;
    
    /* Liberar loading stack */
    for (int i = 0; i < M->loading.depth; i++) {
        free(M->loading.names[i]);
    }
    M->loading.depth = 0;
}

/* ---------- Loading stack (detección de ciclos) ---------- */

int modules_loading_push(LunaModules* M, const char* name) {
    if (M->loading.depth >= LUNA_MAX_LOADING_STACK) return -1;
    
    size_t len = strlen(name);
    char* copy = (char*)malloc(len + 1);
    if (!copy) return -1;
    memcpy(copy, name, len + 1);
    
    M->loading.names[M->loading.depth++] = copy;
    return 0;
}

void modules_loading_pop(LunaModules* M) {
    if (M->loading.depth == 0) return;
    M->loading.depth--;
    free(M->loading.names[M->loading.depth]);
    M->loading.names[M->loading.depth] = NULL;
}

int modules_loading_contains(LunaModules* M, const char* name) {
    for (int i = 0; i < M->loading.depth; i++) {
        if (strcmp(M->loading.names[i], name) == 0) return 1;
    }
    return 0;
}