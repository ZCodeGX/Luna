#ifndef LUNA_MODULES_INTERNAL_H
#define LUNA_MODULES_INTERNAL_H

#include "module.h"
#include "../runtime/value.h"
#include <stddef.h>

#define LUNA_MAX_MODULE_PATHS   32
#define LUNA_MAX_PATH_LEN       1024
#define LUNA_MAX_LOADING_STACK  64

/* ---------- Cache de módulos ---------- */

typedef struct LunaModuleEntry {
    char*                  name;     /* nombre canónico (malloc'd) */
    LunaModuleInternal             module;
    struct LunaModuleEntry* next;    /* lista enlazada (hash chaining) */
} LunaModuleEntry;

#define LUNA_MODULE_CACHE_SIZE 64

typedef struct {
    LunaModuleEntry* buckets[LUNA_MODULE_CACHE_SIZE];
    size_t           count;
} LunaModuleCache;

/* ---------- Stack de carga (detección de ciclos) ---------- */

typedef struct {
    char* names[LUNA_MAX_LOADING_STACK];
    int   depth;
} LunaLoadingStack;

/* ---------- Sistema de módulos (parte de LunaState) ---------- */

typedef struct LunaModules {
    /* Search paths */
    char*  paths[LUNA_MAX_MODULE_PATHS];
    int    num_paths;
    
    /* Cache */
    LunaModuleCache cache;
    
    /* Detección de ciclos */
    LunaLoadingStack loading;
    
    /* Directorio del archivo actualmente en ejecución (para imports relativos) */
    char   current_dir[LUNA_MAX_PATH_LEN];
} LunaModules;

/* ---------- API interna ---------- */

void  modules_init(LunaModules* M);
void  modules_free(LunaModules* M);

/* Cache */
LunaModuleInternal* modules_cache_get(LunaModules* M, const char* name);
int         modules_cache_put(LunaModules* M, const char* name, LunaModuleInternal* mod);

/* Loading stack (detección de ciclos) */
int  modules_loading_push(LunaModules* M, const char* name);
void modules_loading_pop(LunaModules* M);
int  modules_loading_contains(LunaModules* M, const char* name);

/* Resolver: busca un archivo .luna en el search path.
 * Devuelve un string malloc'd con la ruta absoluta, o NULL si no se encuentra. */
char* modules_resolve(LunaModules* M, const char* name, const char* from_dir);

/* NUEVA: establecer directorio actual */
void  modules_set_current_dir(LunaModules* M, const char* file_path);

int luna_register_module_internal(struct LunaState* L, const LunaModuleInternal* mod);

/* Carga un módulo Luna desde un archivo. Devuelve el proto compilado. */
struct LunaProto* modules_load_luna(LunaModules* M, struct LunaState* L,
                                    const char* path, char* error_out, size_t error_size);

/* Verifica visibilidad: ¿puede 'from' acceder al símbolo 'name' en 'mod'? */
int modules_check_visibility(LunaModules* M, const LunaModuleInternal* mod,
                             const char* symbol_name, char* error_out, size_t error_size);

#endif /* LUNA_MODULES_INTERNAL_H */