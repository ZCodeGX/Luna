<p align="center">
  <img src="assets/luna-logo.svg" alt="Luna" width="360">
</p>

**Luna** es un lenguaje de programación pequeño, embebible y con CLI estilo Python.

> [!NOTE]
> Requiere un compilador C11 y Python 3 para el sistema de build `luna.py`.

## Inicio rápido

```bash
./luna.py build
./build/luna -c 'let x = 1'
./build/luna programa.luna
```

> [!TIP]
> `./build/luna` sin argumentos abre el REPL y `-` lee el programa desde stdin.

> [!IMPORTANT]
> Luna usa GPL-3.0: cualquier modificación o distribución debe mantener el código abierto y compartirse bajo la misma licencia.

> [!WARNING]
> La sintaxis y la API C aún pueden cambiar entre versiones.

> [!CAUTION]
> No lo uses en producción: el runtime y la biblioteca estándar siguen en desarrollo.

## Licencia

[GPL-3.0](LICENSE). Las contribuciones se aceptan bajo los mismos términos.
