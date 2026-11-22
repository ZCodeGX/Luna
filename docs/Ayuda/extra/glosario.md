# Glosario

| Término | Definición |
| --- | --- |
| **AST** | Árbol sintáctico abstracto. Representación intermedia producida por el parser. |
| **Bytecode** | Instrucciones compactas ejecutadas por la VM. Se serializan en archivos `.luac`. |
| **Closure** | Función que captura variables de su ámbito circundante mediante upvalues. |
| **DEDENT** | Token que marca la salida de un bloque indentado. |
| **Globals** | Tabla de variables accesibles desde cualquier ámbito. |
| **INDENT** | Token que marca la entrada a un bloque indentado. |
| **LunaState** | Estado opaco del intérprete: pila, globals, manejo de errores y directorio de trabajo. |
| **LunaRuntime** | Runtime compartido entre estados; controla memoria y límites de ejecución. |
| **Opcode** | Código de operación de la VM (por ejemplo `OP_ADD` o `OP_CALL`). |
| **Parser Pratt** | Técnica de análisis de expresiones basada en precedencia, usada por Luna. |
| **REPL** | Bucle interactivo de lectura, evaluación e impresión (`>>>`). |
| **Upvalue** | Variable capturada de un ámbito exterior. |
| **Userdata** | Valor opaco creado desde C y gestionado por el GC. |
| **Truthy / falsy** | Valores que se comportan como verdadero o falso en condiciones. En Luna solo `nil` y `false` son falsy. |
| **dostring / dofile** | Funciones de la API C que ejecutan código desde una cadena o un archivo. |
| **luac** | Extensión de los archivos de bytecode compilado. |

Términos relacionados con la arquitectura: [estructura del proyecto](../informacion/estructura-del-proyecto.md).
