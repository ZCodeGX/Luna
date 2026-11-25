#include "platform_internal.h"

#include <direct.h>
#include <errno.h>
#include <stdio.h>

#define LUNA_PROCESS_OUTPUT_LIMIT ((size_t)8 * 1024 * 1024)

int luna_platform_run_process(const char* command, const char* cwd, int capture,
                              char** out_stdout, size_t* out_stdout_len,
                              char** out_stderr, size_t* out_stderr_len,
                              int* out_code, char* error_out, size_t error_size) {
    if (out_stdout) {
        *out_stdout = NULL;
        if (out_stdout_len) *out_stdout_len = 0;
    }
    if (out_stderr) {
        *out_stderr = NULL;
        if (out_stderr_len) *out_stderr_len = 0;
    }
    if (out_code) *out_code = -1;

    if (!command) {
        platform_set_error(error_out, error_size, "comando vacío");
        return LUNA_PLATFORM_ERROR;
    }

    char* previous = NULL;
    if (cwd) {
        previous = _getcwd(NULL, 0);
        if (_chdir(cwd) != 0) {
            free(previous);
            platform_set_error(error_out, error_size,
                               "no se pudo cambiar de directorio: %s", strerror(errno));
            return LUNA_PLATFORM_ERROR;
        }
    }

    int code = -1;
    if (capture) {
        size_t n = strlen(command);
        char* full = (char*)malloc(n + 16);
        if (!full) {
            if (previous) { _chdir(previous); free(previous); }
            platform_set_error(error_out, error_size, "out of memory");
            return LUNA_PLATFORM_ERROR;
        }
        snprintf(full, n + 16, "%s 2>&1", command);
        FILE* pipe = _popen(full, "r");
        free(full);
        if (!pipe) {
            if (previous) { _chdir(previous); free(previous); }
            platform_set_error(error_out, error_size, "no se pudo ejecutar el comando");
            return LUNA_PLATFORM_ERROR;
        }

        size_t len = 0;
        size_t cap = 0;
        char* data = NULL;
        char chunk[8192];
        size_t got = 0;
        while ((got = fread(chunk, 1, sizeof(chunk), pipe)) > 0) {
            if (len >= LUNA_PROCESS_OUTPUT_LIMIT) continue;
            size_t take = got;
            if (take > LUNA_PROCESS_OUTPUT_LIMIT - len) {
                take = LUNA_PROCESS_OUTPUT_LIMIT - len;
            }
            if (len + take + 1 > cap) {
                size_t next_cap = cap ? cap : 256;
                while (next_cap < len + take + 1) next_cap *= 2;
                char* next = (char*)realloc(data, next_cap);
                if (!next) break;
                data = next;
                cap = next_cap;
            }
            memcpy(data + len, chunk, take);
            len += take;
            data[len] = '\0';
        }
        code = _pclose(pipe);
        if (out_stdout) {
            *out_stdout = data;
            if (out_stdout_len) *out_stdout_len = len;
        } else {
            free(data);
        }
    } else {
        code = system(command);
    }

    if (previous) {
        _chdir(previous);
        free(previous);
    }
    if (out_code) *out_code = code;
    return LUNA_PLATFORM_OK;
}
