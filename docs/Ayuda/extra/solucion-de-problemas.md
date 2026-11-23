# Solución de problemas

## El comando `./luna.py build` falla

- Comprueba que Python 3 esté disponible: `python3 --version`.
- Comprueba el compilador C11: `cc --version`.
- Ejecuta `./luna.py build -v` para ver los comandos exactos de compilación.
- Usa `./luna.py info` para revisar módulos y dependencias.
- Si hay artefactos obsoletos: `./luna.py clean && ./luna.py build`.

## Error: `expected ':' after ...`

Las cabeceras de bloque exigen dos puntos. Escribe `fn nombre():`, `if condicion:`, `else:`, `while condicion:`, `try:` y `catch e:`. `mod` e `import` son sentencias de una línea y no los necesitan. Ejemplo del diagnóstico en [diagnósticos](../../MD/13-diagnosticos.md).

## Error: `tabs are not allowed for indentation`

La indentación debe hacerse con espacios. Sustituye los tabuladores en el código fuente.

## Error: `unindent does not match any outer indentation level`

Una dedentación no coincide con ningún nivel de indentación anterior. Revisa la alineación del bloque.

## Error: `number too long`

El lexer limita la longitud de los literales numéricos: 64 caracteres para hexadecimal/binario y 128 para decimal/float. Divide o simplifica el número.

## Error: `unexpected character` con comillas simples

Luna solo admite cadenas entre comillas dobles. Cambia `'texto'` por `"texto"`.

## Error: `attempt to perform arithmetic on a nil value`

Normalmente significa que una función se llamó con menos argumentos de los necesarios; los parámetros faltantes valen `nil`. Revisa la aridad de la llamada.

## Error: `complex assignment targets not yet supported`

Ya no debería aparecer para casos normales: las asignaciones a `lista[i]`, `tabla[clave]` y `objeto.campo` están implementadas. Si ves este mensaje, comprueba que el objetivo sea una de esas formas y no una expresión arbitraria.

## Error: `for loops not yet implemented`

`for`, `break` y `continue` no están disponibles. Sustituye el bucle por `while` con una condición y una variable de control. Consulta el [estado de implementación](../../MD/11-estado-de-implementacion.md).

## Error: `unsupported expression node`

La expresión utiliza una construcción que el compilador no emite, como una lambda anónima `fn(...)`. Las listas y tablas literales sí funcionan. Consulta las [recetas](../../MD/17-recetas.md).

## Error de tipo al concatenar strings

`+` no concatena cadenas. Importa `string` y usa `string.concat(a, b)` o `string.join(separador, lista)`.

## Error: `circular import detected`

Existe un ciclo entre módulos. Aunque `import` sí se compila, el cargador detecta estos casos. Reestructura las dependencias.

## Error: `ImportError` al importar un módulo

Comprueba que el archivo esté en alguna ruta registrada: directorio del archivo actual, directorio de trabajo o rutas añadidas con `luna_add_path`. Para `import paquete.mod` se prueba `paquete/mod/mod.luna` y luego `paquete/mod.luna`, en ese orden; revisa la [resolución de módulos](../../MD/07-modulos-y-visibilidad.md).

## No encuentro un símbolo exportado

Los símbolos deben declararse con `pub` en el módulo. La tabla de exports se genera al compilar y `import nombre` enlaza esa tabla. Consulta el [ejemplo de módulo](../../MD/17-recetas.md).

## Errores de FFI

| Mensaje | Causa probable |
| --- | --- |
| `ffi: invalid signature '...'` | Falta `retorno(arg1,...)` o aparece un tipo desconocido. |
| `ffi: cannot load '...'` | La biblioteca no existe o el alias es incorrecto. |
| `ffi: symbol not found` | El nombre C no coincide; usa `ffi.declare_symbol`. |
| `ffi: mixed float/integer arguments are not supported` | La firma mezcla floats con enteros, punteros o strings. |
| `ffi: at most 4 arguments are supported` | La llamada supera cuatro argumentos. |

La declaración correcta es, por ejemplo:

```luna
extern abs("c", "int(int)")

print(abs(-7))
```

Salida: `7`. Más detalles en [FFI](../../MD/15-ffi.md).

## `input` no muestra el mensaje esperado

`input(mensaje)` escribe el mensaje sin salto de línea, lee una línea y devuelve un string. Si la entrada llega de una tubería, el mensaje y la salida pueden aparecer juntos en la terminal.

## El binario no muestra colores

Los colores dependen de la terminal y de las variables `NO_COLOR` y `FORCE_COLOR`. También puedes usar `--color` o `--no-color`.

## El bytecode no se carga

La versión de bytecode es `0.1` con magia `LUNA`. Un cambio de versión invalida los archivos `.luac`; recompila desde el fuente `.luna`.

## Dónde pedir ayuda

Revisa primero las [preguntas frecuentes](preguntas-frecuentes.md), el [inicio rápido](../../MD/01-inicio-rapido.md) y la [documentación del lenguaje](../../MD/README.md). Para errores del proyecto, consulta el repositorio y su registro de incidencias.
