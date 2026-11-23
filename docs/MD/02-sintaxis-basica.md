# 02 — Sintaxis básica

## Identificadores

Un identificador comienza con letra o guion bajo y continúa con letras, dígitos o guiones bajos:

```text
[A-Za-z_][A-Za-z0-9_]*
```

## Palabras reservadas

No pueden usarse como identificadores:

```text
and     break   catch   const   continue   else    extern  false
fn      for     if      import  in         let     mod     nil
not     or      pub     return  throw      true    try     while
```

`break` y `continue` están reservados pero todavía no se compilan. `extern` se usa para declarar funciones C; consulta [FFI](15-ffi.md).

## Comentarios

El carácter `#` inicia un comentario hasta el final de la línea. No hay comentarios de bloque:

```text
# comentario de línea
let x = 1  # comentario al final de línea
```

## Bloques e indentación

Los bloques se delimitan con **espacios**. Los tabuladores en la indentación producen un error, al igual que una dedentación inconsistente. La profundidad máxima es 256 niveles.

La cabecera de un bloque **siempre** termina en `:`. Esto incluye `fn`, `if`, `else if`, `else`, `while`, `try` y `catch`. Sin los dos puntos se emite `SyntaxError: expected ':' after ...`.

```luna
fn clasificar(x):
    if x > 10:
        return "grande"
    else:
        return "pequeño"

print(clasificar(42))
```

Salida:

```text
grande
```

`mod nombre`, `import nombre`, `let`, `const`, `return`, `throw` y `extern` son sentencias de una línea: no llevan `:`.

Dentro de paréntesis, corchetes y llaves se ignoran los saltos de línea de indentación, por lo que se pueden repartir expresiones en varias líneas.

## Sentencias

Las sentencias simples terminan con salto de línea; `;` se trata como salto de línea. Entre las sentencias simples están `let`, `const`, `return`, `import`, `extern`, `throw` y las expresiones.

## Declaraciones

```luna
let x = 10
const pi = 3.14159
```

`pub` marca una declaración como pública para exportación desde un módulo:

```luna
pub let version = 1
pub const nombre = "luna"
pub fn principal():
    return 0
```

> [!NOTE]
> `const` se registra en el ámbito y el compilador rechaza cualquier reasignación con un error que incluye archivo, línea y columna.

Continúa con [tipos de datos](03-tipos-de-datos.md).
