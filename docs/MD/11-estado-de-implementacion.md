# 11 — Estado de implementación

Luna se encuentra en versión `0.7.6` (ABI 1, GPL-3.0). La gramática `luna.ebnf` describe el lenguaje objetivo, pero no todas sus construcciones están disponibles en el compilador actual.

## Soportado de extremo a extremo

| Característica | Estado |
| --- | --- |
| Literales `nil`, booleanos, enteros, floats y strings | Disponible |
| Números hexadecimales y binarios | Disponible |
| `let` y `const` | Disponible |
| Chequeo de reasignación de `const` | Disponible |
| Funciones `fn`, parámetros, `return`, closures y lambdas `fn(...)` | Disponible |
| `if` / `else if` / `else` | Disponible |
| `while`, `for ... in`, `break` y `continue` | Disponible |
| Operadores aritméticos, de comparación y lógicos | Disponible |
| Asignación a identificadores, índices y campos | Disponible |
| Asignaciones compuestas `+=`, `-=`, `*=`, `/=` | Disponible |
| `print` y el resto de la biblioteca estándar | Disponible |
| REPL, `-c`, `-` y ejecución de archivos | Disponible |
| API C y wrapper C++ | Disponible |
| Diagnósticos estilo Python (tipos con nombre, multi-error, sin códigos) | Disponible |
| Sistema de build con `luna.build` | Disponible |
| `import`, `mod` y `pub` | Disponible |
| Listas y tablas literales con indexación | Disponible |
| `try` / `catch` y `throw` | Disponible |
| Errores de compilación con archivo/línea/columna | Disponible |
| FFI con `extern` y módulo `ffi` | Disponible desde 0.7.6 |

`print` no es la única función nativa: al crear un estado se registran las globales `print`, `len`, `type`, `str`, `int`, `float`, `bool` e `input`, todas uso directo sin `import`. También están los módulos opcionales `math`, `string`, `list`, `table`, `io`, `os`, `time`, `path` y `ffi`. Consulta la [biblioteca estándar](14-biblioteca-estandar.md).

## Reconocido pero no compilado

| Característica | Parser | Compilador / VM |
| --- | --- | --- |
| Concatenación de strings con `+` | Sí | Error de tipo en runtime |

Usa `string.concat` o `string.join` en su lugar. La sintaxis de bloque válida se resume en [sintaxis básica](02-sintaxis-basica.md).

## FFI (0.7.6)

- Palabra clave `extern`: `extern nombre("librería", "firma")`; equivale a
  `let nombre = __ffi_declare("nombre", "librería", "firma")`.
- Firma: `"retorno(arg1,arg2,...)"`.
- Tipos: `void`, `int`/`i64`, `float`/`f64`/`double`, `str`/`string`/`cstr`, `ptr`/`pointer`.
- Alias de librería: `""`, `"c"`, `"libc"`, `"self"`, `"current"` (proceso actual) y `"m"`/`"libm"`.
- Hasta 8 parámetros declarados y 4 parámetros en la llamada.
- Argumentos homogéneos: todos enteros/punteros/strings o todos `float`; no se permite mezclarlos.
- Solo ABI C. Para C++ exportar con `extern "C"` o usar `ffi.declare_symbol`.
- Módulo `ffi`: `declare`, `declare_symbol`, `load`, `extension`. `ffi.load()` devuelve un `userdata` con finalizer; cada llamada abre su propio handle y no hay caché global.
- Ejecutables de ejemplo: `tests/regression/ffi.luna` y `tests/regression/ffi_lifecycle.luna`, con sus archivos `.expected`.

La referencia completa está en [FFI](15-ffi.md).

## HTTP (0.7.6)

- Solo `http://`; `https://` se rechaza con un mensaje explícito porque no hay TLS.
- Cabeceras: máximo 1 MiB agregados, 8192 bytes por línea y sin CR/LF en nombres o valores.
- Cuerpo: máximo 64 MiB. URL: máximo 8192 bytes y host: máximo 512 bytes.
- El método, el host y el destino de la petición se validan antes de conectar.

## Procesos (No publicada)

- `os.run(command, cwd?)` captura hasta 8 MiB por flujo (`stdout`, `stderr`) y devuelve `ok`, `code`, `stdout` y `stderr`.
- `os.system(command, cwd?)` hereda la consola y devuelve el código de salida.
- El comando se interpreta con `/bin/sh -c` en POSIX y con `cmd.exe` en Windows; se ejecuta con los permisos del proceso Luna.
- En Windows `stderr` se fusiona con `stdout` (`_popen`), porque no hay captura separada de flujos.
- No hay `timeout`, ni paso de entrada estándar por tubería, ni ejecución directa por argv: usá comandos de shell.

## Otros límites

- Solo se permiten espacios para indentar; los tabs son error. Profundidad máxima: 256.
- Las cabeceras de bloque (`fn`, `if`, `else`, `while`, `try`, `catch`) exigen `:`.
- Los strings usan únicamente comillas dobles.
- El `print` de floats usa `%g`.
- Límite de longitud de números en el lexer: 64 caracteres para hex/binario y 128 para decimal/float.
- La sintaxis y la API C pueden cambiar entre versiones; no usar en producción.

## Fuentes

- `luna.ebnf` — gramática.
- `src/parser/` — parser y precedencia.
- `src/compiler/compiler.c` — lo que realmente se compila.
- `src/vm/` — ejecución y opcodes.
- `tests/` — ejemplos y pruebas unitarias en C.
- [Primer proyecto](16-primer-proyecto.md) y [recetas](17-recetas.md) — ejemplos ejecutables.

Volver al [índice de documentación](README.md).
