# Guía práctica de bibliotecas

Esta guía muestra cómo usar las bibliotecas disponibles en Luna `0.7.6`. La referencia de alto nivel está en [14 — Biblioteca estándar](14-biblioteca-estandar.md) y los límites de FFI en [15 — FFI](15-ffi.md).

## Convenciones

- Las listas se indexan desde `0`: `xs[0]` es el primer elemento.
- Las tablas literales usan `clave = valor`: `{nombre = "Luna", activo = true}`.
- Para concatenar strings usa `string.concat(a, b)` o `string.join(sep, xs)`; `+` no está disponible.
- `for`, `break`, `continue` y las lambdas anónimas están disponibles; usa `string.concat` para unir cadenas.
- Los módulos nativos `*_native` están debajo de las fachadas. Importa la fachada salvo que necesites la primitiva directa.

## Globales y entrada de terminal

`print`, `len`, `type`, `str`, `int`, `float`, `bool` e `input` están integradas en el lenguaje y se usan sin `import`. `input(mensaje)` acepta un mensaje opcional y devuelve una línea de stdin sin el salto final; `bool(valor)` aplica el valor de verdad de Luna (`nil`, `false`, cero y contenedores vacíos son falsos).

```luna
let nombre = input("Nombre: ")
print("Hola", nombre)
print(len(nombre), type(nombre), str(42))
```

## `math`

Agrupa constantes, operaciones numéricas, trigonometría, estadística y aleatoriedad.

```luna
import math

print(math.pi)
print(math.sqrt(16), math.pow(2, 8))
print(math.clamp(15, 0, 10), math.gcd(24, 18))
print(math.sin(0), math.cos(0))

let valores = [3, 1, 4, 1, 5]
print(math.sum(valores), math.mean(valores), math.median(valores))
print(math.sort(valores), math.min_of(valores), math.max_of(valores))

math.seed(7)
print(math.randint(1, 6), math.choice(valores))
```

## `string`

Incluye búsqueda, troceado, unión, reemplazo y transformaciones de texto.

```luna
import string

let texto = "  Luna Script  "
print(string.trim(texto))
print(string.upper("luna"), string.lower("LUNA"))
print(string.sub("Luna", 0, 2))
print(string.contains("Luna", "na"), string.find("Luna", "na"))
print(string.replace("a-b-c", "-", ":"))

let partes = string.split("a,b,c", ",")
print(partes)
print(string.join(" | ", partes))
```

`string.sub(cadena, inicio, fin)` devuelve el rango `[inicio, fin)`.

## `list`

Las operaciones funcionales reciben funciones con nombre; no aceptan lambdas.

```luna
import list

fn doble(x):
    return x * 2

fn par(x):
    return x % 2 == 0

fn sumar(acc, x):
    return acc + x

let xs = list.new()
list.push(xs, 1)
list.push(xs, 2)
list.push(xs, 3)
list.push(xs, 4)
print(xs, list.len(xs), list.first(xs), list.last(xs))

print(list.map(xs, doble))
print(list.filter(xs, par))
print(list.reduce(xs, sumar, 0))
print(list.slice(xs, 1, 3), list.unique([1, 2, 2, 3]))
```

## `table`

Las claves de tabla no tienen orden garantizado. Usa `table.keys` o `table.values` cuando el orden no importe.

```luna
import table

let cfg = table.new()
table.set(cfg, "host", "127.0.0.1")
table.set(cfg, "port", 8080)
print(table.get(cfg, "host"), table.get_or(cfg, "debug", false))
print(table.keys(cfg), table.values(cfg))

table.update(cfg, {debug = true, port = 9090})
print(cfg)
print(table.has(cfg, "debug"), table.has_all(cfg, ["host", "port"]))
table.remove(cfg, "debug")
print(cfg)
```

Los literales `{host = "x"}` son equivalentes y convenientes para tablas pequeñas.

## `io` y `path`

`io` lee y escribe archivos; `path` compone y consulta rutas sin depender del separador del sistema.

```luna
import io
import path

let dir = path.build(["datos", "luna"])
io.ensure_dir(dir)
let file = path.join(dir, "notas.txt")
io.write(file, "primera\nsegunda\n")

print(io.exists(file), io.size(file))
print(io.read(file))
print(io.read_lines(file))
io.append(file, "tercera\n")
print(io.read_lines(file))

print(path.dirname(file), path.basename(file), path.ext(file), path.stem(file))
print(path.has_ext(file, ".txt"), path.is_absolute(file))
```

## `os`

Información del entorno, variables y nombre de plataforma.

```luna
import os

print(os.name())
print(os.cwd())
print(os.is_linux(), os.is_macos(), os.is_windows())
print(os.getenv("LUNA_STDLIB_MISSING_VAR_XYZ"))
print(os.getenv_or("LUNA_STDLIB_MISSING_VAR_XYZ", "fallback"))
print(os.separator_char())
```

También permite ejecutar procesos. `os.run(command, cwd?)` captura `stdout` y `stderr` en una tabla con `ok`, `code`, `stdout` y `stderr`; `os.system(command, cwd?)` deja que el comando escriba en la consola y devuelve el código de salida.

