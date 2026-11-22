# Registro de cambios

Todas las versiones de Luna siguen el formato `MAJOR.MINOR.PATCH`.

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
