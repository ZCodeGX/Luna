# 10 — API C

## Encabezados

- `include/luna/luna.h` — API C principal.
- `include/luna/luna_types.h` — tipos y enums públicos.
- `include/luna/luna.hpp` — wrapper C++.

## Versiones

```c
#define LUNA_VERSION_MAJOR 0
#define LUNA_VERSION_MINOR 7
#define LUNA_VERSION_PATCH 0
#define LUNA_VERSION_STRING "0.7.0"
#define LUNA_ABI_VERSION   1
```

`LUNA_API` controla la exportación/importación de símbolos en Windows mediante `LUNA_BUILD_SHARED` y `LUNA_USE_SHARED`.

## Tipos

| Tipo | Descripción |
| --- | --- |
| `LunaStatus` | Estado de una operación (ver documento 08) |
| `LunaType` | `LUNA_TNIL`, `LUNA_TBOOLEAN`, `LUNA_TINTEGER`, `LUNA_TFLOAT`, `LUNA_TSTRING`, `LUNA_TLIST`, `LUNA_TTABLE`, `LUNA_TFUNCTION`, `LUNA_TUSERDATA` |
| `LunaState` | Estado opaco del intérprete |
| `LunaRuntime` | Runtime compartido |
| `LunaTable` | Tabla opaca |
| `LunaAllocator` | `{ void *ud; LunaAllocFn alloc; }` |
| `LunaCFunction` | `int (LunaState *L)` |
| `LunaFinalizer` | `void (LunaState *L, void *ptr)` |
| `LunaModule` | `{ name, register_functions, shutdown }` |

## Ejemplo mínimo

```c
#include <luna/luna.h>

int main(void) {
    LunaState *L = luna_new(NULL);
    luna_dostring(L, "print(1 + 2)", "=ejemplo");
    luna_close(L);
    return 0;
}
```

## Gestión del runtime y del estado

```c
LunaRuntime *luna_runtime_new(const LunaAllocator *allocator);
void  luna_runtime_free(LunaRuntime *runtime);
void  luna_runtime_set_memory_limit(LunaRuntime *runtime, size_t limit);
void  luna_runtime_set_instruction_limit(LunaRuntime *runtime, uint64_t limit);

LunaState *luna_new(LunaRuntime *runtime);
void       luna_close(LunaState *L);
```

`luna_new(NULL)` crea un runtime propio.

## Ejecución

```c
LunaStatus luna_dofile(LunaState *L, const char *path);
LunaStatus luna_dostring(LunaState *L, const char *src, const char *name);
LunaStatus luna_call(LunaState *L, int nargs, int nresults);
LunaStatus luna_pcall(LunaState *L, int nargs, int nresults, int errfunc);
```

## Errores

```c
LunaStatus  luna_status(LunaState *L);
const char *luna_last_error(LunaState *L);
const char *luna_last_error_file(LunaState *L);
int         luna_last_error_line(LunaState *L);
const char *luna_last_traceback(LunaState *L);
void        luna_clear_error(LunaState *L);
```

## Pila

```c
int  luna_gettop(LunaState *L);
void luna_settop(LunaState *L, int index);
void luna_pushvalue(LunaState *L, int index);
void luna_pop(LunaState *L, int count);
void luna_remove(LunaState *L, int index);
void luna_insert(LunaState *L, int index);
void luna_replace(LunaState *L, int index);
void luna_xmove(LunaState *from, LunaState *to, int count);
```

## Tipos y conversión

```c
LunaType    luna_type(LunaState *L, int index);
const char *luna_typename(LunaType type);

int luna_isnil(LunaState *L, int index);
int luna_isboolean(LunaState *L, int index);
int luna_isinteger(LunaState *L, int index);
int luna_isfloat(LunaState *L, int index);
int luna_isnumber(LunaState *L, int index);
int luna_isstring(LunaState *L, int index);
int luna_islist(LunaState *L, int index);
int luna_istable(LunaState *L, int index);
int luna_isfunction(LunaState *L, int index);
int luna_isuserdata(LunaState *L, int index);
```

Empujar valores: `luna_pushnil`, `luna_pushboolean`, `luna_pushinteger`, `luna_pushfloat`, `luna_pushstring`, `luna_pushlstring`, `luna_pushcfunction`, `luna_pushcclosure`, `luna_pushlightuserdata`, `luna_newuserdata`, `luna_newuserdata_finalized`.

Leer valores: `luna_toboolean`, `luna_tointeger`, `luna_tonumber`, `luna_tostring`, `luna_touserdata`.

Comprobación con error automático: `luna_checkinteger`, `luna_checknumber`, `luna_checkstring`, `luna_checkuserdata`, `luna_checkboolean`.

## Tablas, listas y referencias

```c
void   luna_newtable(LunaState *L);
void   luna_newlist(LunaState *L);
void   luna_gettable(LunaState *L, int index);
void   luna_settable(LunaState *L, int index);
void   luna_getfield(LunaState *L, int index, const char *name);
void   luna_setfield(LunaState *L, int index, const char *name);
void   luna_getindex(LunaState *L, int index, int64_t i);
void   luna_setindex(LunaState *L, int index, int64_t i);
int64_t luna_len(LunaState *L, int index);
void   luna_list_push(LunaState *L, int index);

int         luna_ref(LunaState *L, int index);
void        luna_unref(LunaState *L, int ref);
void        luna_getref(LunaState *L, int ref);
void        luna_getglobal(LunaState *L, const char *name);
void        luna_setglobal(LunaState *L, const char *name);
int         luna_getupvalue(LunaState *L, int funcindex, int n);
```

## Registro

```c
void luna_register_function(LunaState *L, const char *name, LunaCFunction fn);
void luna_register_module(LunaState *L, const LunaModule *module);
```

Una función nativa devuelve cuántos resultados dejó en la pila; un negativo indica error con el mensaje en el tope. Nunca se deben propagar excepciones C++ a través de la API C.

## Wrapper C++

`luna.hpp` define el namespace `luna`, la clase `Error : std::runtime_error` con `status()`, y la clase `State` (move-only) con `raw()`, `load()`, `do_string()`, `call<R>()`, `call_void()`, `register_function()`, `set_global()`, `get_global<T>()`, `push()` y `get<T>()`. Las excepciones C++ se convierten en errores Luna mediante un trampolín.

Continúa con [estado de implementación](11-estado-de-implementacion.md).
