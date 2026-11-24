# 04 — Operadores

## Precedencia

De menor a mayor prioridad:

| Nivel | Operadores | Asociatividad |
| --- | --- | --- |
| 1 | `or` | izquierda |
| 2 | `and` | izquierda |
| 3 | `not` | prefijo |
| 4 | `==` `!=` `<` `>` `<=` `>=` | izquierda |
| 5 | `+` `-` | izquierda |
| 6 | `*` `/` `%` | izquierda |
| 7 | `**`, unario `-` | derecha |

```luna
print(2 ** 3 ** 2)
print(-7)
```

Salida:

```text
512
-7
```

> [!NOTE]
> El operando de `not` se analiza con su propia precedencia: `not 1 == 2` se evalúa como `(not 1) == 2`.

## Aritmética

- `int op int`: `+`, `-`, `*` y `%` devuelven `integer`; `/` y `**` devuelven `float`.
- Si alguno de los operandos es `float`, el resultado es `float`.
- Dividir o tomar módulo entre cero produce un error capturable.
- El desbordamiento de enteros produce un error capturable (no hay wraparound).
- `+` **no** concatena cadenas; usar dos strings con `+` da error de tipo. Para unir strings, usa `string.concat` o `string.join`.

```luna
print(7 + 2)
print(7 - 2)
print(7 * 2)
print(7 / 2)
print(7 % 2)
print(2 ** 10)
```

Salida:

```text
9
5
14
3.5
1
1024
```

## Comparación

- `==` y `!=` funcionan con cualquier par de valores.
- `integer` y `float` se comparan entre sí por valor.
- Las cadenas se comparan por contenido.
- `<`, `<=`, `>` y `>=` aceptan números o cadenas; otros tipos dan error.
- Los objetos gestionados por el GC se comparan por identidad.

```luna
print(1 == 1.0)
print("a" < "b")
print(2 <= 2)
```

Salida:

```text
true
true
true
```

## Lógica

`not` devuelve un booleano. `and` y `or` devuelven uno de sus operandos según su valor de verdad, con evaluación de corte corto:

```luna
print(nil or "por defecto")
print(1 and "sí")
print(not false)
```

Salida:

```text
por defecto
sí
true
```

## Asignación

Operadores de asignación: `=`, `+=`, `-=`, `*=`, `/=`. Los destinos admitidos son identificadores, índices de lista o tabla y campos de tabla:

```luna
let x = 1
x = x + 1
x += 1
print(x)

let datos = [1, 2]
datos[0] += 5
print(datos[0])

let cfg = { activo = false }
cfg.activo = true
print(cfg.activo)
```

Salida:

```text
3
6
true
```

Continúa con [control de flujo](05-control-de-flujo.md).
