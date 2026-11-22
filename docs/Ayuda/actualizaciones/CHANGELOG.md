# Registro de cambios

Todas las versiones de Luna siguen el formato `MAJOR.MINOR.PATCH`: `MAJOR` sube en actualizaciones muy amplias, `MINOR` en mejoras nuevas y `PATCH` en cada corrección.

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
