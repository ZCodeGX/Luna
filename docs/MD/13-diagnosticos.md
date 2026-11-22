# 13 — Diagnósticos

Luna `0.7.0` usa diagnósticos con nombres de tipo estilo Python, texto en inglés y **sin códigos numéricos**. El parser continúa tras un error, se sincroniza en fronteras de sentencia y puede reportar varios problemas en un mismo pase.

## Formato

```text
  File "p.luna", line 1
    fn foo()
            ^
SyntaxError: expected ':' after function signature

    help: insert ':' at the end of the line
```

El encabezado identifica el archivo y la línea; el carets va bajo el token problemático (o al final del token anterior cuando falta `:`). El tipo (`SyntaxError`, `IndentationError`, …) es el identificador del error. No hay `[E000x]`.

## Tipos de error

| Tipo | Uso |
| --- | --- |
| `SyntaxError` | Estructura inválida del programa. |
| `IndentationError` | Indentación faltante, excesiva, mixta o con tabs. |
| `NameError` | Nombre no definido. |
| `TypeError` | Operación con tipos incompatibles. |
| `ValueError` | Valor correcto en tipo pero inapropiado. |
| `RuntimeError` | Fallos generales de ejecución. |
| `ZeroDivisionError` | División por cero. |
| `IndexError` | Índice fuera de rango. |
| `KeyError` | Clave inexistente en tabla. |
| `AttributeError` | Campo inexistente. |
| `ImportError` | Módulo no importable. |

Los tipos específicos se mantienen porque son más informativos que un código numérico.

## Mensajes habituales

| Tipo | Mensaje | Help |
| --- | --- | --- |
| `SyntaxError` | `expected ':' after function signature` | `insert ':' at the end of the line` |
| `SyntaxError` | `expected ':' after 'if' condition` | `insert ':' at the end of the line` |
| `SyntaxError` | `expected ':' after 'while' condition` | `insert ':' at the end of the line` |
| `SyntaxError` | `expected ':' after 'for' clause` | `insert ':' at the end of the line` |
| `SyntaxError` | `expected ')' to close function call` | `insert ')' after argument list` |
| `SyntaxError` | `expected identifier` | `names must start with a letter or '_'` |
| `SyntaxError` | `invalid syntax` | `expected an expression` |
| `SyntaxError` | `'break' outside loop` | `'break' can only be used inside 'while' or 'for'` |
| `SyntaxError` | `'try' block requires a 'catch' clause` | `add 'catch e:' at the same indentation level` |
| `IndentationError` | `expected an indented block after 'fn' declaration on line N` | `indent the block with 4 spaces` |
| `IndentationError` | `unexpected indent` | `this line is more indented than the previous one` |
| `IndentationError` | `tabs are not allowed for indentation` | `use spaces only (4 spaces per level)` |
| `IndentationError` | `unindent does not match any outer indentation level` | `check your indentation; use 4 spaces per level` |

## CLI

| Opción | Efecto |
| --- | --- |
| `--no-color` | Desactiva ANSI en diagnósticos. |
| `--color` | Fuerza ANSI incluso sin TTY. |
| `--max-errors=N` | Limita errores mostrados; `0` equivale a sin límite (20 por defecto). |

También se respeta `NO_COLOR`.

## API C

```c
#include <luna/diagnostics.h>

LunaDiagnosticSink sink;
luna_diag_sink_init(&sink, 20);

LunaParser parser;
LunaNode* program = luna_parse_with_diagnostics(src, len, "main.luna", &sink, &parser);
if (!program) {
    luna_diag_render_all(&sink, src, len, "main.luna", stderr, false);
    luna_parser_destroy(&parser);
    luna_diag_sink_free(&sink);
    return 1;
}
```

`LunaDiagnostic` guarda un `LunaErrorType`, un `LunaSpan` y texto en inglés. El renderer convierte offsets a `File "...", line N` y dibuja los carets.

## Pruebas

```bash
python3 tests/parser_diag/run.py
```

Cada fixture `.luna` tiene su `.expected` exacto generado con `--no-color`.
