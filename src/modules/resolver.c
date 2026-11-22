#include "modules_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* strdup portable (no POSIX) — MOVIDO AL PRINCIPIO */
static char* strdup_safe(const char* s) {
    size_t len = strlen(s);
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len + 1);
    return out;
}

/* ---------- Helpers de paths ---------- */

static int file_exists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* Convierte "a.b.c" en "a/b/c" (reemplaza puntos por separadores) */
static char* name_to_path(const char* name) {
    size_t len = strlen(name);
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    for (size_t i = 0; i <= len; i++) {
        out[i] = (name[i] == '.') ? '/' : name[i];
    }
    return out;
}

/* Concatena dir + "/" + file en buffer. Devuelve 0 si cabe, -1 si no. */
static int join_path(char* out, size_t out_size, const char* dir, const char* file) {
    size_t dlen = strlen(dir);
    size_t flen = strlen(file);
    /* +2 por el '/' y el '\0' */
    if (dlen + 1 + flen + 1 > out_size) return -1;
    
    memcpy(out, dir, dlen);
    out[dlen] = '/';
    memcpy(out + dlen + 1, file, flen + 1);
    return 0;
}

/* Obtiene el directorio de un path (modifica el buffer in-place) */
static void dirname_inplace(char* path) {
    char* last_sep = NULL;
    for (char* p = path; *p; p++) {
        if (*p == '/') last_sep = p;
    }
    if (last_sep) {
        *last_sep = '\0';
    } else {
        /* Sin directorio: usar "." */
        path[0] = '.';
        path[1] = '\0';
    }
}

/* ---------- Resolución ---------- */

/*
 * Intenta encontrar el archivo para un módulo dado:
 *   1. <base>/<name_con_puntos_a_slashes>/mod.luna
 *   2. <base>/<name_con_puntos_a_slashes>.luna
 *
 * Devuelve ruta absoluta malloc'd, o NULL si no se encuentra.
 */
static char* try_resolve_in(const char* base, const char* name) {
    char* name_path = name_to_path(name);
    if (!name_path) return NULL;
    
    char buf[LUNA_MAX_PATH_LEN];
    
    /* Opción 1: <base>/<name>/mod.luna */
    if (join_path(buf, sizeof(buf), base, name_path) == 0) {
        char mod_path[LUNA_MAX_PATH_LEN];
        if (join_path(mod_path, sizeof(mod_path), buf, "mod.luna") == 0) {
            if (file_exists(mod_path)) {
                free(name_path);
                return strdup_safe(mod_path);
            }
        }
    }
    
    /* Opción 2: <base>/<name>.luna */
    {
        char with_ext[LUNA_MAX_PATH_LEN];
        size_t nplen = strlen(name_path);
        if (nplen + 5 + 1 <= sizeof(with_ext)) {  /* ".luna" + '\0' */
            memcpy(with_ext, name_path, nplen);
            memcpy(with_ext + nplen, ".luna", 6);
            
            char full_path[LUNA_MAX_PATH_LEN];
            if (join_path(full_path, sizeof(full_path), base, with_ext) == 0) {
                if (file_exists(full_path)) {
                    free(name_path);
                    return strdup_safe(full_path);
                }
            }
        }
    }
    
    free(name_path);
    return NULL;
}

char* modules_resolve(LunaModules* M, const char* name, const char* from_dir) {
    /* 1. Buscar en paths del usuario (en orden) */
    for (int i = 0; i < M->num_paths; i++) {
        char* found = try_resolve_in(M->paths[i], name);
        if (found) return found;
    }
    
    /* 2. Buscar relativo al directorio del archivo actual */
    if (from_dir && from_dir[0]) {
        char* found = try_resolve_in(from_dir, name);
        if (found) return found;
    }
    
    /* 3. Buscar en current_dir del estado */
    if (M->current_dir[0]) {
        char* found = try_resolve_in(M->current_dir, name);
        if (found) return found;
    }
    
    return NULL;
}

/* ---------- Utilidades expuestas ---------- */

void modules_set_current_dir(LunaModules* M, const char* file_path) {
    if (!file_path) {
        M->current_dir[0] = '\0';
        return;
    }
    size_t len = strlen(file_path);
    if (len >= LUNA_MAX_PATH_LEN) len = LUNA_MAX_PATH_LEN - 1;
    memcpy(M->current_dir, file_path, len);
    M->current_dir[len] = '\0';
    dirname_inplace(M->current_dir);
}