# Luna 0.3 — Performance Revolution

## Decisión de arquitectura

**C: intérprete optimizado con arquitectura preparada para JIT.**

Se mantiene el intérprete de bytecode actual como base y se exprime su rendimiento antes de introducir un compilador dinámico. El despacho central (`src/vm/opcodes.c`) y la representación genérica de valores (`src/runtime/value.c`) tienen margen medible sin asumir todavía la complejidad de un JIT. El bytecode y la convención de registros se mantienen estables para dejar una costura limpia a una futura capa JIT.

## Objetivo

Reducir el coste de cada instrucción y de cada acceso a memoria, sin cambiar la sintaxis, sin romper programas existentes y sin sacrificar seguridad de memoria.

| Pilar | Objetivo |
| --- | --- |
| Fast Runtime | Menor coste por instrucción y menos comprobaciones redundantes |
| Optimizing Engine | Optimizaciones seguras de bytecode antes de ejecutar |
| Memory Efficiency | Menos asignaciones, copias y presión sobre el GC |

## Principios

1. Medir antes y después de cada cambio.
2. Comparar medianas de al menos cinco ejecuciones, no una sola muestra.
3. Mantener C como referencia nativa y Python como referencia de intérprete.
4. Verificar que la salida del programa no cambia: rendimiento sin regresiones funcionales.
5. Introducir primero las optimizaciones demostrablemente seguras.

## Fases

### Fase 1 — Diagnóstico y benchmarks

- Harness reproducible en [`benchmarks/run.py`](../../benchmarks/run.py).
- Casos: bucle entero, Fibonacci recursivo y bucle de coma flotante.
- Salida: tiempos por ejecución, mediana, mínimo y ratio frente a Luna.
- Resultado guardado en `benchmarks/results/latest.json` (ignorado por git).

Línea base medida en el commit `989775c` (Luna `0.2.5`), con 5 ejecuciones y 1 calentamiento:

| Caso | Luna | Python | C |
| --- | --- | --- | --- |
| `loop` (20M) | 2.63 s | 2.67 s | 0.018 s |
| `fib(28)` | 0.097 s | 0.048 s | 0.0008 s |
| `math` (50M) | 7.75 s | 6.85 s | 0.16 s |

Luna empata con Python en enteros, pero va ~2x más lento en llamadas recursivas y ~13% peor en coma flotante. Esa es la referencia que debe superar 0.3.

### Fase 2 — Runtime optimizado

- Fast paths numéricos para enteros y floats en `OP_ADD`, `OP_SUB`, `OP_MUL`, `OP_DIV` y comparaciones.
- Reducción de comprobaciones de tipo repetidas dentro del bucle de despacho.
- Acceso directo a registros y variables locales.
- Menos llamadas internas entre `vm`, `arithmetic` y `compare`.

### Fase 3 — Bytecode optimizado

- Constant folding y propagación de constantes.
- Eliminación de código muerto y saltos redundantes.
- Peephole optimization sobre secuencias de instrucciones.
- Todas las transformaciones deben conservar la semántica y la versión de bytecode.

### Fase 4 — Memoria y estabilidad

- Reutilización de valores temporales y reducción de copias.
- Menos asignaciones en operaciones aritméticas y comparaciones.
- Pruebas de casos límite: desbordamiento, NaN, recursión profunda y cierre de upvalues.
- Sin sustituir el recolector de basura sin medir antes su impacto.

### Fase 5 — Validación y lanzamiento

- Ejecutar la suite completa con la misma configuración de la Fase 1.
- Documentar mejoras reales, sin prometer cifras no medidas.
- Publicar `0.3.0` solo si no hay regresiones funcionales.

## Objetivos de ingeniería

| Benchmark | Objetivo para 0.3 |
| --- | --- |
| Bucles | Mejorar claramente el resultado actual |
| Matemáticas | Superar consistentemente a Python |
| Fibonacci | Reducir el coste de llamadas y upvalues |
| Variables | Reducir el coste de acceso |
| Funciones | Reducir el overhead por llamada |
| Memoria | Reducir asignaciones innecesarias |

## Fuera de alcance en 0.3

- JIT.
- Reescribir el lenguaje o cambiar la sintaxis.
- Nuevas características que no aporten rendimiento.
- Cambios de ABI o de formato de bytecode sin necesidad medible.

La línea base y cada avance se registrarán con el mismo comando:

```bash
python3 benchmarks/run.py --runs 5
```
