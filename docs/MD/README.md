# Documentación de Luna

Aprende Luna en 20 minutos y escribe tu primer programa. Esta guía Markdown es la fuente de verdad complementaria a la [documentación web](../Web/index.html): explica qué funciona hoy en `0.7.6` y qué solo está en la gramática.

## Ruta guiada

| Hito | Objetivo | Lectura |
| --- | --- | --- |
| Despegue | Instalar y ejecutar | [01 — Inicio rápido](01-inicio-rapido.md), [09 — CLI y build](09-cli-y-build.md) |
| Órbita | Sintaxis y tipos | [02 — Sintaxis](02-sintaxis-basica.md), [03 — Tipos](03-tipos-de-datos.md), [04 — Operadores](04-operadores.md) |
| Aterrizaje | Funciones, colecciones y errores | [05 — Control de flujo](05-control-de-flujo.md), [06 — Funciones](06-funciones.md), [07 — Módulos](07-modulos-y-visibilidad.md), [08 — Errores](08-manejo-de-errores.md), [16 — Primer proyecto](16-primer-proyecto.md) |
| Exploración | Módulos, stdlib y FFI | [14 — Biblioteca estándar](14-biblioteca-estandar.md), [15 — FFI](15-ffi.md), [17 — Recetas](17-recetas.md), [18 — Guía de bibliotecas](18-guia-de-bibliotecas.md) |

## Índice

| Documento | Contenido |
| --- | --- |
| [01 — Inicio rápido](01-inicio-rapido.md) | Requisitos, build, ejecución de archivo, REPL y primer programa |
| [02 — Sintaxis básica](02-sintaxis-basica.md) | Identificadores, `extern`, comentarios, `:` e indentación |
| [03 — Tipos de datos](03-tipos-de-datos.md) | Literales, listas, tablas, indexación y `type` |
| [04 — Operadores](04-operadores.md) | Precedencia, aritmética, lógica y asignación compuesta |
| [05 — Control de flujo](05-control-de-flujo.md) | `if`, `while` y construcciones aún no compiladas |
| [06 — Funciones](06-funciones.md) | `fn`, retorno, closures y nativas |
| [07 — Módulos y visibilidad](07-modulos-y-visibilidad.md) | `import`, `mod`, `pub` y resolución de archivos |
| [08 — Manejo de errores](08-manejo-de-errores.md) | `throw`, `try`/`catch` y estados de error |
| [09 — CLI y build](09-cli-y-build.md) | Opciones del intérprete y sistema de build |
| [10 — API C](10-api-c.md) | Encabezados, tipos, funciones públicas y FFI |
| [11 — Estado de implementación](11-estado-de-implementacion.md) | Qué funciona de extremo a extremo y qué no |
| [12 — Rendimiento y Luna 0.3](12-rendimiento-0-3.md) | Plan de optimización del runtime y benchmarks |
| [13 — Diagnósticos](13-diagnosticos.md) | Errores con spans, multi-error y renderizado |
| [14 — Biblioteca estándar](14-biblioteca-estandar.md) | Funciones globales y módulos `math`, `string`, `list`, `table`, `io`, `os`, `time`, `path`, `http`, `json`, `csv`, `gml` y `ffi` |
| [15 — FFI](15-ffi.md) | `extern`, firmas, alias de librería, módulo `ffi` y límites |
| [16 — Primer proyecto](16-primer-proyecto.md) | Mini agenda: listas, tablas, funciones y errores |
| [17 — Recetas](17-recetas.md) | Ejemplos cortos con problema, código y salida |
| [18 — Guía de bibliotecas](18-guia-de-bibliotecas.md) | Uso práctico de todas las bibliotecas disponibles |

La gramática está en [`luna.ebnf`](../../luna.ebnf); los ejemplos ejecutables están en [`tests/`](../../tests/). Para ayuda, consulta el [índice de Ayuda](../Ayuda/README.md).

> [!IMPORTANT]
> Luna es `0.7.6` (ABI 1, GPL-3.0). `for`, `break`, `continue`, las lambdas anónimas y `+` para strings están en la gramática pero **no** son usables: consulta el [estado de implementación](11-estado-de-implementacion.md).
