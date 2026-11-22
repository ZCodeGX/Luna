# Registro de cambios

Todas las versiones de Luna siguen el formato `MAJOR.MINOR.PATCH`: `MAJOR` sube en actualizaciones muy amplias, `MINOR` en mejoras nuevas y `PATCH` en cada corrección.

## 0.7.2

### Corregido

- El parser ya no ignora texto sobrante después de una sentencia: `print(1 + 2)F` y `let x = 1 let y = 2` son `SyntaxError`.
- `print(1 + 2):` ahora explica que `:` solo es válido después de headers de bloque.
- Se añaden fixtures para ambos casos.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.7.2 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.7.1

### Corregido

- Los errores léxicos fatales (cadena sin cerrar, literal numérico inválido, tabs) ya no se pierden ni cuelgan el parser; se reportan una sola vez con caret de un carácter.
- El sink de diagnósticos respeta `--max-errors=0` como «sin límite», valida argumentos inválidos y solo avisa de truncamiento cuando realmente se descartaron errores.
- Se corrigen fugas de AST en rutas de error de `if`, `for`, `while`, `fn` y `try`, y el caret de asignaciones inválidas apunta al objetivo.
- Se eliminan todos los warnings del build y se añade un límite de profundidad para expresiones muy anidadas.

### Añadido

- Fixtures para cadenas sin cerrar y literales hexadecimales inválidos.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.7.1 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.7.0

### Cambiado

- Los diagnósticos usan tipos con nombre estilo Python (`SyntaxError`, `IndentationError`, `NameError`, `TypeError`, `ValueError`, `RuntimeError` y específicos como `ImportError`, `IndexError`, `KeyError`).
- Se eliminaron los códigos `E0xxx`: un error se identifica por tipo y mensaje.
- Formato de salida estilo Python: `File "x.luna", line N`, snippet con carets, `TypeError: ...` y `help:`.
- Todos los mensajes del parser están en inglés.
- `require_colon` ubica el caret al final del header; los bloques sin indentación reportan `IndentationError` con el número de línea del header.
- Tabs al inicio de línea son `IndentationError`.
- `break`/`continue` fuera de bucle son `SyntaxError`.

### Añadido

- Fixtures para `fn game():`, indentación faltante, tabs y `break` fuera de bucle.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.7.0 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.6.0

### Añadido

- `LunaSpan`, `LunaDiagnostic`, `LunaDiagnosticSink` y renderizado estructurado con snippets, notas y help.
- Recuperación multi-error en el parser: `panic_mode`, `synchronize()` y tokens de error léxico continuables.
- Flags CLI `--no-color`, `--color` y `--max-errors=N`; color automático en TTY y respeto a `NO_COLOR`.
- Runner `tests/parser_diag/run.py` con seis fixtures de diagnósticos.
- Documento `docs/MD/13-diagnosticos.md`.

### Cambiado

- Los errores de parseo se imprimen una sola vez como diagnóstico estructurado; la CLI ya no añade el `luna: ...` duplicado para `LUNA_ERR_SYNTAX`.
- Mensajes del parser traducidos al español.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.6.0 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.5.1

### Añadido

- Chequeo en tiempo de compilación de reasignaciones de variables `const`, incluyendo captured upvalues de un scope padre.
- Fixtures `tests/test_const_valid.luna` y `tests/test_const_invalid.luna`.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.5.1 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.5.0

### Añadido

- Literales de lista `[1, 2, 3]` y de tabla `{ host = "localhost", port = 8080 }` compilados a `OP_NEW_LIST`/`OP_LIST_APPEND` y `OP_NEW_TABLE`/`OP_TABLE_SET`.
- Lectura por índice `lista[i]` y `tabla[clave]` compilada a `OP_GET_INDEX`.
- Asignaciones a `lista[i] = x`, `tabla[clave] = x` y `obj.campo = x`.
- Asignaciones compuestas `+=`, `-=`, `*=` y `/=` sobre identificadores, índices y campos.
- Fixture de integración `tests/test_fase1.luna`.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.5.0 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.4.0

### Añadido

- `import` y `mod` compilados: los módulos `.luna` se resuelven, ejecutan y cachean, y el nombre importado enlaza la tabla de exports.
- `pub` en `let`, `const` y `fn` publica símbolos en la tabla de exports del módulo.
- Acceso a campos `obj.campo` compilado como `OP_GET_FIELD`.
- Nuevo opcode `OP_IMPORT` y publicación de exports mediante la global `__exports__`.
- Fixtures de integración en `tests/test_imports/` con import, exports y acceso a campos.

### Corregido

