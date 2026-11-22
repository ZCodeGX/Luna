#ifndef LUNA_H
#define LUNA_H

#include <stddef.h>
#include <stdint.h>
#include "luna_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LUNA_VERSION_MAJOR 0
#define LUNA_VERSION_MINOR 2
#define LUNA_VERSION_PATCH 6
#define LUNA_VERSION_STRING "0.2.6"
#define LUNA_ABI_VERSION   1

#if defined(_WIN32) && defined(LUNA_BUILD_SHARED)
#  define LUNA_API __declspec(dllexport)
#elif defined(_WIN32) && defined(LUNA_USE_SHARED)
#  define LUNA_API __declspec(dllimport)
#else
#  define LUNA_API
#endif

/* ---------- Tipos opacos ---------- */
typedef struct LunaRuntime LunaRuntime;
struct LunaTable;

/* ---------- Función nativa ----------
 * Retorno >= 0: número de resultados en el top de la pila.
 * Retorno <  0: error. El mensaje debe estar en el top de la pila.
 * NUNCA lanzar excepciones C++ a través de esta frontera.
 */
/* ---------- Runtime (configuración, opcional) ---------- */
LUNA_API LunaRuntime *luna_runtime_new(const LunaAllocator *allocator);
LUNA_API void         luna_runtime_free(LunaRuntime *rt);
LUNA_API void         luna_runtime_set_memory_limit(LunaRuntime *rt, size_t bytes);
LUNA_API void         luna_runtime_set_instruction_limit(LunaRuntime *rt, uint64_t n);

/* ---------- Estado ---------- */
LUNA_API LunaState   *luna_new(LunaRuntime *rt);   /* rt puede ser NULL */
LUNA_API void         luna_close(LunaState *L);

/* ---------- Ejecución (protegida) ---------- */
LUNA_API LunaStatus   luna_dofile  (LunaState *L, const char *path);
LUNA_API LunaStatus   luna_dostring(LunaState *L, const char *src, const char *chunkname);
LUNA_API LunaStatus   luna_call    (LunaState *L, int nargs, int nresults);
LUNA_API LunaStatus   luna_pcall   (LunaState *L, int nargs, int nresults, int errfunc);

/* ---------- Errores ---------- */
LUNA_API LunaStatus   luna_status          (LunaState *L);
LUNA_API const char  *luna_last_error      (LunaState *L);
LUNA_API const char  *luna_last_error_file (LunaState *L);
LUNA_API int          luna_last_error_line (LunaState *L);
LUNA_API const char  *luna_last_traceback  (LunaState *L);
LUNA_API void         luna_clear_error     (LunaState *L);

/* ---------- Pila ---------- */
LUNA_API int  luna_gettop  (LunaState *L);
LUNA_API void luna_settop  (LunaState *L, int index);
LUNA_API void luna_pushvalue(LunaState *L, int index);
LUNA_API void luna_pop     (LunaState *L, int n);
LUNA_API void luna_remove  (LunaState *L, int index);
LUNA_API void luna_insert  (LunaState *L, int index);
LUNA_API void luna_replace (LunaState *L, int index);
LUNA_API void luna_xmove   (LunaState *from, LunaState *to, int n);

/* ---------- Tipo ---------- */
LUNA_API LunaType     luna_type      (LunaState *L, int index);
LUNA_API const char  *luna_typename  (LunaType t);
LUNA_API int          luna_isnil     (LunaState *L, int index);
LUNA_API int          luna_isboolean (LunaState *L, int index);
LUNA_API int          luna_isinteger (LunaState *L, int index);
LUNA_API int          luna_isfloat   (LunaState *L, int index);
LUNA_API int          luna_isnumber  (LunaState *L, int index);
LUNA_API int          luna_isstring  (LunaState *L, int index);
LUNA_API int          luna_islist    (LunaState *L, int index);
LUNA_API int          luna_istable   (LunaState *L, int index);
LUNA_API int          luna_isfunction(LunaState *L, int index);
LUNA_API int          luna_isuserdata(LunaState *L, int index);

