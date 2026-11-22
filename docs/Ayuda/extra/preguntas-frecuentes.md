# Preguntas frecuentes

## ¿Qué es Luna?

Un lenguaje de programación pequeño, embebible y con CLI estilo Python, escrito en C11.

## ¿Cómo compilo el intérprete?

```bash
./luna.py build
```

Necesitas Python 3 y un compilador C11. El binario queda en `build/luna`.

## ¿Cómo ejecuto una línea de código sin crear un archivo?

```bash
./build/luna -c 'print(1 + 2)'
```

## ¿Cómo leo un programa desde stdin?

```bash
cat programa.luna | ./build/luna -
```

## ¿Por qué mi función con `for` no funciona?

Porque `for` está en la gramática y el parser lo acepta, pero el compilador aún no genera código para él. Lo mismo ocurre con `break` y `continue`. Consulta la [tabla de estado](../../MD/11-estado-de-implementacion.md).

## ¿Por qué `+` no concatena cadenas?

El operador `+` es aritmético. Usar dos strings con `+` produce un error de tipo. La biblioteca estándar todavía no incluye funciones de cadenas.

## ¿Puedo usar comillas simples?

No. Los literales de cadena usan comillas dobles. Un `'` se considera error léxico.

## ¿Por qué me sale error al indentar con tabuladores?

Luna exige espacios para la indentación. Los tabs en la indentación son un error.

## ¿`const` evita que reasigne una variable?

Todavía no. El compilador guarda la marca `const`, pero no comprueba las reasignaciones.

## ¿Qué funciones trae la biblioteca estándar?

Por ahora solo `print`. La CLI añade la global `argv`.

## ¿Cómo incrusto Luna en mi programa C?

Incluye `luna/luna.h`, crea un estado con `luna_new(NULL)` y ejecuta código con `luna_dostring` o `luna_dofile`. Hay un ejemplo mínimo en la [API C](../../MD/10-api-c.md).

## ¿Bajo qué licencia está?

GPL-3.0. Consulta [licencia](../informacion/licencia.md).

## ¿Puedo usar Luna en producción?

No se recomienda. La sintaxis y la API C pueden cambiar y el runtime sigue en desarrollo.
