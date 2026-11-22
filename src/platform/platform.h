#ifndef LUNA_PLATFORM_H
#define LUNA_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================
 * Códigos de retorno
 * ============================================================ */

#define LUNA_PLATFORM_OK        0
#define LUNA_PLATFORM_ERROR    -1
#define LUNA_PLATFORM_NOTFOUND -2
#define LUNA_PLATFORM_NOSUPPORT -3

/* ============================================================
 * Filesystem
 * ============================================================ */

/*
 * Lee un archivo completo en memoria.
 * Devuelve un buffer malloc'd que el llamante debe liberar con free().
 * En caso de error, devuelve NULL y (si error_out no es NULL) escribe
 * el mensaje en error_out (tamaño error_size).
 *
 * Si out_len no es NULL, se escribe la longitud del buffer.
 */
char* luna_platform_read_file(const char* path, size_t* out_len,
                              char* error_out, size_t error_size);

/*
 * Escribe un buffer a un archivo (crea o trunca).
 * Devuelve LUNA_PLATFORM_OK en éxito, LUNA_PLATFORM_ERROR en error.
 */
int luna_platform_write_file(const char* path, const void* data, size_t len,
                             char* error_out, size_t error_size);

/*
 * Verifica si un archivo existe y es accesible.
 * Devuelve 1 si existe, 0 si no.
 */
int luna_platform_file_exists(const char* path);

/*
 * Obtiene el tamaño de un archivo en bytes.
 * Devuelve -1 en error.
 */
int64_t luna_platform_file_size(const char* path);

/*
 * Verifica si una ruta es un directorio.
 * Devuelve 1 si es directorio, 0 si no.
 */
int luna_platform_is_directory(const char* path);

/*
 * Crea un directorio (y padres si create_parents es true).
 * Devuelve LUNA_PLATFORM_OK en éxito.
 */
int luna_platform_mkdir(const char* path, int create_parents);

/* ============================================================
 * Carga dinámica de bibliotecas
 * ============================================================ */

typedef void* LunaDynLib;
typedef void* LunaDynSym;

/*
 * Carga una biblioteca dinámica (.so / .dll / .dylib).
 * Devuelve NULL en error.
 */
LunaDynLib luna_platform_dlopen(const char* path, char* error_out, size_t error_size);

/*
 * Busca un símbolo en una biblioteca cargada.
 * Devuelve NULL si no se encuentra.
 */
LunaDynSym luna_platform_dlsym(LunaDynLib lib, const char* name,
                               char* error_out, size_t error_size);

/*
 * Cierra una biblioteca dinámica.
 */
void luna_platform_dlclose(LunaDynLib lib);

/*
 * Devuelve la extensión nativa de bibliotecas dinámicas
 * (".so", ".dll", ".dylib").
 */
const char* luna_platform_dl_extension(void);

/* ============================================================
 * Paths
 * ============================================================ */

/*
 * Separador de directorios nativo ('/' en POSIX, '\\' en Windows).
 */
char luna_platform_path_separator(void);

/*
 * Une dos componentes de path.
 * Devuelve un string malloc'd que el llamante debe liberar.
 * Ejemplo: join("/usr", "local") → "/usr/local"
 */
char* luna_platform_path_join(const char* a, const char* b);

/*
 * Normaliza un path:
 *   - Reemplaza '\\' por '/' (en todas las plataformas).
 *   - Elimina slashes duplicados.
 *   - Resuelve "." y ".." cuando es posible.
 * Devuelve un string malloc'd.
 */
char* luna_platform_path_normalize(const char* path);

/*
 * Obtiene el directorio de un path.
 * Ejemplo: dirname("/usr/local/bin") → "/usr/local"
 * Devuelve un string malloc'd.
 */
char* luna_platform_path_dirname(const char* path);

/*
 * Obtiene el nombre base de un path.
 * Ejemplo: basename("/usr/local/bin/luna") → "luna"
 * Devuelve un string malloc'd.
 */
char* luna_platform_path_basename(const char* path);

/*
 * Obtiene la extensión de un archivo.
 * Ejemplo: path_ext("foo.luna") → ".luna"
 * Devuelve puntero DENTRO del string original (no malloc'd),
 * o NULL si no tiene extensión.
 */
const char* luna_platform_path_ext(const char* path);

/*
 * Obtiene el directorio de trabajo actual.
 * Devuelve un string malloc'd.
 */
char* luna_platform_getcwd(void);

/* ============================================================
 * Tiempo
 * ============================================================ */

/*
 * Reloj monótono de alta resolución (en nanosegundos).
 * Útil para medir intervalos. No se puede usar como hora de pared.
 */
uint64_t luna_platform_monotonic_ns(void);

/*
 * Hora de pared en nanosegundos desde el epoch (Unix timestamp * 1e9).
 */
uint64_t luna_platform_wall_ns(void);

/*
 * Pausa la ejecución durante al menos `ns` nanosegundos.
 */
void luna_platform_sleep_ns(uint64_t ns);

/* ============================================================
 * Entorno
 * ============================================================ */

/*
 * Obtiene una variable de entorno.
 * Devuelve NULL si no existe.
 * El string devuelto NO debe liberarse (apunta a memoria del proceso).
 */
const char* luna_platform_getenv(const char* name);

#endif /* LUNA_PLATFORM_H */