```luna
import os
import string

let resultado = os.run("echo hola; echo aviso 1>&2; exit 3")
print(resultado.ok, resultado.code)
print(string.trim(resultado.stdout), string.trim(resultado.stderr))
print(os.system("exit 0"))
```

Salida: `false 3`, luego `hola aviso` y finalmente `0`. Los comandos usan el shell del sistema y los permisos del proceso Luna: validá cualquier dato que inserts en la cadena. Cada salida capturada se corta en 8 MiB y en Windows `stderr` se mezcla con `stdout`.

## `time`

Relojes, temporizadores y formato de duraciones.

```luna
import time

let t0 = time.nanos()
time.sleep(0.01)
print(time.since(t0))
print(time.now())
print(time.format_duration(3661), time.millis(1234567))
```

## `http`

Cliente HTTP/1.1 y servidor básico. No hay soporte TLS: las URL `https://` se rechazan.

```luna
import http

let response = http.get("http://127.0.0.1:8080/index.html", {timeout = 3})
print(response.ok, response.status, response.reason)
print(response.body)
print(http.is_ok(response))
```

Para enviar datos:

```luna
import http
import table

let headers = table.new()
table.set(headers, "Content-Type", "application/json")
let res = http.post("http://127.0.0.1:8080/api", "{\"ok\":true}", {headers = headers})
print(res.status, res.body)
```

Servidor con un handler escrito en Luna:

```luna
import http
import string

fn handler(req):
    return http.text(string.concat("recibido: ", req.body), nil)

let server = http.listen("127.0.0.1", 8080)
http.serve(server, handler, 4, 1)
http.close(server)
```

`req` contiene `method`, `path`, `body` y `headers`. `serve_once` atiende una conexión y devuelve `false` si expira el timeout; `serve` repite hasta `max_requests`.

Límites del módulo: URLs de hasta 8192 bytes, hosts de hasta 512 bytes, cabeceras de hasta 8192 bytes por línea y 1 MiB en total, y cuerpos de hasta 64 MiB. Los nombres y valores de cabecera no pueden contener CR/LF; el método y el destino de la petición se validan antes de abrir la conexión. Solo se admite `http://`: para `https://` se requiere un cliente con TLS.

## `json`

Conversión entre valores Luna y texto JSON. Objetos son tablas y arrays son listas.

```luna
import json

let data = {nombre = "Luna", activo = true, numeros = [1, 2, 3]}
let text = json.encode(data)
print(text)
print(json.pretty(data))

let back = json.decode(text)
print(back.nombre, back.numeros[2])
print(json.is_object(back), json.is_array(back.numeros))
print(json.parse("[1,2,3]"), json.stringify(data))
```

`decode` y `parse` son aliases; `encode` y `stringify` también.

## `csv`

Filas delimitadas con soporte para comillas, comas internas y saltos de línea.

```luna
import csv

let text = "nombre,valor\na,1\nb,2\n"
let rows = csv.parse(text)
print(rows)
print(rows[1][1], rows[2][1])

let rows2 = [["nombre", "valor"], ["a", "1"], ["b", "2"]]
print(csv.encode(rows2, ","))
print(csv.decode("a;b\nc;d", ";"))
```

`parse` y `decode` devuelven una lista de filas; cada fila es una lista de strings. El delimitador por defecto es `,`.

## `gml`

Interfaz gráfica estilo Tk. Requiere `python3` con `tkinter`; en Linux también necesita un servidor de pantalla y `DISPLAY`.

```luna
import gml

let app = gml.app()
let win = gml.window(app, "Hola GML", 420, 200)
gml.label(app, win, "Nombre:", 20, 20, 80, 28)
let field = gml.entry(app, win, "", 110, 20, 180, 28)
gml.set_text(app, field, "Luna")

fn handler(event):
    if event.type == "click":
        print(gml.get_text(app, field))
        gml.set_text(app, field, "enviado")
    if event.type == "close" or event.type == "quit":
        gml.quit(app)

gml.button(app, win, "Aceptar", 20, 70, 120, 32)
gml.run(app, handler, 16)
```

También están `gml.text`, `gml.checkbox`, `gml.poll` y `gml.show`. Los eventos incluyen `type`, `id`, `window`, `text` o `message`.

## `ffi`

Declara símbolos C con `extern` o con el módulo `ffi`. Es el mecanismo de escape para bibliotecas del sistema.

```luna
import ffi

extern abs("c", "int(int)")
print(abs(-7))

let powi = ffi.declare("pow", "m", "float(float,float)")
print(powi(2, 8))
```

La firma usa `c` como alias de `libc`, `m` de `libm` y el tipo de retorno con los tipos de argumentos entre paréntesis. Hay un máximo de 8 parámetros declarados y 4 por llamada; no se admiten structs, variádicas ni callbacks C→Luna. Consulta [15 — FFI](15-ffi.md) para los detalles completos.

## Módulos nativos

Si una fachada no expone una primitiva concreta, puedes importar el módulo nativo directamente: `math_native`, `string_native`, `list_native`, `table_native`, `io_native`, `os_native`, `time_native`, `path_native`, `ffi_native`, `http_native`, `json_native`, `csv_native` y `gml_native`. Estos nombres son internos y pueden cambiar; prefiere siempre la fachada pública.

Volver al [índice de documentación](README.md).
