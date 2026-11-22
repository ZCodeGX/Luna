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
let y = 2 ** 3 ** 2   # 2 ** (3 ** 2)
let z = -x ** 2
```

> [!NOTE]
> El ejemplo del EBNF para `not` sugiere `not x == y`. La implementación parsea el operando con la precedencia 3, por lo que `not 1 == 2` se evalúa como `(not 1) == 2`.

## Aritmética

- `int op int`: `+`, `-`, `*` y `%` devuelven `integer`; `/` y `**` devuelven `float`.
- Si alguno de los operandos es `float`, el resultado es `float`.
- Dividir o tomar módulo entre cero produce un error.
- `+` **no** concatena cadenas; usar dos strings con `+` da error de tipo.

```luna
print(7 / 2)    # 3.5
print(7 % 2)    # 1
print(2 ** 10)  # 1024.0
```

## Comparación

- `==` y `!=` funcionan con cualquier par de valores.
- `integer` y `float` se comparan entre sí por valor.
- Las cadenas se comparan por contenido.
- `<`, `<=`, `>` y `>=` aceptan números o cadenas; otros tipos dan error.
- Los objetos gestionados por el GC se comparan por identidad.

## Lógica

`not` devuelve un booleano. `and` y `or` devuelven uno de sus operandos según su valor de verdad, con evaluación de corte corto:

```luna
print(nil or "por defecto")  # por defecto
print(1 and "sí")           # sí
```

## Asignación

Operadores de asignación: `=`, `+=`, `-=`, `*=`, `/=`.

```luna
x = x + 1
x += 1
```

> [!NOTE]
> El parser acepta destinos de asignación como `lista[i]` y `objeto.campo`, pero el compilador actual solo admite identificadores.

Continúa con [control de flujo](05-control-de-flujo.md).
