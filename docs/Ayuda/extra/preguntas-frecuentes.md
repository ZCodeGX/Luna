# Preguntas frecuentes

## ¿Qué es Luna?

Un lenguaje de programación pequeño, embebible y con CLI estilo Python, escrito en C11. La versión actual es `0.7.6` (ABI 1, GPL-3.0).

## ¿Cómo compilo el intérprete?

```bash
./luna.py build
```

Necesitas Python 3 y un compilador C11. El binario queda en `build/luna`. Para ejecutar las cuatro suites de pruebas: `./luna.py test`.

## ¿Cómo ejecuto una línea de código sin crear un archivo?

```bash
./build/luna -c 'print(1 + 2)'
```

## ¿Cómo leo un programa desde stdin?

```bash
cat programa.luna | ./build/luna -
```

## ¿Cómo itero sobre una lista o una tabla?

Usa `for x in lista:` o `for clave, valor in tabla:`. `break` y `continue` funcionan en `for` y en `while`. Consulta [control de flujo](../../MD/05-control-de-flujo.md).

## ¿Por qué `+` no concatena cadenas?

El operador `+` es aritmético: con dos strings produce un error de tipo. La biblioteca estándar sí incluye funciones de cadenas:

```luna
import string

print(string.concat("hola", " luna"))
print(string.join("-", ["a", "b"]))
```

Salida:

```text
hola luna
a-b
```

## ¿Puedo usar comillas simples?

No. Los literales de cadena usan comillas dobles. Un `'` se considera error léxico.

## ¿Por qué me sale error al indentar con tabuladores?

Luna exige espacios para la indentación. Los tabs en la indentación son un error. Además, las cabeceras de bloque (`fn`, `if`, `else`, `while`, `try`, `catch`) exigen `:`.

## ¿`const` evita que reasigne una variable?

Sí. El compilador comprueba las reasignaciones y falla con un mensaje que incluye archivo, línea y columna.

## ¿Funcionan los módulos, las listas y las asignaciones compuestas?

Sí. `import`, `mod`, `pub`, listas, tablas, `lista[i]`, `objeto.campo`, `+=`, `-=`, `*=` y `/=` están compilados. Hay un ejemplo completo en [primer proyecto](../../MD/16-primer-proyecto.md) y otro en [módulos](../../MD/07-modulos-y-visibilidad.md).

## ¿Qué funciones trae la biblioteca estándar?

Globales `len`, `type`, `str`, `int`, `float`, `input` y `print`, y módulos `math`, `string`, `list`, `table`, `io`, `os`, `time`, `path` y `ffi`. La lista detallada está en [biblioteca estándar](../../MD/14-biblioteca-estandar.md).

## ¿Cómo llamo a una función C?

Con `extern` o con el módulo `ffi`, disponibles desde 0.7.6:

```luna
extern abs("c", "int(int)")

print(abs(-7))
```

Salida: `7`. Consulta la [guía de FFI](../../MD/15-ffi.md).

## ¿Cómo incrusto Luna en mi programa C?

Incluye `luna/luna.h`, crea un estado con `luna_new(NULL)` y ejecuta código con `luna_dostring` o `luna_dofile`. Hay un ejemplo mínimo en la [API C](../../MD/10-api-c.md).

## ¿Bajo qué licencia está?

GPL-3.0. Consulta la [licencia](../informacion/licencia.md).

## ¿Puedo usar Luna en producción?

No se recomienda. La sintaxis y la API C pueden cambiar y el runtime sigue en desarrollo.
