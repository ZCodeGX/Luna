#include "api_internal.h"

void luna_register_function(LunaState* L, const char* name, LunaCFunction fn) {
    luna_pushcfunction(L, fn);
    if (L->status != LUNA_OK) return;
    luna_setglobal(L, name);
}

void luna_register_module(LunaState* L, const LunaModule* mod) {
    if (!mod || !mod->name) return;

    /* Convertir a LunaModuleInternal y registrar */
    LunaModuleInternal internal;
    internal.name = mod->name;
    internal.register_fn = mod->register_functions;
    internal.shutdown_fn = mod->shutdown;
    internal.proto = NULL;
    internal.exports = NULL;
    internal.source_path = NULL;
    internal.loaded = 0;

    if (luna_register_module_internal(L, &internal) != 0) {
        luna_error_set_simple(L, LUNA_ERR_MODULE,
                              "failed to register module '%s'", mod->name);
    }
}