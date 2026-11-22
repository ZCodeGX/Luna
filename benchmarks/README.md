# Benchmarks de Luna

Harness reproducible para la Fase 1 de [Luna 0.3](../docs/MD/12-rendimiento-0-3.md).

## Uso

```bash
python3 benchmarks/run.py --runs 5
```

Opciones principales:

| Opción | Descripción |
| --- | --- |
| `--runs N` | Ejecuciones medidas por motor y caso (por defecto 5) |
| `--warmup N` | Ejecuciones previas descartadas (por defecto 1) |
| `--iterations N` | Sobrescribe `N` en `loop` y `math` |
| `--fib-n N` | Sobrescribe `N` en `fib` |
| `--cases loop,fib,math` | Selecciona casos |
| `--luna RUTA` | Binario de Luna a medir |
| `--no-build` | No compila Luna antes de medir |

## Casos

| Caso | Operación | N por defecto |
| --- | --- | --- |
| `loop` | Bucle entero con multiplicación, suma y módulo | 20.000.000 |
| `fib` | Fibonacci recursivo | 28 |
| `math` | Bucle de coma flotante | 50.000.000 |

Cada caso se ejecuta con Luna, Python y C. El harness compara la salida de los tres motores antes de aceptar la medición, descarta el calentamiento y guarda tiempos individuales, mediana, mínimo y ratio frente a Luna.

## Salida

Los resultados se escriben en `benchmarks/results/latest.json`, ignorado por git. Incluye commit de git, versiones de Luna/Python/C, configuración usada y tiempos crudos.

Para una comprobación rápida sin mediciones largas:

```bash
python3 benchmarks/run.py --runs 2 --warmup 0 --iterations 200000 --cases loop,fib
```
