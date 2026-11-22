# Estructura del proyecto

```text
Luna/
├── assets/          recursos gráficos (luna-logo.svg)
├── build/           artefactos generados (ignorado por git)
├── cli/             main.c del intérprete
├── docs/            documentación
│   ├── index.html   documentación web (página principal)
│   ├── web/         styles.css y script.js
│   ├── MD/          documentación Markdown del lenguaje
│   └── Ayuda/       actualizaciones, información y archivos extra
├── include/luna/    encabezados públicos (luna.h, luna.hpp, luna_types.h)
├── src/             módulos del intérprete
├── tests/           programas .luna y pruebas unitarias en C
├── luna.ebnf        gramática formal
├── luna.py          sistema de build
├── LICENSE          GPL-3.0
└── README.md        presentación del proyecto
```

## Módulos de `src/`

Cada módulo contiene un archivo `luna.build` con `SOURCES`, `HEADERS`, `INCLUDES`, `DEPENDS` y `CFLAGS`.

| Módulo | Dependencias | Responsabilidad |
| --- | --- | --- |
| `memory/` | — | Allocator, GC mark & sweep, límites, objetos GC |
| `runtime/` | memory | `LunaState`, valores tipados, strings, listas, tablas, userdata |
| `errors/` | runtime | `LunaError`, traceback y nombres de estados |
| `platform/` | — | Sistema de archivos, dlopen, rutas, tiempo, entorno |
| `lexer/` | — | Tokens, números e indentación |
| `ast/` | lexer | Nodos del árbol sintáctico |
| `parser/` | lexer, ast | Parser de expresiones y sentencias |
| `bytecode/` | — | Opcode set, `LunaProto`, serialización `.luac` |
| `compiler/` | ast, bytecode | Ámbitos, emisión de bytecode, closures |
| `vm/` | runtime, bytecode, errors, memory | Pila, aritmética, comparación, llamadas, opcodes |
| `modules/` | runtime | Registro, resolución, carga, caché, visibilidad |
| `api/` | runtime, errors, modules, vm, memory, compiler, parser, platform | Implementación de la API C pública |

## Notas

- Existen archivos `lume.build` heredados en seis módulos; `luna.py` solo descubre `luna.build` y los ignora.
- Las pruebas unitarias en `tests/unit/` son dumps C manuales; `luna.py` no tiene comando `test`.
- Los artefactos de compilación se escriben en `build/src/<mod>/` y el binario en `build/luna`.

Para ver el grafo de dependencias real:

```bash
./luna.py graph > deps.dot
```
