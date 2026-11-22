#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "luna/luna.h"
#include "luna/diagnostics.h"

static const char* base_name(const char* path) {
    const char* slash = strrchr(path, '/');
    const char* back = strrchr(path, '\\');
    if (back && (!slash || back > slash)) slash = back;
    return slash ? slash + 1 : path;
}

static void print_usage(const char* prog) {
    printf("Uso: %s [opciones] [archivo.luna] [args...]\n\n", prog);
    printf("Opciones:\n");
    printf("  -c CODIGO      ejecuta CODIGO como programa\n");
    printf("  -              lee el programa desde stdin\n");
    printf("  -h, --help     muestra esta ayuda\n");
    printf("  -V, --version  muestra la versión\n");
    printf("  --no-color     desactiva el color en diagnósticos\n");
    printf("  --color        fuerza el color en diagnósticos\n");
    printf("  --max-errors=N limita el número de errores mostrados (0 = sin límite)\n");
    printf("\nSin argumentos inicia el intérprete interactivo.\n");
}

static void print_error(LunaState* L) {
    const char* msg = luna_last_error(L);
    fprintf(stderr, "luna: %s\n", msg ? msg : "error desconocido");
}

static void set_argv(LunaState* L, int argc, char** argv, int start) {
    luna_newlist(L);
    int list_idx = luna_gettop(L);
    for (int i = start; i < argc; i++) {
        luna_pushstring(L, argv[i]);
        luna_list_push(L, list_idx);
    }
    luna_setglobal(L, "argv");
}

static int run_source(LunaState* L, const char* src, const char* chunkname) {
    LunaStatus st = luna_dostring(L, src, chunkname);
    if (st != LUNA_OK) {
        if (st != LUNA_ERR_SYNTAX) print_error(L);
        return 1;
    }
    return 0;
}

static char* read_stdin(void) {
    size_t cap = 4096;
    size_t len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) return NULL;

    for (;;) {
        if (len + 1 >= cap) {
            size_t new_cap = cap * 2;
            char* tmp = (char*)realloc(buf, new_cap);
            if (!tmp) { free(buf); return NULL; }
            buf = tmp;
            cap = new_cap;
        }
        size_t n = fread(buf + len, 1, cap - len - 1, stdin);
        len += n;
        if (n == 0) break;
    }
    buf[len] = '\0';
    return buf;
}

static int run_repl(LunaState* L) {
    char line[8192];
    for (;;) {
        fputs(">>> ", stdout);
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) {
            putchar('\n');
            return 0;
        }
        if (line[0] == '\0') continue;
        run_source(L, line, "<stdin>");
        luna_clear_error(L);
    }
}

int main(int argc, char** argv) {
    const char* prog = base_name(argc > 0 ? argv[0] : "luna");
    bool color = isatty(STDERR_FILENO) && !getenv("NO_COLOR");
    int first = 1;

    for (; first < argc; first++) {
        const char* arg = argv[first];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_usage(prog);
            return 0;
        }
        if (strcmp(arg, "-V") == 0 || strcmp(arg, "--version") == 0) {
            printf("Luna %s\n", LUNA_VERSION_STRING);
            return 0;
        }
        if (strcmp(arg, "--no-color") == 0) {
            color = false;
            continue;
        }
        if (strcmp(arg, "--color") == 0) {
            color = true;
            continue;
        }
        if (strncmp(arg, "--max-errors=", 13) == 0) {
            const char* value = arg + 13;
            char* end = NULL;
            errno = 0;
            long v = strtol(value, &end, 10);
            if (end == value || *end != '\0' || v < 0 ||
                errno == ERANGE || v > INT_MAX) {
                fprintf(stderr, "luna: --max-errors requiere un entero >= 0\n");
                return 1;
            }
            luna_diag_set_max_errors((int)v);
            continue;
        }
        break;
    }

    luna_diag_set_color(color);

    LunaRuntime* rt = luna_runtime_new(NULL);
    LunaState* L = luna_new(rt);
    if (!L) {
        fprintf(stderr, "luna: no se pudo crear el estado\n");
        luna_runtime_free(rt);
        return 1;
    }

    int result = 0;

    if (first >= argc) {
        result = run_repl(L);
    } else if (strcmp(argv[first], "-c") == 0) {
        if (first + 1 >= argc) {
            fprintf(stderr, "%s: -c requiere una cadena de código\n", prog);
            result = 1;
        } else {
            set_argv(L, argc - first, argv + first, 1);
            result = run_source(L, argv[first + 1], "<string>");
        }
    } else if (strcmp(argv[first], "-") == 0) {
        char* src = read_stdin();
        if (!src) {
            fprintf(stderr, "luna: error leyendo stdin\n");
            result = 1;
        } else {
            set_argv(L, argc - first, argv + first, 1);
            result = run_source(L, src, "<stdin>");
            free(src);
        }
    } else {
        set_argv(L, argc - first, argv + first, 1);
        LunaStatus st = luna_dofile(L, argv[first]);
        if (st != LUNA_OK) {
            if (st != LUNA_ERR_SYNTAX) print_error(L);
            result = 1;
        }
    }

    luna_close(L);
    luna_runtime_free(rt);
    return result;
}
