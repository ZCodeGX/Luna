#include "stdlib_internal.h"

#include "../platform/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* path_arg(LunaState* L, int index) {
    LunaValue v = lib_arg(L, index);
    if (v.type != LUNA_TSTRING) {
        lib_error(L, "expected string path, got %s", luna_type_name(v.type));
        return NULL;
    }
    return (const char*)luna_string_data(luna_val_string(v));
}

static int push_owned_string(LunaState* L, char* text) {
    if (!text) return lib_error(L, "out of memory");
    luna_pushstring(L, text);
    free(text);
    return 1;
}

static char* dup_arg_string(LunaState* L, LunaValue value, const char* label) {
    if (value.type != LUNA_TSTRING) {
        lib_error(L, "expected string %s, got %s", label, luna_type_name(value.type));
        return NULL;
    }
    LunaString* text = luna_val_string(value);
    if (memchr(text->data, '\0', text->len)) {
        lib_error(L, "%s contains a NUL byte", label);
        return NULL;
    }
    char* copy = (char*)malloc(text->len + 1);
    if (!copy) {
        lib_error(L, "out of memory");
        return NULL;
    }
    memcpy(copy, text->data, text->len);
    copy[text->len] = '\0';
    return copy;
}

static int io_read(LunaState* L) {
    const char* path = path_arg(L, 0);
    if (!path) return -1;
    size_t len = 0;
    char err[256] = {0};
    char* data = luna_platform_read_file(path, &len, err, sizeof(err));
    if (!data) return lib_error(L, "%s", err[0] ? err : "cannot read file");
    luna_pushlstring(L, data, len);
    free(data);
    return 1;
}

static int io_write(LunaState* L) {
    const char* path = path_arg(L, 0);
    LunaValue data = lib_arg(L, 1);
    if (!path) return -1;
    if (data.type != LUNA_TSTRING) return lib_error(L, "expected string data");
    LunaString* s = luna_val_string(data);
    char err[256] = {0};
    int ok = luna_platform_write_file(path, s->data, s->len, err, sizeof(err));
    luna_pushboolean(L, ok == LUNA_PLATFORM_OK);
    return 1;
}

static int io_exists(LunaState* L) {
    const char* path = path_arg(L, 0);
    if (!path) return -1;
    luna_pushboolean(L, luna_platform_file_exists(path));
    return 1;
}

static int io_size(LunaState* L) {
    const char* path = path_arg(L, 0);
    if (!path) return -1;
    luna_pushinteger(L, luna_platform_file_size(path));
    return 1;
}

static int io_is_dir(LunaState* L) {
    const char* path = path_arg(L, 0);
    if (!path) return -1;
    luna_pushboolean(L, luna_platform_is_directory(path));
    return 1;
}

static int io_mkdir(LunaState* L) {
    const char* path = path_arg(L, 0);
    if (!path) return -1;
    int parents = 0;
    if (lib_nargs(L) > 1) {
        LunaValue pv = lib_arg(L, 1);
        parents = luna_value_truthy(pv);
    }
    luna_pushboolean(L, luna_platform_mkdir(path, parents) == LUNA_PLATFORM_OK);
    return 1;
}

static int os_getenv(LunaState* L) {
    const char* name = path_arg(L, 0);
    if (!name) return -1;
    const char* value = getenv(name);
    if (!value) luna_pushnil(L);
    else luna_pushstring(L, value);
    return 1;
}

static int os_cwd(LunaState* L) {
    (void)L;
    return push_owned_string(L, luna_platform_getcwd());
}

static int os_separator(LunaState* L) {
    char sep[2] = { luna_platform_path_separator(), '\0' };
    luna_pushstring(L, sep);
    return 1;
}

static int os_name(LunaState* L) {
#if defined(_WIN32)
    const char* name = "windows";
#elif defined(__APPLE__)
    const char* name = "macos";
#else
    const char* name = "linux";
#endif
    luna_pushstring(L, name);
    return 1;
}

static int os_run(LunaState* L) {
    char* command = dup_arg_string(L, lib_arg(L, 0), "command");
    if (!command) return -1;
    char* cwd = NULL;
    if (lib_nargs(L) > 1 && lib_arg(L, 1).type != LUNA_TNIL) {
        cwd = dup_arg_string(L, lib_arg(L, 1), "cwd");
        if (!cwd) {
            free(command);
            return -1;
        }
    }

    char* stdout_data = NULL;
    char* stderr_data = NULL;
    size_t stdout_len = 0;
    size_t stderr_len = 0;
    int code = -1;
    char message[256] = {0};
    int status = luna_platform_run_process(command, cwd, 1,
                                           &stdout_data, &stdout_len,
                                           &stderr_data, &stderr_len,
                                           &code, message, sizeof(message));
    free(command);
    free(cwd);
    if (status != LUNA_PLATFORM_OK) {
        free(stdout_data);
        free(stderr_data);
        return lib_error(L, "%s", message[0] ? message : "cannot run command");
    }

    luna_newtable(L);
    int result = luna_gettop(L);
    luna_pushboolean(L, code == 0);
    luna_setfield(L, result, "ok");
    luna_pushinteger(L, code);
    luna_setfield(L, result, "code");
    luna_pushlstring(L, stdout_data ? stdout_data : "", stdout_len);
    luna_setfield(L, result, "stdout");
    luna_pushlstring(L, stderr_data ? stderr_data : "", stderr_len);
    luna_setfield(L, result, "stderr");
    free(stdout_data);
    free(stderr_data);
    return 1;
}

