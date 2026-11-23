# 08 — Manejo de errores

## throw

`throw` acepta una expresión. Si el valor lanzado es una cadena, se usa como mensaje del error. Una excepción no capturada termina la ejecución y se informa como error no manejado.

## try / catch

La sintaxis es:

```luna
try:
    throw "boom"
catch e:
    print("capturado:", e)

print("después")
```

Salida:

```text
capturado: boom
después
```

Tanto `try` como `catch` terminan en `:`; el nombre después de `catch` recibe el valor lanzado. El compilador emite `OP_SETUP_EXC`, `OP_POP_EXC` y `OP_GET_EXC`. Cada frame mantiene una pila de manejadores (hasta 32 niveles anidados); `OP_THROW` propaga el valor a través de los frames.

Los errores de runtime se convierten en excepciones capturables, incluida la división entre cero:

```luna
try:
    print(1 / 0)
catch e:
    print("capturado:", e)

print("después")
```

Salida:

```text
capturado: division by zero
después
```

Este es el comportamiento verificado en `tests/regression/exceptions.luna`. Un error lanzado dentro de una función se propaga al `try` de la función llamadora, como se ve en el [primer proyecto](16-primer-proyecto.md).

## Errores de FFI

Las llamadas FFI fallan con mensajes como `ffi: invalid signature '...'`, `ffi: cannot load '...'` o `ffi: mixed float/integer arguments are not supported`. Se pueden capturar con `try`/`catch` igual que cualquier otro error de runtime:

```luna
import ffi

try:
    ffi.declare("no_existe", "c", "int(int)")
catch e:
    print("capturado:", e)
```

El mensaje concreto depende de la plataforma y de la biblioteca; consulta [FFI](15-ffi.md).

## Estados de error

Los estados públicos están definidos en `LunaStatus` (`include/luna/luna_types.h`):

| Estado | Significado |
| --- | --- |
| `LUNA_OK` | Sin error |
| `LUNA_ERR_SYNTAX` | Error de sintaxis |
| `LUNA_ERR_RUNTIME` | Error en tiempo de ejecución |
| `LUNA_ERR_TYPE` | Error de tipo |
| `LUNA_ERR_MEMORY` | Sin memoria |
| `LUNA_ERR_MODULE` | Error de módulo |
| `LUNA_ERR_NATIVE` | Error en código nativo |
| `LUNA_ERR_ARGUMENT` | Argumento inválido |
| `LUNA_ERR_IO` | Error de entrada/salida |
| `LUNA_ERR_LIMIT` | Límite excedido |
| `LUNA_ERR_DIVZERO` | División entre cero |
| `LUNA_ERR_INDEX` | Índice fuera de rango |
| `LUNA_ERR_KEY` | Clave no encontrada |
| `LUNA_ERR_ASSERT` | Aserción fallida |
| `LUNA_ERR_USER` | Error definido por el usuario |

Desde la API C se consultan con `luna_status`, `luna_last_error`, `luna_last_error_line`, `luna_last_error_file` y `luna_last_traceback`. La CLI imprime los errores como `luna: <mensaje>` en stderr y termina con código 1. Para el formato de diagnósticos, consulta [diagnósticos](13-diagnosticos.md).

Continúa con [CLI y build](09-cli-y-build.md).
