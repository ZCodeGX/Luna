# 13 — Diagnósticos

Luna `0.6.0` usa diagnósticos estructurados compartidos por lexer y parser. Ya no se aborta en el primer error: el parser emite varios diagnósticos en un mismo pase y se sincroniza en fronteras de sentencia para evitar cascadas.

## Formato

```text
error[E0001]: se esperaba ; pero se encontró IDENT
  --> programa.luna:3:15
3 | let x = 1 + 2 y
  |               ^
   = note: token encontrado: identificador

error[E0002]: se esperaba una expresión
  --> programa.luna:4:9
4 | let z = =
  |         ^
   = note: token encontrado: ASSIGN
```

`LunaSpan` guarda offsets `[start, end)` sobre el source; la conversión a `línea:columna` ocurre solo al renderizar. Los strings de un diagnóstico son propiedad del `LunaDiagnosticSink`.

## Códigos

| Código | Significado |
| --- | --- |
| `E0001` | Token esperado no encontrado (`expect`). |
| `E0002` | Error sintáctico general o error léxico propagado. |

Cuando el token encontrado está cerca del esperado, el sink añade `help: quizás quisiste decir ...`.

## CLI

| Opción | Efecto |
| --- | --- |
| `--no-color` | Desactiva ANSI en diagnósticos. |
| `--color` | Fuerza ANSI incluso sin TTY. |
| `--max-errors=N` | Limita errores mostrados; `0` equivale a sin límite (el intérprete usa 20 por defecto). |

También se respeta `NO_COLOR`. En TTY el color se activa automáticamente.

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

Cuando el sink es propio del parser, `luna_parse_named` lo crea y `luna_parser_destroy` lo libera. No llames a `luna_parser_destroy` antes de renderizar.

## Pruebas

```bash
python3 tests/parser_diag/run.py
```

Cada fixture `.luna` tiene su `.expected` exacto generado con `--no-color`.