static int os_system(LunaState* L) {
    char* command = dup_arg_string(L, lib_arg(L, 0), "command");
    if (!command) return -1;
    char* cwd = NULL;
    if (lib_nargs(L) > 1 && lib_arg(L, 1).type != LUNA_TNIL) {
        cwd = dup_arg_string(L, lib_arg(L, 1), "cwd");
        if (!cwd) {
            free(command);
            return -1;
        }
    }

    int code = -1;
    char message[256] = {0};
    int status = luna_platform_run_process(command, cwd, 0,
                                           NULL, NULL, NULL, NULL,
                                           &code, message, sizeof(message));
    free(command);
    free(cwd);
    if (status != LUNA_PLATFORM_OK) {
        return lib_error(L, "%s", message[0] ? message : "cannot run command");
    }
    luna_pushinteger(L, code);
    return 1;
}

static int time_now(LunaState* L) {
    (void)L;
    luna_pushfloat(L, (double)luna_platform_wall_ns() / 1e9);
    return 1;
}

static int time_nanos(LunaState* L) {
    (void)L;
    luna_pushinteger(L, (int64_t)luna_platform_monotonic_ns());
    return 1;
}

static int time_mono(LunaState* L) {
    (void)L;
    luna_pushfloat(L, (double)luna_platform_monotonic_ns() / 1e9);
    return 1;
}

static int time_sleep(LunaState* L) {
    LunaValue v = lib_arg(L, 0);
    if (v.type != LUNA_TINTEGER && v.type != LUNA_TFLOAT)
        return lib_error(L, "expected number of seconds");
    double secs = v.type == LUNA_TINTEGER ? (double)luna_val_int(v) : luna_val_float(v);
    if (secs < 0) return lib_error(L, "sleep time cannot be negative");
    luna_platform_sleep_ns((uint64_t)(secs * 1e9));
    luna_pushnil(L);
    return 1;
}

static int time_since(LunaState* L) {
    LunaValue v = lib_arg(L, 0);
    if (v.type != LUNA_TINTEGER) return lib_error(L, "expected nanoseconds from time.nanos()");
    uint64_t start = (uint64_t)luna_val_int(v);
    uint64_t now = luna_platform_monotonic_ns();
    luna_pushfloat(L, (double)(now - start) / 1e9);
    return 1;
}

static int path_join(LunaState* L) {
    const char* a = path_arg(L, 0);
    const char* b = path_arg(L, 1);
    if (!a || !b) return -1;
    return push_owned_string(L, luna_platform_path_join(a, b));
}

static int path_normalize_fn(LunaState* L) {
    const char* p = path_arg(L, 0);
    if (!p) return -1;
    return push_owned_string(L, luna_platform_path_normalize(p));
}

static int path_dirname(LunaState* L) {
    const char* p = path_arg(L, 0);
    if (!p) return -1;
    return push_owned_string(L, luna_platform_path_dirname(p));
}

static int path_basename(LunaState* L) {
    const char* p = path_arg(L, 0);
    if (!p) return -1;
    return push_owned_string(L, luna_platform_path_basename(p));
}

static int path_ext(LunaState* L) {
    const char* p = path_arg(L, 0);
    if (!p) return -1;
    const char* ext = luna_platform_path_ext(p);
    luna_pushstring(L, ext ? ext : "");
    return 1;
}

static int path_separator(LunaState* L) {
    return os_separator(L);
}

static void register_io(LunaState* L, LunaTable* exports) {
    lib_export_function(L, exports, "read", io_read);
    lib_export_function(L, exports, "write", io_write);
    lib_export_function(L, exports, "exists", io_exists);
    lib_export_function(L, exports, "size", io_size);
    lib_export_function(L, exports, "is_dir", io_is_dir);
    lib_export_function(L, exports, "mkdir", io_mkdir);
}

static void register_os(LunaState* L, LunaTable* exports) {
    lib_export_function(L, exports, "getenv", os_getenv);
    lib_export_function(L, exports, "cwd", os_cwd);
    lib_export_function(L, exports, "separator", os_separator);
    lib_export_function(L, exports, "name", os_name);
    lib_export_function(L, exports, "run", os_run);
    lib_export_function(L, exports, "system", os_system);
}

static void register_time(LunaState* L, LunaTable* exports) {
    lib_export_function(L, exports, "now", time_now);
    lib_export_function(L, exports, "nanos", time_nanos);
    lib_export_function(L, exports, "mono", time_mono);
    lib_export_function(L, exports, "sleep", time_sleep);
    lib_export_function(L, exports, "since", time_since);
}

static void register_path(LunaState* L, LunaTable* exports) {
    lib_export_function(L, exports, "join", path_join);
    lib_export_function(L, exports, "normalize", path_normalize_fn);
    lib_export_function(L, exports, "dirname", path_dirname);
    lib_export_function(L, exports, "basename", path_basename);
    lib_export_function(L, exports, "ext", path_ext);
    lib_export_function(L, exports, "separator", path_separator);
}

static const LunaModule g_io = { "io_native", register_io, NULL };
static const LunaModule g_os = { "os_native", register_os, NULL };
static const LunaModule g_time = { "time_native", register_time, NULL };
static const LunaModule g_path = { "path_native", register_path, NULL };

void luna_open_system_lib(LunaState* L) {
    luna_register_module(L, &g_io);
    luna_register_module(L, &g_os);
    luna_register_module(L, &g_time);
    luna_register_module(L, &g_path);
}
