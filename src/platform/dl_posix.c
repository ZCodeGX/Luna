#include "platform.h"
#include "platform_internal.h"

#ifndef _WIN32

#include <dlfcn.h>
#include <string.h>

LunaDynLib luna_platform_dlopen(const char* path, char* error_out, size_t error_size) {
    void* lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!lib) {
        if (error_out) {
            const char* err = dlerror();
            snprintf(error_out, error_size, "dlopen failed: %s", err ? err : "unknown");
        }
        return NULL;
    }
    return lib;
}

LunaDynSym luna_platform_dlsym(LunaDynLib lib, const char* name,
                               char* error_out, size_t error_size) {
    /* Limpiar errores previos */
    dlerror();

    void* sym = dlsym(lib, name);
    const char* err = dlerror();
    if (err) {
        if (error_out) {
            snprintf(error_out, error_size, "dlsym('%s') failed: %s", name, err);
        }
        return NULL;
    }
    return sym;
}

void luna_platform_dlclose(LunaDynLib lib) {
    if (lib) dlclose(lib);
}

const char* luna_platform_dl_extension(void) {
#if defined(__APPLE__)
    return ".dylib";
#else
    return ".so";
#endif
}

#endif /* !_WIN32 */
