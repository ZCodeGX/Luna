# 09 — CLI y build

La CLI de Luna `0.7.6` se usa desde `build/luna`; el sistema de build es `luna.py`. Si todavía no has compilado, empieza por el [inicio rápido](01-inicio-rapido.md).

## Intérprete

Ayuda exacta del binario (`cli/main.c`):

```text
Uso: <prog> [opciones] [archivo.luna] [args...]
  -c CODIGO      ejecuta CODIGO como programa
  -              lee el programa desde stdin
  -h, --help     muestra esta ayuda
  -V, --version  muestra la versión
  --no-color     desactiva el color en diagnósticos
  --color        fuerza el color en diagnósticos
  --max-errors=N limita el número de errores mostrados (0 = sin límite)
Sin argumentos inicia el intérprete interactivo.
```

| Forma | Comportamiento |
| --- | --- |
| `./build/luna -c '...'` | Ejecuta el código como `<string>` |
| `./build/luna -` | Lee todo `stdin` como `<stdin>` |
| `./build/luna archivo.luna [args…]` | Ejecuta el archivo y expone `argv` |
| `./build/luna` | REPL con prompt `>>>` |

El REPL lee una línea por vez (máximo 8192 bytes) y limpia el error después de cada una. En los modos `-c` y `-`, la lista `argv` incluye el propio indicador en el índice 1.

Ejemplos con salida:

```bash
./build/luna -c 'print(1 + 2)'
```

```text
3
```

```bash
printf 'print("hola")\n' | ./build/luna -
```

```text
hola
```

Las opciones `--no-color`, `--color` y `--max-errors=N` controlan el renderizado de diagnósticos; consulta [diagnósticos](13-diagnosticos.md).

## Sistema de build

`luna.py` es un build declarativo inspirado en `moz.build` (versión `0.2.5`).

| Comando | Opciones | Descripción |
| --- | --- | --- |
| `build` | `-v` / `--verbose`, `--main` | Descubre módulos, resuelve dependencias, compila y linkea `build/luna` |
| `clean` | — | Elimina `build/` |
| `test` | — | Ejecuta las suites stdlib, diagnósticos, regresiones, lenguaje y tests unitarios |
| `info` | — | Lista módulos, rutas, fuentes, headers y dependencias |
| `graph` | — | Imprime el grafo de dependencias en formato DOT |

Ejemplos:

```bash
./luna.py build
./luna.py build -v
./luna.py build --main cli/main.c
./luna.py graph > deps.dot
```

### Cómo funciona

- Cada `src/*/luna.build` se ejecuta como Python y define `SOURCES`, `HEADERS`, `INCLUDES`, `DEPENDS` y `CFLAGS`.
- El orden de compilación se calcula con un ordenamiento topológico; se detectan dependencias circulares y módulos desconocidos.
- Se compila con `cc -std=c11 -Wall -Wextra -Wno-unused-parameter -g -O2` más los includes y `CFLAGS` de cada módulo.
- Los objetos se escriben en `build/src/<mod>/`. La compilación es incremental y compara fechas de fuentes y headers.
- El linkeo produce `build/luna` con `-lm -ldl` (salvo Windows).
- Los colores de la salida respetan `NO_COLOR` y `FORCE_COLOR`.

> [!NOTE]
> Existen archivos `lume.build` heredados en seis módulos, pero `luna.py` solo descubre `luna.build`, por lo que se ignoran.

Continúa con [API C](10-api-c.md).
