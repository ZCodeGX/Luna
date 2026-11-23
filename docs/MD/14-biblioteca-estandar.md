# 14 — Biblioteca estándar

Luna `0.7.6` incluye una biblioteca estándar disponible al crear el estado: `print` es solo una de las funciones nativas registradas. Cada carpeta de `Lib/` contiene varios archivos `.luna`: `mod.luna` actúa como fachada pública y el resto implementa la funcionalidad en el propio lenguaje.

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
| `ffi` | `mod.luna` (fachada del módulo nativo `ffi_native`) |

## Funciones globales

| Función | Descripción |
| --- | --- |
| `len(valor)` | Longitud de strings, listas y tablas. |
| `type(valor)` | Nombre del tipo en minúsculas. |
| `str(valor)` | Representación textual. |
| `int(valor)` | Convierte números, booleanos o strings a entero. |
| `float(valor)` | Convierte números, booleanos o strings a float. |
| `input(mensaje)` | Escribe el mensaje y lee una línea de entrada estándar; devuelve un string. |

```luna
print(len("hola"))
print(type(42))
print(str(3.5))
print(int("10"))
print(float("2.5"))
```

Salida:

```text
4
integer
3.5
10
2.5
```

## Funciones añadidas en Luna

| Módulo | Funciones |
| --- | --- |
| `math` | `clamp`, `lerp`, `sign`, `gcd`, `lcm`, `hypotenuse`, `sum`, `mean`, `median`, `sort`, `min_of`, `max_of`, `randint`, `choice`, `shuffle`, `tau`. |
| `string` | `concat`, `repeat`, `reverse`, `pad_left`, `pad_right`, `contains`, `starts_with`, `ends_with`, `split`, `join`. |
| `list` | `first`, `last`, `is_empty`, `extend`, `slice`, `map`, `filter`, `reduce`, `index_of`, `count`, `unique`. |
| `table` | `get_or`, `set_default`, `update`, `values`, `merge`, `has_all`, `clear`. |
| `io` | `ensure_dir`, `read_bytes`, `read_lines`, `write_lines`, `append`. |
| `os` | `getenv_or`, `is_windows`, `is_linux`, `is_macos`, `separator_char`. |
| `time` | `elapsed(start)`, `format_duration(seconds)`, `millis(nanoseconds)`. |
| `path` | `build`, `stem`, `has_ext`, `is_absolute`. |

## Ejemplo: cálculo y cadenas

```luna
import math
import string
import list

fn doble(x):
    return x * 2

let valores = list.map([1, 2, 3], doble)
print(math.sum(valores))
print(string.join(",", valores))
```

Salida:

```text
12
2,4,6
```

## Ejemplo: sistema y tiempo

```luna
import os
import path
import time

print(os.is_linux())
print(os.getenv_or("LUNA_STDLIB_MISSING_VAR_XYZ", "fallback"))
print(path.build(["a", "b", "c"]))
print(path.stem("/tmp/x.txt"))
print(time.format_duration(3661))
print(time.millis(1234567))
```

Salida en Linux:

```text
true
fallback
a/b/c
x
01:01:01
1
```

En macOS o Windows la primera línea cambia. `time.millis` convierte nanosegundos a milisegundos; `time.elapsed(start)` usa el valor devuelto por `time.nanos()`.

## Módulo `ffi`

`ffi` expone `declare`, `declare_symbol`, `load` y `extension`. Se usa para llamar funciones C sin compilar extensiones; está disponible desde `0.7.6` y se documenta en [FFI](15-ffi.md).

## Resolución en runtime

`luna_new()` registra automáticamente las rutas `Lib` que encuentra en este orden:

1. `LUNA_HOME/Lib` y `LUNA_HOME/share/luna/Lib`.
2. Cada entrada de `LUNA_PATH` (separada por `:` en Unix y `;` en Windows) y su subdirectorio `Lib`.
3. `Lib` junto al ejecutable, `Lib` en el directorio padre y `share/luna/Lib` en el padre.
4. La ruta relativa `Lib` del directorio de trabajo, como último recurso.

Las rutas duplicadas se ignoran. Desde C también se puede añadir una ruta con `luna_add_path()` (alias de `luna_add_module_path()`).

Los módulos nativos usan el prefijo `_native`. Por ejemplo, `import math_native` accede directamente a las primitivas implementadas en C; `import math` ofrece la interfaz completa escrita en Luna.

Volver al [índice de documentación](README.md).
