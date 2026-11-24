#include "modules_internal.h"
#include "module.h"
#include "../runtime/runtime_internal.h"
#include "../runtime/table.h"
#include "../runtime/lstring.h"
#include "../bytecode/proto.h"
#include "../errors/errors_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward: la VM ejecutará el proto y llenará la tabla de exports.
 * En v0.1 sin VM, esto es un placeholder que falla. */
extern int luna_vm_execute_module(struct LunaState* L, struct LunaProto* proto,
                                  struct LunaTable* exports);

/* ---------- Carga de módulos Luna ---------- */

static char* read_file(const char* path, size_t* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    
    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    
    if (n != (size_t)sz) {
        free(buf);
        return NULL;
    }
    buf[sz] = '\0';
    
    if (out_len) *out_len = (size_t)sz;
    return buf;
}

/*
 * Placeholder para el parser+compilador.
 * En la versión completa, esto llamaría a luna_parse() + luna_compile().
 */
extern struct LunaProto* luna_compile_source(struct LunaState* L,
                                             const char* source, size_t length,
                                             const char* filename,
                                             char* error_out, size_t error_size);

struct LunaProto* modules_load_luna(LunaModules* M, struct LunaState* L,
                                    const char* path, char* error_out, size_t error_size) {
    (void)M;
    
    size_t src_len = 0;
    char* source = read_file(path, &src_len);
    if (!source) {
        if (error_out) snprintf(error_out, error_size, "cannot read file '%s'", path);
        return NULL;
    }
    
    /* Guardar directorio actual para imports relativos dentro del módulo */
    char saved_dir[LUNA_MAX_PATH_LEN];
    memcpy(saved_dir, M->current_dir, sizeof(saved_dir));
    modules_set_current_dir(M, path);
    
    struct LunaProto* proto = luna_compile_source(L, source, src_len, path,
                                                   error_out, error_size);
    free(source);
    
    /* Restaurar directorio */
    memcpy(M->current_dir, saved_dir, sizeof(saved_dir));
    
    return proto;
}

/* ---------- Import público ---------- */

struct LunaTable* luna_import_module(struct LunaState* L, const char* name) {
    LunaModules* M = &L->modules;
    
    /* 1. ¿Ya está en cache? */
    LunaModuleInternal* cached = modules_cache_get(M, name);
    if (cached && cached->loaded) {
        return cached->exports;
    }
    
    /* 2. ¿Ciclo de imports? */
    if (modules_loading_contains(M, name)) {
        luna_error_set_simple(L, LUNA_ERR_MODULE, "circular import detected: '%s'", name);
        return NULL;
    }
    
    /* 3. ¿Es un módulo nativo registrado? */
    if (cached && cached->register_fn) {
        if (cached->loaded && cached->exports) {
            return cached->exports;
        }

        /* Crear tabla de exports */
        struct LunaTable* exports = luna_table_new(L);
        if (!exports) return NULL;
        
        modules_loading_push(M, name);
        cached->register_fn(L, exports);
        modules_loading_pop(M);
        
        cached->exports = exports;
        cached->loaded = 1;
        return exports;
    }
    
    /* 4. Resolver path del archivo */
    char* path = modules_resolve(M, name, M->current_dir);
    if (!path) {
        luna_error_set_simple(L, LUNA_ERR_MODULE, "module '%s' not found", name);
        return NULL;
    }
    
    /* 5. Cargar y compilar */
    char error_buf[256] = {0};
    struct LunaProto* proto = modules_load_luna(M, L, path, error_buf, sizeof(error_buf));
    if (!proto) {
        free(path);
        luna_error_set_simple(L, LUNA_ERR_MODULE, "failed to load module '%s': %s",
                              name, error_buf);
        return NULL;
    }
    
    /* 6. Crear módulo y añadirlo al cache (aún no cargado) */
    LunaModuleInternal mod = {0};
    mod.name = name;
    mod.proto = proto;
    mod.source_path = path;
    mod.exports = luna_table_new(L);
    if (!mod.exports) {
        luna_proto_free(proto);
        free(path);
        return NULL;
    }
    
    if (modules_cache_put(M, name, &mod) != 0) {
        luna_proto_free(proto);
        free(path);
        return NULL;
    }
    
    /* 7. Ejecutar el proto (llenando exports) */
    LunaModuleInternal* stored = modules_cache_get(M, name);
    modules_loading_push(M, name);
    
    int exec_ok = luna_vm_execute_module(L, proto, stored->exports);
    
    modules_loading_pop(M);
    
    if (exec_ok != 0) {
        /* No dejamos una entrada fallida en el cache: un reintento debe volver a cargar */
        modules_cache_remove(L, name);
        return NULL;
    }

    LunaString* exports_key = luna_string_newz(L, "__exports__");
    LunaValue exports_val;
    if (exports_key && luna_table_get(L->globals, luna_string_val(exports_key),
                                      &exports_val) &&
        exports_val.type == LUNA_TTABLE) {
        stored->exports = luna_val_table(exports_val);
    }
    
    stored->loaded = 1;
    return stored->exports;
}

/* ---------- Registro de módulos nativos ---------- */

int luna_register_module_internal(struct LunaState* L, const LunaModuleInternal* mod) {
    if (!mod || !mod->name || !mod->register_fn) return -1;
    
    LunaModules* M = &L->modules;
    
    /* Copiar el descriptor */
    LunaModuleInternal copy = *mod;
    copy.source_path = NULL;
    copy.exports = NULL;
    copy.loaded = 0;
    
    return modules_cache_put(M, mod->name, &copy);
}

/* ---------- Obtener módulo cargado ---------- */

struct LunaTable* luna_get_module(struct LunaState* L, const char* name) {
    LunaModuleInternal* m = modules_cache_get(&L->modules, name);
    if (!m || !m->loaded) return NULL;
    return m->exports;
}

/* ---------- Añadir search path ---------- */

int luna_add_module_path(struct LunaState* L, const char* path) {
    LunaModules* M = &L->modules;
    if (!path || !path[0]) return -1;
    if (M->num_paths >= LUNA_MAX_MODULE_PATHS) return -1;

    for (int i = 0; i < M->num_paths; i++) {
        if (strcmp(M->paths[i], path) == 0) return 0;
    }

    size_t len = strlen(path);
    char* copy = (char*)malloc(len + 1);
    if (!copy) return -1;
    memcpy(copy, path, len + 1);
    
    M->paths[M->num_paths++] = copy;
    return 0;
}

int luna_add_path(struct LunaState* L, const char* path) {
    return luna_add_module_path(L, path);
}