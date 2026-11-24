# 11 — Estado de implementación

Luna se encuentra en versión `0.7.6` (ABI 1, GPL-3.0). La gramática `luna.ebnf` describe el lenguaje objetivo, pero no todas sus construcciones están disponibles en el compilador actual.

## Soportado de extremo a extremo

| Característica | Estado |
| --- | --- |
| Literales `nil`, booleanos, enteros, floats y strings | Disponible |
| Números hexadecimales y binarios | Disponible |
| `let` y `const` | Disponible |
| Chequeo de reasignación de `const` | Disponible |
| Funciones `fn`, parámetros, `return` y closures | Disponible |
| `if` / `else if` / `else` | Disponible |
| `while` | Disponible |
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

`print` no es la única función nativa: al crear un estado se registran las globales `len`, `type`, `str`, `int`, `float` e `input` y los módulos `math`, `string`, `list`, `table`, `io`, `os`, `time`, `path` y `ffi`. Consulta la [biblioteca estándar](14-biblioteca-estandar.md).

## Reconocido pero no compilado

| Característica | Parser | Compilador / VM |
| --- | --- | --- |
| `for` ... `in` | Sí | Emite `for loops not yet implemented` |
| `break`, `continue` | Sí | Sin generación de código |
| Concatenación de strings con `+` | Sí | Error de tipo en runtime |

Usa `while`, funciones con nombre y `string.concat` en su lugar. La sintaxis de bloque válida se resume en [sintaxis básica](02-sintaxis-basica.md).

## FFI (0.7.6)

- Palabra clave `extern`: `extern nombre("librería", "firma")`; equivale a
  `let nombre = __ffi_declare("nombre", "librería", "firma")`.
- Firma: `"retorno(arg1,arg2,...)"`.
- Tipos: `void`, `int`/`i64`, `float`/`f64`/`double`, `str`/`string`/`cstr`, `ptr`/`pointer`.
- Alias de librería: `""`, `"c"`, `"libc"`, `"self"`, `"current"` (proceso actual) y `"m"`/`"libm"`.
- Hasta 8 parámetros declarados y 4 parámetros en la llamada.
- Argumentos homogéneos: todos enteros/punteros/strings o todos `float`; no se permite mezclarlos.
- Solo ABI C. Para C++ exportar con `extern "C"` o usar `ffi.declare_symbol`.
- Módulo `ffi`: `declare`, `declare_symbol`, `load`, `extension`.
- Ejecutable de ejemplo: `tests/regression/ffi.luna`, con salida en `tests/regression/ffi.expected`.

La referencia completa está en [FFI](15-ffi.md).

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
