# 05 — Control de flujo

Las cabeceras de bloque terminan siempre en `:`. El cuerpo se indenta con espacios.

## if / else if / else

Los condicionales usan la verdad del programa: solo `nil` y `false` son falsy.

```luna
let x = 15

if x > 20:
    print("grande")
else if x > 10:
    print("mediano")
else:
    print("pequeño")
```

Salida:

```text
mediano
```

Los `else if` se recogen en una lista plana en el AST, no como condicionales anidados.

## while

```luna
let n = 3

while n > 0:
    print(n)
    n = n - 1
```

Salida:

```text
3
2
1
```

La condición se evalúa antes de cada iteración. No hay un `break` compilado: si necesitas salir, controla la condición del `while`.

## Construcciones no compiladas

| Construcción | Parser | Compilador |
| --- | --- | --- |
| `for ... in` | Sí | Emite `for loops not yet implemented` |
| `break`, `continue` | Sí | No genera código |
| Lambdas anónimas `fn(...)` | Sí | Nodo `NODE_LAMBDA` sin compilar |
| `+` entre strings | Sí | Error de tipo en runtime |

Un `for` o un `break` en código aparentemente válido produce un error en tiempo de compilación o ejecución. La alternativa es `while`, como en el ejemplo anterior. Consulta el [estado de implementación](11-estado-de-implementacion.md) y las [recetas](17-recetas.md).

## try / catch

Los bloques con manejo de errores también exigen `:` en `try` y en `catch`:

```luna
try:
    print(1 / 0)
catch error:
    print("capturado:", error)
```

Salida:

```text
capturado: division by zero
```

La explicación completa está en [manejo de errores](08-manejo-de-errores.md).

Continúa con [funciones](06-funciones.md).
