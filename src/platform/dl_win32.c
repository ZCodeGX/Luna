#include "platform.h"
#include "platform_internal.h"

#ifdef _WIN32

#include <windows.h>
#include <string.h>

LunaDynLib luna_platform_dlopen(const char* path, char* error_out, size_t error_size) {
    HMODULE lib = LoadLibraryA(path);
    if (!lib) {
        if (error_out) {
            DWORD err = GetLastError();
            snprintf(error_out, error_size,
                     "LoadLibrary failed for '%s' (error code %lu)", path, (unsigned long)err);
        }
        return NULL;
    }
    return (LunaDynLib)lib;
}

LunaDynSym luna_platform_dlsym(LunaDynLib lib, const char* name,
                               char* error_out, size_t error_size) {
    FARPROC sym = GetProcAddress((HMODULE)lib, name);
    if (!sym) {
        if (error_out) {
            DWORD err = GetLastError();
            snprintf(error_out, error_size,
                     "GetProcAddress('%s') failed (error code %lu)",
                     name, (unsigned long)err);
        }
        return NULL;
    }
    return (LunaDynSym)sym;
}

void luna_platform_dlclose(LunaDynLib lib) {
    if (lib) FreeLibrary((HMODULE)lib);
}

const char* luna_platform_dl_extension(void) {
    return ".dll";
}

#endif /* _WIN32 */
