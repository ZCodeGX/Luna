# Documentación de Luna

Documentación del lenguaje de programación **Luna**, organizada por temas.

| Documento | Contenido |
| --- | --- |
| [01 — Inicio rápido](01-inicio-rapido.md) | Requisitos, compilación y primeros programas |
| [02 — Sintaxis básica](02-sintaxis-basica.md) | Identificadores, palabras reservadas, comentarios e indentación |
| [03 — Tipos de datos](03-tipos-de-datos.md) | Literales, números, cadenas, listas y tablas |
| [04 — Operadores](04-operadores.md) | Precedencia, aritmética, comparación y lógica |
| [05 — Control de flujo](05-control-de-flujo.md) | `if`, `while` y construcciones pendientes |
| [06 — Funciones](06-funciones.md) | Declaración, llamadas, retorno y funciones nativas |
| [07 — Módulos y visibilidad](07-modulos-y-visibilidad.md) | `import`, `mod`, `pub` y resolución de archivos |
| [08 — Manejo de errores](08-manejo-de-errores.md) | `throw`, `try/catch` y estados de error |
| [09 — CLI y build](09-cli-y-build.md) | Opciones del intérprete y sistema de build |
| [10 — API C](10-api-c.md) | Encabezados, tipos y funciones públicas |
| [11 — Estado de implementación](11-estado-de-implementacion.md) | Qué funciona hoy y qué está en la gramática |
| [12 — Rendimiento y Luna 0.3](12-rendimiento-0-3.md) | Plan de optimización del runtime y benchmarks |
| [13 — Diagnósticos](13-diagnosticos.md) | Errores con spans, recuperación multi-error y renderizado |

La gramática completa fuente de verdad está en [`luna.ebnf`](../../luna.ebnf). La versión web de esta guía está en [`../Web/index.html`](../Web/index.html).

> [!WARNING]
> Luna es `0.7.0`. Muchas construcciones están reconocidas por el parser pero aún no se compilan. Consulta el documento 11 antes de dar algo por soportado.
