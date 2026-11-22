#include "platform.h"
#include "platform_internal.h"

#ifdef _WIN32

#include <windows.h>
#include <io.h>
#include <sys/stat.h>
#include <direct.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* luna_platform_read_file(const char* path, size_t* out_len,
                              char* error_out, size_t error_size) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        if (error_out) {
            snprintf(error_out, error_size, "cannot open '%s': %s",
                     path, strerror(errno));
        }
        return NULL;
    }

    if (_fseeki64(f, 0, SEEK_END) != 0) {
        if (error_out) snprintf(error_out, error_size, "fseek failed on '%s'", path);
        fclose(f);
        return NULL;
    }

    __int64 sz = _ftelli64(f);
    if (sz < 0) {
        if (error_out) snprintf(error_out, error_size, "ftell failed on '%s'", path);
        fclose(f);
        return NULL;
    }
    _fseeki64(f, 0, SEEK_SET);

    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) {
        if (error_out) snprintf(error_out, error_size, "out of memory");
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);

    if (n != (size_t)sz) {
        free(buf);
        if (error_out) snprintf(error_out, error_size, "short read on '%s'", path);
        return NULL;
    }

    buf[sz] = '\0';
    if (out_len) *out_len = (size_t)sz;
    return buf;
}

int luna_platform_write_file(const char* path, const void* data, size_t len,
                             char* error_out, size_t error_size) {
    FILE* f = fopen(path, "wb");
    if (!f) {
        if (error_out) {
            snprintf(error_out, error_size, "cannot open '%s' for writing: %s",
                     path, strerror(errno));
        }
        return LUNA_PLATFORM_ERROR;
    }

    size_t written = fwrite(data, 1, len, f);
    fclose(f);

    if (written != len) {
        if (error_out) snprintf(error_out, error_size, "short write on '%s'", path);
        return LUNA_PLATFORM_ERROR;
    }

    return LUNA_PLATFORM_OK;
}

int luna_platform_file_exists(const char* path) {
    struct _stat64 st;
    return _stat64(path, &st) == 0 ? 1 : 0;
}

int64_t luna_platform_file_size(const char* path) {
    struct _stat64 st;
    if (_stat64(path, &st) != 0) return -1;
    return (int64_t)st.st_size;
}

int luna_platform_is_directory(const char* path) {
    struct _stat64 st;
    if (_stat64(path, &st) != 0) return 0;
    return (st.st_mode & _S_IFDIR) ? 1 : 0;
}

int luna_platform_mkdir(const char* path, int create_parents) {
    if (create_parents) {
        char* tmp = platform_strdup(path);
        if (!tmp) return LUNA_PLATFORM_ERROR;

        size_t len = strlen(tmp);
        if (len > 0 && (tmp[len - 1] == '/' || tmp[len - 1] == '\\')) {
            tmp[len - 1] = '\0';
        }

        for (char* p = tmp + 1; *p; p++) {
            if (*p == '/' || *p == '\\') {
                *p = '\0';
                if (_mkdir(tmp) != 0 && errno != EEXIST) {
                    free(tmp);
                    return LUNA_PLATFORM_ERROR;
                }
                *p = '\\';
            }
        }
        int r = _mkdir(tmp);
        free(tmp);
        if (r != 0 && errno != EEXIST) return LUNA_PLATFORM_ERROR;
        return LUNA_PLATFORM_OK;
    } else {
        if (_mkdir(path) != 0) return LUNA_PLATFORM_ERROR;
        return LUNA_PLATFORM_OK;
    }
}

#endif /* _WIN32 */
