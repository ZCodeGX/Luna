# Solución de problemas

## El comando `./luna.py build` falla

- Comprueba que Python 3 esté disponible: `python3 --version`.
- Comprueba el compilador C11: `cc --version`.
- Ejecuta `./luna.py build -v` para ver los comandos exactos de compilación.
- Usa `./luna.py info` para revisar módulos y dependencias.
- Si hay un problema de artefactos obsoletos: `./luna.py clean && ./luna.py build`.

## Error: `tabs are not allowed for indentation`

La indentación debe hacerse con espacios. Sustituye los tabuladores en el código fuente.

## Error: `inconsistent dedent`

Una dedentación no coincide con ningún nivel de indentación anterior. Revisa la alineación del bloque.

## Error: `number too long`

El lexer limita la longitud de los literales numéricos: 64 caracteres para hexadecimal/binario y 128 para decimal/float. Divide o simplifica el número.

## Error: `unexpected character` con comillas simples

Luna solo admite cadenas entre comillas dobles. Cambia `'texto'` por `"texto"`.

## Error: `attempt to perform arithmetic on a nil value`

Normalmente significa que una función se llamó con menos argumentos de los necesarios; los parámetros faltantes valen `nil`. Revisa la aridad de la llamada.

## Error: `complex assignment targets not yet supported`

El compilador solo admite asignaciones a identificadores. Las asignaciones a `lista[i]` u `objeto.campo` están en la gramática pero no implementadas.

## Error: `for loops not yet implemented`

`for`, `break` y `continue` no están disponibles todavía. Usa `while` como alternativa.

## Error: `unsupported expression node`

La expresión utiliza una construcción que el compilador no emite, como literales de lista/tabla o lambdas. Consulta [estado de implementación](../../MD/11-estado-de-implementacion.md).

## Error: `circular import detected`

Existe un ciclo entre módulos. Aunque `import` aún no se compila, el cargador detecta estos casos. Reestructura las dependencias.

## Error de tipo en una comparación

`<`, `<=`, `>` y `>=` solo aceptan números o cadenas. Para igualdad usa `==` o `!=`, que funcionan con cualquier par de valores.

## El binario no muestra colores

Los colores dependen de la terminal y de las variables `NO_COLOR` y `FORCE_COLOR`. Configúralas según tu entorno.

## El bytecode no se carga

La versión de bytecode es `0.1` con magia `LUNA`. Un cambio de versión invalida los archivos `.luac`; recompila desde el fuente `.luna`.

## No encuentro un símbolo exportado

La visibilidad depende de la tabla `exports`. Los símbolos deben declararse con `pub`. Ten en cuenta que la emisión de exports desde el compilador aún está pendiente.

## Dónde pedir ayuda

Revisa primero las [preguntas frecuentes](preguntas-frecuentes.md) y la [documentación del lenguaje](../../MD/README.md). Para errores del proyecto, consulta el repositorio y su registro de incidencias.
