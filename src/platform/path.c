#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "platform.h"
#include "platform_internal.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef _WIN32
#include <unistd.h>
#endif

char luna_platform_path_separator(void) {
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

/* Normaliza separadores in-place: reemplaza '\\' por '/' */
static void normalize_seps(char* s) {
    for (char* p = s; *p; p++) {
        if (*p == '\\') *p = '/';
    }
}

char* luna_platform_path_join(const char* a, const char* b) {
    if (!a || !a[0]) return platform_strdup(b ? b : "");
    if (!b || !b[0]) return platform_strdup(a);

    size_t la = strlen(a);
    size_t lb = strlen(b);

    /* ¿'a' ya termina en separador? */
    char last = a[la - 1];
    int has_sep = (last == '/' || last == '\\');

    /* ¿'b' empieza con separador? */
    int b_has_sep = (b[0] == '/' || b[0] == '\\');

    size_t total;
    char sep = luna_platform_path_separator();

    if (has_sep && b_has_sep) {
        /* "a/" + "/b" → "a/b" (quitar uno) */
        total = la + lb;  /* uno de los dos se salta */
        char* out = (char*)malloc(total + 1);
        if (!out) return NULL;
        memcpy(out, a, la - 1);
        out[la - 1] = sep;
        memcpy(out + la, b + 1, lb);
        out[total] = '\0';
        return out;
    } else if (has_sep || b_has_sep) {
        /* Ya hay un separador entre ambos */
        total = la + lb;
        char* out = (char*)malloc(total + 1);
        if (!out) return NULL;
        memcpy(out, a, la);
        if (b_has_sep) {
            out[la] = sep;
            memcpy(out + la + 1, b + 1, lb - 1);
        } else {
            memcpy(out + la, b, lb);
        }
        out[total] = '\0';
        return out;
    } else {
        /* Ninguno tiene separador: añadir uno */
        total = la + 1 + lb;
        char* out = (char*)malloc(total + 1);
        if (!out) return NULL;
        memcpy(out, a, la);
        out[la] = sep;
        memcpy(out + la + 1, b, lb);
        out[total] = '\0';
        return out;
    }
}

char* luna_platform_path_normalize(const char* path) {
    if (!path) return platform_strdup("");

    char* tmp = platform_strdup(path);
    if (!tmp) return NULL;
    normalize_seps(tmp);

    /* Resolver "." y ".." */
    /* Dividir en componentes */
    char* parts[256];
    int nparts = 0;

    char* saveptr = NULL;
    char* tok = strtok_r(tmp, "/", &saveptr);
    int is_absolute = (path[0] == '/' || path[0] == '\\') ||
                      (strlen(path) >= 2 && isalpha((unsigned char)path[0]) && path[1] == ':');

    while (tok && nparts < 256) {
        if (strcmp(tok, ".") == 0) {
            /* Ignorar */
        } else if (strcmp(tok, "..") == 0) {
            if (nparts > 0 && strcmp(parts[nparts - 1], "..") != 0) {
                nparts--;
            } else if (!is_absolute) {
                parts[nparts++] = tok;
            }
            /* Si es absoluto y no hay nada que subir, ignorar */
        } else if (tok[0] != '\0') {
            parts[nparts++] = tok;
        }
        tok = strtok_r(NULL, "/", &saveptr);
    }

    /* Reconstruir */
    size_t total = 0;
    if (is_absolute) total = 1;  /* '/' inicial */
    for (int i = 0; i < nparts; i++) {
        total += strlen(parts[i]) + 1;  /* +1 por '/' */
    }
    if (total == 0) total = 2;  /* mínimo "." */

    char* out = (char*)malloc(total + 1);
    if (!out) { free(tmp); return NULL; }

    size_t pos = 0;
    if (is_absolute) {
        out[pos++] = '/';
    }

    if (nparts == 0) {
        if (!is_absolute) {
            out[0] = '.';
            out[1] = '\0';
        } else {
            out[0] = '\0';
        }
    } else {
        for (int i = 0; i < nparts; i++) {
            size_t plen = strlen(parts[i]);
            memcpy(out + pos, parts[i], plen);
            pos += plen;
            if (i < nparts - 1) {
                out[pos++] = '/';
            }
        }
        out[pos] = '\0';
    }

    free(tmp);
    return out;
}

char* luna_platform_path_dirname(const char* path) {
    if (!path || !path[0]) return platform_strdup(".");

    char* tmp = platform_strdup(path);
    if (!tmp) return NULL;
    normalize_seps(tmp);

    /* Quitar trailing slashes */
    size_t len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') {
        tmp[--len] = '\0';
    }

    /* Buscar último '/' */
    char* last_sep = strrchr(tmp, '/');
    if (!last_sep) {
        free(tmp);
        return platform_strdup(".");
    }

    if (last_sep == tmp) {
        /* "/algo" → "/" */
        free(tmp);
        return platform_strdup("/");
    }

    *last_sep = '\0';
    char* result = platform_strdup(tmp);
    free(tmp);
    return result;
}

char* luna_platform_path_basename(const char* path) {
    if (!path || !path[0]) return platform_strdup("");

    char* tmp = platform_strdup(path);
    if (!tmp) return NULL;
    normalize_seps(tmp);

    /* Quitar trailing slashes */
    size_t len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') {
        tmp[--len] = '\0';
    }

    char* last_sep = strrchr(tmp, '/');
    const char* base = last_sep ? last_sep + 1 : tmp;

    char* result = platform_strdup(base);
    free(tmp);
    return result;
}

const char* luna_platform_path_ext(const char* path) {
    if (!path) return NULL;

    const char* base = strrchr(path, '/');
    const char* base2 = strrchr(path, '\\');
    if (base2 && (!base || base2 > base)) base = base2;
    if (base) {
        path = base + 1;
    }

    const char* dot = strrchr(path, '.');
    if (!dot || dot == path) return NULL;  /* sin extensión, o empieza con "." */
    return dot;
}

char* luna_platform_getcwd(void) {
    char buf[4096];
#ifdef _WIN32
    if (!_getcwd(buf, sizeof(buf))) return NULL;
#else
    if (!getcwd(buf, sizeof(buf))) return NULL;
#endif
    return platform_strdup(buf);
}
