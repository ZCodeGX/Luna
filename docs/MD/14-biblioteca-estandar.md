# 14 — Biblioteca estándar

Luna `0.7.6` incluye una biblioteca estándar disponible al crear el estado: `print` es solo una de las funciones nativas registradas. Cada carpeta de `Lib/` contiene varios archivos `.luna`: `mod.luna` actúa como fachada pública y el resto implementa la funcionalidad en el propio lenguaje.

La capa nativa en C (`*_native`) aporta las operaciones primitivas y es invisible para el código de usuario habitual: los módulos `.luna` las envuelven, las combinan y añaden lógica escrita en Luna. Para ejemplos de uso de cada módulo, consulta la [guía práctica de bibliotecas](18-guia-de-bibliotecas.md).

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
| `http` | `client.luna`, `server.luna`, `mod.luna` (fachada del módulo nativo `http_native`) |
| `json` | `mod.luna` (fachada del módulo nativo `json_native`) |
| `csv` | `mod.luna` (fachada del módulo nativo `csv_native`) |
| `gml` | `mod.luna` (fachada del módulo nativo `gml_native`) |
| `ffi` | `mod.luna` (fachada del módulo nativo `ffi_native`) |

## Funciones globales

| Función | Descripción |
| --- | --- |
| `len(valor)` | Longitud de strings, listas y tablas. |
| `type(valor)` | Nombre del tipo en minúsculas. |
| `str(valor)` | Representación textual. |
| `int(valor)` | Convierte números, booleanos o strings a entero. |
| `float(valor)` | Convierte números, booleanos o strings a float. |
| `input(mensaje)` | Escribe el mensaje opcional y lee una línea de entrada estándar; devuelve un string. |

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
| `http` | `encode`, `decode`, `build_url`, `request`, `get`, `post`, `put`, `delete`, `is_ok`, `listen`, `serve`, `serve_once`, `close`, `reply`, `text`, `html`. |
| `json` | `encode`, `stringify`, `decode`, `parse`, `pretty`, `is_object`, `is_array`. |
| `csv` | `parse`, `decode`, `encode`, `stringify`. |
| `gml` | `app`, `window`, `button`, `label`, `entry`, `text`, `checkbox`, `set_text`, `get_text`, `poll`, `show`, `quit`, `run`. |

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

## Módulo `http`

`http` ofrece un cliente HTTP/1.1 sobre `http://` y un servidor básico para escuchar en una interfaz local. Las funciones nativas viven en `http_native`; el módulo `http` añade codificación de consultas, cabeceras de respuesta y el bucle de aceptación escrito en Luna.

```luna
import http

let response = http.get("http://127.0.0.1:8080/index.html", {timeout = 3})
print(response.ok)
print(response.status)
print(response.reason)
print(response.body)
```

`response.headers` es una tabla con las cabeceras de respuesta en minúsculas. `get`, `post`, `put` y `delete` devuelven `{ok, status, reason, headers, body}` o lanzan una excepción si la petición falla. `https://` no está soportado: se requiere una capa TLS.

```luna
import http
import string

fn handler(req):
    return http.reply(200, string.concat("recibido: ", req.body), nil)

let server = http.listen("127.0.0.1", 8080)
http.serve(server, handler, 4, 1)
http.close(server)
```

`req` contiene `method`, `path`, `body` y `headers`. `serve_once(server, handler, timeout)` devuelve `false` cuando expira el tiempo de espera; `serve(server, handler, max_requests, timeout)` repite hasta alcanzar `max_requests` o hasta que se cierre el servidor. `reply`, `text` y `html` construyen la tabla de respuesta que consume `http_native.respond`.

## Módulos `json` y `csv`

`json` convierte entre valores Luna y texto JSON. Los objetos se representan con tablas y los arrays con listas; `pretty` genera una versión legible con sangría.

```luna
import json

let data = {nombre = "Luna", activo = true, numeros = [1, 2, 3]}
let text = json.pretty(data)
print(text)
let back = json.decode(text)
print(back.nombre, back.numeros[2])
```

`csv` parsea y serializa filas de texto delimitado. `parse` devuelve una lista de filas y `encode` acepta listas o tablas.

```luna
import csv

let rows = csv.parse("nombre,valor\na,1\nb,2\n")
print(rows[1][1], rows[2][1])
print(csv.encode(rows, ";"))
```

## Módulo `gml`

`gml` es una interfaz gráfica estilo Tk para Luna. Requiere `python3` con `tkinter` y un servidor de pantalla; en Linux necesita `DISPLAY`. Cada función recibe la aplicación devuelta por `gml.app()`, excepto `run`, que despacha eventos al handler escrito en Luna.

```luna
import gml

let app = gml.app()
let win = gml.window(app, "Hola", 420, 200)
gml.label(app, win, "Nombre:", 20, 20, 80, 28)
let field = gml.entry(app, win, "", 110, 20, 180, 28)

fn handler(event):
    if event.type == "click":
        print("texto:", gml.get_text(app, field))
        gml.quit(app)

gml.button(app, win, "Aceptar", 20, 70, 120, 32)
gml.run(app, handler, 16)
```

Los eventos son tablas con `type`, `id`, `window`, `text` o `message`. Los tipos son `ready`, `click`, `enter`, `toggle`, `close`, `value`, `quit` y `error`. `gml.run` termina cuando recibe `close` o `quit`.

## Entrada desde terminal

La global `input(mensaje)` ya está integrada en el lenguaje. El mensaje es opcional; `input()` lee una línea completa desde stdin, elimina el salto de línea y devuelve un string.

```luna
let nombre = input("Nombre: ")
print("Hola", nombre)
```

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
