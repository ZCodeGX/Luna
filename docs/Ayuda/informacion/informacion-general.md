# Información general

| Dato | Valor |
| --- | --- |
| Nombre | Luna |
| Descripción | Lenguaje de programación pequeño, embebible y con CLI estilo Python |
| Versión | 0.2.5 |
| ABI | 1 |
| Licencia | GPL-3.0 |
| Compilador requerido | C11 |
| build requiere | Python 3 |
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

Además de los operadores y estructuras básicas, la única función nativa registrada es `print`. La CLI expone `argv` como global. El resto de la biblioteca está en construcción según la [tabla de estado](../../MD/11-estado-de-implementacion.md).

## Enlaces útiles

- [Referencia del lenguaje](../../MD/README.md)
- [Documentación web](../../Web/index.html)
- [Gramática formal](../../../luna.ebnf)
- [Licencia](licencia.md)
- [Estructura del proyecto](estructura-del-proyecto.md)
