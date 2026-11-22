# 14 — Biblioteca estándar

Luna 0.7.3 incluye una biblioteca estándar disponible al crear el estado. Los módulos se resuelven desde `Lib/` y también están disponibles como aliases nativos (`math_native`, `string_native`, etc.) para la API C.

## Funciones globales

| Función | Descripción |
| --- | --- |
| `len(valor)` | Longitud de strings, listas y tablas. |
| `type(valor)` | Nombre del tipo en minúsculas. |
| `str(valor)` | Representación textual. |
| `int(valor)` | Convierte números, booleanos o strings a entero. |
| `float(valor)` | Convierte números, booleanos o strings a float. |
| `input(prompt)` | Lee una línea de entrada estándar. |

## Módulos

| Módulo | Funciones principales |
| --- | --- |
| `math` | `sqrt`, `cbrt`, `floor`, `ceil`, `round`, `sin`, `cos`, `pow`, `min`, `max`, `random`, constantes `pi` y `e`. |
| `string` | `upper`, `lower`, `split`, `join`, `replace`, `trim`, `find`, `contains`, `sub`. |
| `list` | `new`, `push`, `pop`, `get`, `set`, `len`, `clear`, `reverse`, `contains`. |
| `table` | `new`, `get`, `set`, `has`, `remove`, `keys`, `len`. |
| `io` | `read`, `write`, `exists`, `size`, `is_dir`, `mkdir`. |
| `os` | `cwd`, `getenv`, `name`, `separator`. |
| `time` | `now`, `nanos`, `mono`, `sleep`, `since`. |
| `path` | `join`, `normalize`, `dirname`, `basename`, `ext`, `separator`. |

## Ejemplo

```luna
import math
import string

let n = int(input("Número: "))
print(math.sqrt(n))
print(string.upper("luna"))
```

Los módulos nativos usan el prefijo `_native`. Por ejemplo, `import math_native` accede directamente a las funciones implementadas en C sin pasar por el wrapper de `Lib/math/mod.luna`.

Volver al [índice de documentación](README.md).
