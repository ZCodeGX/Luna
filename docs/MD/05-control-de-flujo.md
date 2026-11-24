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

La condición se evalúa antes de cada iteración. Usa `break` para salir y `continue` para saltar a la siguiente iteración.

## for ... in

```luna
for x in [10, 20, 30]:
    print(x)

for clave, valor in {uno = 1}:
    print(clave, valor)
```

Una sola variable recibe el elemento de una lista o la clave de una tabla; dos variables reciben clave y valor. En tablas, el orden de iteración sigue el almacenamiento interno y no está garantizado.

## Construcciones no compiladas

| Construcción | Parser | Compilador |
| --- | --- | --- |
| `+` entre strings | Sí | Error de tipo en runtime |

Usa `string.concat` o `string.join` para unir cadenas. Consulta el [estado de implementación](11-estado-de-implementacion.md) y las [recetas](17-recetas.md).

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
