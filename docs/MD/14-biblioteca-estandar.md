# 14 — Biblioteca estándar

Luna 0.7.5 incluye una biblioteca estándar disponible al crear el estado. Cada carpeta de `Lib/` contiene varios archivos `.luna`: `mod.luna` actúa como fachada pública y el resto de archivos implementan la funcionalidad en el propio lenguaje.

La capa nativa en C (`*_native`) aporta las operaciones primitivas y es invisible para el código de usuario habitual: los módulos `.luna` las envuelven, las combinan y añaden lógica escrita en Luna.

## Estructura

| Módulo | Archivos |
| --- | --- |
| `math` | `constants.luna`, `core.luna`, `trig.luna`, `stats.luna`, `rng.luna`, `mod.luna` |
| `string` | `core.luna`, `search.luna`, `splitting.luna`, `mod.luna` |
| `list` | `core.luna`, `ops.luna`, `mod.luna` |
| `table` | `core.luna`, `ops.luna`, `mod.luna` |
| `io` | `fs.luna`, `text.luna`, `mod.luna` |
| `os` | `env.luna`, `info.luna`, `mod.luna` |
| `time` | `clock.luna`, `fmt.luna`, `mod.luna` |
| `path` | `ops.luna`, `info.luna`, `mod.luna` |

## Funciones globales

| Función | Descripción |
| --- | --- |
| `len(valor)` | Longitud de strings, listas y tablas. |
| `type(valor)` | Nombre del tipo en minúsculas. |
| `str(valor)` | Representación textual. |
| `int(valor)` | Convierte números, booleanos o strings a entero. |
| `float(valor)` | Convierte números, booleanos o strings a float. |
| `input(prompt)` | Lee una línea de entrada estándar. |

## Funciones añadidas en Luna

| Módulo | Funciones |
| --- | --- |
| `math` | `clamp`, `lerp`, `sign`, `gcd`, `lcm`, `hypotenuse`, `sum`, `mean`, `median`, `sort`, `min_of`, `max_of`, `randint`, `choice`, `shuffle`, `tau`. |
| `string` | `repeat`, `reverse`, `pad_left`, `pad_right`, `contains`, `starts_with`, `ends_with`, `split`, `join`. |
| `list` | `first`, `last`, `is_empty`, `extend`, `slice`, `map`, `filter`, `reduce`, `index_of`, `count`, `unique`. |
| `table` | `get_or`, `set_default`, `update`, `values`, `merge`, `has_all`, `clear`. |
| `io` | `ensure_dir`, `read_bytes`, `read_lines`, `write_lines`, `append`. |
| `os` | `getenv_or`, `is_windows`, `is_linux`, `is_macos`, `separator_char`. |
| `time` | `elapsed`, `format_duration`, `millis`. |
| `path` | `build`, `stem`, `has_ext`, `is_absolute`. |

## Resolución en runtime

`luna_new()` registra automáticamente las rutas `Lib` que encuentra en este orden:

1. `LUNA_HOME/Lib` y `LUNA_HOME/share/luna/Lib`.
2. Cada entrada de `LUNA_PATH` (separada por `:` en Unix y `;` en Windows) y su subdirectorio `Lib`.
3. `Lib` junto al ejecutable, `Lib` en el directorio padre y `share/luna/Lib` en el padre.
4. La ruta relativa `Lib` del directorio de trabajo, como último recurso.

Las rutas duplicadas se ignoran. Desde C también se puede añadir una ruta con `luna_add_path()` (alias de `luna_add_module_path()`).

## Ejemplo

```luna
import math
import string
import list

fn double(x):
    return x * 2

let values = list.map([1, 2, 3], double)
print(math.sum(values))
print(string.join(",", values))
```

Los módulos nativos usan el prefijo `_native`. Por ejemplo, `import math_native` accede directamente a las primitivas implementadas en C; `import math` ofrece la interfaz completa escrita en Luna.

Volver al [índice de documentación](README.md).
