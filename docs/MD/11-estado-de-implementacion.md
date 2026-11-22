# 11 — Estado de implementación

Luna se encuentra en versión `0.6.0` (ABI 1). La gramática `luna.ebnf` describe el lenguaje objetivo, pero no todas sus construcciones están disponibles en el compilador actual.

## Soportado de extremo a extremo

| Característica | Estado |
| --- | --- |
| Literales `nil`, booleanos, enteros, floats y strings | Disponible |
| Números hexadecimales y binarios | Disponible |
| `let` y `const` | Disponible |
| Chequeo de reasignación de `const` | Disponible |
| Funciones `fn`, parámetros y `return` | Disponible |
| `if` / `else if` / `else` | Disponible |
| `while` | Disponible |
| Operadores aritméticos, de comparación y lógicos | Disponible |
| Asignación simple a identificadores | Disponible |
| `print` (función nativa) | Disponible |
| REPL, `-c`, `-` y ejecución de archivos | Disponible |
| API C y wrapper C++ | Disponible |
| Diagnósticos con spans y multi-error del parser | Disponible |
| Sistema de build con `luna.build` | Disponible |
| `import`, `mod` y `pub` | Disponible |
| Acceso a campos `obj.campo` | Disponible |
| Listas y tablas literales | Disponible |
| Indexación `lista[i]` / `tabla[clave]` | Disponible |
| Asignaciones a `lista[i]` y `obj.campo` | Disponible |
| Asignaciones compuestas `+=`, `-=`, `*=`, `/=` | Disponible |

## Reconocido pero no compilado

| Característica | Parser | Compilador / VM |
| --- | --- | --- |
| `for` ... `in` | Sí | Emite `for loops not yet implemented` |
| `break`, `continue` | Sí | Sin generación de código |
| `try` / `catch` | Sí | La VM tiene opcodes, el compilador no emite |
| `throw` | Sí | Compilado (`OP_THROW`) |
| Lambdas `fn(...)` | Sí | Nodo `NODE_LAMBDA` sin compilar |

| Concatenación de strings con `+` | Sí | Error de tipo en runtime |

## Otros límites

- Solo se permiten espacios para indentar; los tabs son error. Profundidad máxima: 256.
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

Volver al [índice de documentación](README.md).