- Encadenamiento de `LunaTable`: los buckets con colisiones ya no generan ciclos en `find_node`.
- Ejecución anidada de módulos: cada `luna_vm_execute` termina en su propio nivel de frame y reserva stack por encima del frame llamador.
- Resolución de imports relativos al archivo ejecutado con `luna_dofile`.
- Los errores de importación usan `LUNA_ERR_MODULE` y son visibles desde la API C.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.4.0 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.3.0

### Añadido

- Pool de strings internados con tabla hash y limpieza débil integrada en `luna_gc_collect`.
- Plegado de constantes en expresiones binarias aritméticas y comparaciones de literales numéricos.

### Cambiado

- `vm_execute_loop` sustituye al bucle de `vm_run` y usa computed goto con tabla de dispatch en GCC/Clang; otros compiladores conservan un fallback con `switch`.
- El límite de instrucciones se comprueba por presupuestos de 32 instrucciones, ajustando el último tramo para mantener el límite exacto.
- Benchmarks (5 ejecuciones, 1 calentamiento): `loop` 1.39 s, `fib` 0.050 s, `math` 3.97 s (~30 % más rápido que 0.2.9).

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.3.0 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.2.9

### Cambiado

- Fast paths int/int en `OP_ADD`, `OP_SUB`, `OP_MUL`, `OP_DIV`, `OP_MOD` y `OP_POW`: el despacho evita la llamada a `vm_arith`.
- Comparaciones enteras directas en `OP_EQ`, `OP_NEQ`, `OP_LT`, `OP_LE`, `OP_GT` y `OP_GE`, sin pasar por `vm_compare`.
- División y módulo por cero conservan `LUNA_ERR_DIVZERO` y sus mensajes.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.2.9 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.2.8

### Cambiado

- `luna_value_truthy` es `static inline` en `value.h`: condicionales y bucles ya no hacen una llamada por evaluación.
- El límite de instrucciones se comprueba en el propio `vm_run`; el mensaje de error se delega a `luna_report_instruction_limit`.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.2.8 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.2.7

### Cambiado

- `vm_run` mantiene el frame y el bytecode activos entre instrucciones; `base` se recarga en cada paso para soportar reasignaciones del stack.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.2.7 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.2.6

### Cambiado

- `luna_opcode_mode` es `static inline` en `opcodes.h`: el bucle de ejecución ya no hace una llamada externa por instrucción para calcular el avance del PC.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.2.6 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.2.5

### Añadido

- Instaladores `install.sh` (Linux/macOS) e `install.ps1` (Windows): instalan dependencias, compilan, instalan el binario y configuran el PATH.
- Instrucciones de instalación en `README.md` y reglas ampliadas en `.gitignore`.

### Cambiado

- Versión centralizada en `LUNA_VERSION_STRING` (`include/luna/luna.h`); el CLI y `luna.py` la leen desde ahí.
- Activación de ANSI y UTF-8 en consolas de Windows.

### Corregido

- Puente `cc.exe` para MinGW-w64: firma de `_spawnv` y detección de rutas del compilador.
- Declaración de `_getcwd` en Windows mediante `<direct.h>`.
- Recursividad: el compilador reserva la función en el ámbito padre y el VM enlaza los upvalues desde el closure.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.2.5 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`, sin cambios) |

## 0.1.0

Primera versión documentada del repositorio.

### Incluido

- Intérprete con CLI estilo Python: REPL, `-c`, lectura desde `stdin` y ejecución de archivos.
- Lexer, parser y AST con indentación significativa y gramática formal en `luna.ebnf`.
- Compilador a bytecode y VM con pila, aritmética, comparaciones, llamadas y excepciones de bajo nivel.
- Runtime con valores tipados, cadenas, listas, tablas y userdata, con recolector de basura.
- Sistema de build declarativo `luna.py` con descubrimiento de módulos y orden topológico.
- API C pública y wrapper C++, con registro de funciones y módulos nativos.
- Cargador de módulos con resolución de rutas, caché y detección de ciclos.
- Función nativa `print` y global `argv` en la CLI.
- Licencia GPL-3.0.

### Limitaciones conocidas

Consulta [estado de implementación](../../MD/11-estado-de-implementacion.md). En resumen: `for`, `break`, `continue`, `try/catch`, `import`, `mod`, lambdas, literales de lista/tabla e indexación no están disponibles desde código fuente compilado.

### Versiones relacionadas

| Componente | Versión |
| --- | --- |
| API C | 0.1.0 (ABI 1) |
| Bytecode `.luac` | 0.1 (magia `LUNA`) |
| Gramática `luna.ebnf` | “Luna 1.0” (versión de la especificación, no de release) |

## Plantilla para futuras entradas

```markdown
## X.Y.Z — fecha

### Añadido
### Cambiado
### Corregido
### Eliminado
### Limitaciones
```
