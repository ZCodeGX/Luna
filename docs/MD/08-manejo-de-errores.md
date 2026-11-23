# 08 — Manejo de errores

## throw

```luna
throw "algo salió mal"
```

`throw` acepta una expresión. El compilador genera el opcode `OP_THROW`; si el valor lanzado es una cadena, se usa como mensaje del error. Una excepción no capturada termina la ejecución y se informa como error no manejado.

## try / catch

Sintaxis soportada:

```luna
try:
    operacion_riesgosa()
catch error:
    print("falló:", error)
```

El compilador emite `OP_SETUP_EXC`, `OP_POP_EXC` y `OP_GET_EXC`. Cada frame mantiene una pila de manejadores (hasta 32 niveles anidados); `OP_THROW` propaga el valor a través de los frames y `catch` lo recibe en la variable indicada.

Los errores de runtime se convierten en excepciones lanzables: por ejemplo, `1 / 0` dentro de un `try` se captura como `"division by zero"`. El valor lanzado se conserva en `L->thrown_value` y es una raíz para el GC.

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

Desde la API C se consultan con `luna_status`, `luna_last_error`, `luna_last_error_line`, `luna_last_error_file` y `luna_last_traceback`. La CLI imprime los errores como `luna: <mensaje>` en stderr y termina con código 1.

Continúa con [CLI y build](09-cli-y-build.md).