/* ---------- Push ---------- */
LUNA_API void  luna_pushnil       (LunaState *L);
LUNA_API void  luna_pushboolean   (LunaState *L, int b);
LUNA_API void  luna_pushinteger   (LunaState *L, int64_t n);
LUNA_API void  luna_pushfloat     (LunaState *L, double n);
LUNA_API void  luna_pushstring    (LunaState *L, const char *s);
LUNA_API void  luna_pushlstring   (LunaState *L, const char *s, size_t len);
LUNA_API void  luna_pushcfunction (LunaState *L, LunaCFunction fn);
LUNA_API void  luna_pushcclosure  (LunaState *L, LunaCFunction fn, int nupvalues);
LUNA_API void  luna_pushlightuserdata(LunaState *L, void *p);
LUNA_API void *luna_newuserdata   (LunaState *L, size_t size);
LUNA_API void *luna_newuserdata_finalized(LunaState *L, size_t size, LunaFinalizer fin);

/* ---------- Get ---------- */
LUNA_API int      luna_toboolean(LunaState *L, int index);
LUNA_API int64_t  luna_tointeger(LunaState *L, int index, int *ok);
LUNA_API double   luna_tonumber (LunaState *L, int index, int *ok);
LUNA_API const char *luna_tostring(LunaState *L, int index, size_t *len);
LUNA_API void    *luna_touserdata(LunaState *L, int index);

/* ---------- Check (error si tipo incorrecto, vía status) ---------- */
LUNA_API int64_t     luna_checkinteger (LunaState *L, int index);
LUNA_API double      luna_checknumber  (LunaState *L, int index);
LUNA_API const char *luna_checkstring  (LunaState *L, int index, size_t *len);
LUNA_API void       *luna_checkuserdata(LunaState *L, int index);
LUNA_API int         luna_checkboolean (LunaState *L, int index);

/* ---------- Tablas ---------- */
LUNA_API void    luna_newtable (LunaState *L);
LUNA_API void    luna_newlist  (LunaState *L);
LUNA_API void    luna_gettable (LunaState *L, int index);   /* pop key, push value */
LUNA_API void    luna_settable (LunaState *L, int index);   /* pop key y value */
LUNA_API void    luna_getfield (LunaState *L, int index, const char *k);
LUNA_API void    luna_setfield (LunaState *L, int index, const char *k);
LUNA_API void    luna_getindex (LunaState *L, int index, int64_t i);
LUNA_API void    luna_setindex (LunaState *L, int index, int64_t i);
LUNA_API int64_t luna_len      (LunaState *L, int index);

/* ---------- Listas ---------- */
LUNA_API void luna_list_push(LunaState *L, int index); /* mueve el top a la lista */

/* ---------- Referencias (seguras frente al GC) ---------- */
LUNA_API int  luna_ref  (LunaState *L, int index);  /* devuelve id > 0 */
LUNA_API void luna_unref(LunaState *L, int ref);
LUNA_API void luna_getref(LunaState *L, int ref);

/* ---------- Globales ---------- */
LUNA_API void luna_getglobal(LunaState *L, const char *name);
LUNA_API void luna_setglobal(LunaState *L, const char *name);

/* ---------- Upvalues (para closures nativos) ---------- */
LUNA_API int  luna_getupvalue(LunaState *L, int funcindex, int n); /* push upvalue, devuelve tipo */

/* ---------- Registro de funciones y módulos ---------- */
typedef struct LunaModule {
    const char *name;
    void (*register_functions)(LunaState *L, struct LunaTable *exports);
    void (*shutdown)(LunaState *L); /* opcional */
} LunaModule;

LUNA_API void luna_register_function(LunaState *L, const char *name, LunaCFunction fn);
LUNA_API void luna_register_module  (LunaState *L, const LunaModule *mod);

#ifdef __cplusplus
}
#endif

#endif /* LUNA_H */