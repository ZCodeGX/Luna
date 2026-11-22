#ifndef LUNA_MODULE_H
#define LUNA_MODULE_H

#include <stddef.h>

struct LunaState;
struct LunaTable;
struct LunaProto;

/*
 * Descriptor de un módulo. Puede ser:
 *   - Módulo Luna: proto != NULL, register_fn == NULL
 *   - Módulo nativo: proto == NULL, register_fn != NULL
 */
typedef struct LunaModuleInternal {
    const char* name;                              /* nombre canónico (ej: "math.basic") */
    
    /* Para módulos nativos */
    void (*register_fn)(struct LunaState* L, struct LunaTable* exports);
    void (*shutdown_fn)(struct LunaState* L);      /* opcional, al cerrar el estado */
    
    /* Para módulos Luna (se rellena tras la carga) */
    struct LunaProto*  proto;
    struct LunaTable*  exports;                    /* tabla con los símbolos pub */
    
    /* Metadata */
    char*  source_path;                            /* ruta absoluta al archivo .luna (o NULL) */
    int    loaded;                                 /* 1 si ya está cargado y en cache */
} LunaModuleInternal;

/* ---------- API pública ---------- */

/*
 * Registra un módulo nativo. El estado toma posesión del descriptor.
 * register_fn se llamará la primera vez que se importe el módulo.
 */
int luna_register_module_internal(struct LunaState* L, const LunaModuleInternal* mod);

/*
 * Importa un módulo por nombre (ej: "math.basic").
 * Si ya está en cache, devuelve la tabla de exports.
 * Si no, lo carga (desde disco o nativo), lo cachea y devuelve la tabla.
 *
 * Devuelve NULL en error; el mensaje queda en L->error_message.
 * La tabla devuelta es propiedad del estado (no liberar).
 */
struct LunaTable* luna_import_module(struct LunaState* L, const char* name);

/*
 * Añade un directorio al search path de módulos.
 * Los paths se buscan en orden: primero los añadidos por el usuario,
 * luego el directorio del archivo actual, luego los built-in.
 */
int luna_add_module_path(struct LunaState* L, const char* path);

/*
 * Obtiene la tabla de exports de un módulo ya cargado.
 * Devuelve NULL si el módulo no está en cache.
 */
struct LunaTable* luna_get_module(struct LunaState* L, const char* name);

#endif /* LUNA_MODULE_H */