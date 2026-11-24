# Información general

| Dato | Valor |
| --- | --- |
| Nombre | Luna |
| Descripción | Lenguaje de programación pequeño, embebible y con CLI estilo Python |
| Versión | 0.7.6 |
| ABI | 1 |
| Licencia | GPL-3.0 |
| Compilador requerido | C11 |
| `luna.py` requiere | Python 3 (sistema de build 0.2.5) |
| Extensión de fuente | `.luna` |
| Extensión de bytecode | `.luac` |
| Encabezados públicos | `include/luna/` |

## Estado

La sintaxis y la API C pueden cambiar entre versiones. El runtime y la biblioteca estándar siguen en desarrollo: no se recomienda el uso en producción.

## Formas de ejecución

```bash
./luna.py build
./build/luna programa.luna
./build/luna -c 'let x = 1'
./build/luna
```

## Superficie disponible hoy

Además de los operadores y las estructuras básicas, Luna registra la biblioteca estándar completa: globales `len`, `type`, `str`, `int`, `float` e `input`, módulos `math`, `string`, `list`, `table`, `io`, `os`, `time`, `path` y `ffi`, y la función `print`. La CLI expone `argv` como global.

`import`, `mod`, `pub`, listas, tablas, indexación, asignaciones compuestas, `try`/`catch`, `for`, `break`, `continue` y las lambdas anónimas funcionan de extremo a extremo. `+` para strings sigue sin estar disponible. FFI con `extern` está disponible desde 0.7.6. Consulta la [tabla de estado](../../MD/11-estado-de-implementacion.md).

## Enlaces útiles

- [Inicio rápido](../../MD/01-inicio-rapido.md)
- [Referencia del lenguaje](../../MD/README.md)
- [Documentación web](../../Web/index.html)
- [Gramática formal](../../../luna.ebnf)
- [Licencia](licencia.md)
- [Estructura del proyecto](estructura-del-proyecto.md)
