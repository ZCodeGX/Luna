# 02 — Sintaxis básica

## Identificadores

Un identificador comienza con letra o guion bajo y continúa con letras, dígitos o guiones bajos:

```text
[A-Za-z_][A-Za-z0-9_]*
```

## Palabras reservadas

No pueden usarse como identificadores:

```text
and  break  catch  const  continue  else   false  fn    for
if   import in     let    mod       nil     not    or    pub:
return   throw    true    try       while
```

## Comentarios

El carácter `#` inicia un comentario que llega hasta el final de la línea. No hay comentarios de bloque.

```luna
# comentario
let x = 1  # comentario al final de línea
```

## Bloques e indentación

Los bloques se delimitan por indentación con **espacios**. Los tabuladores en la indentación producen un error, al igual que una dedentación inconsistente. La profundidad máxima es 256 niveles.

Dentro de paréntesis, corchetes y llaves se ignoran los saltos de línea de indentación, por lo que se pueden repartir expresiones en varias líneas.

El dos puntos antes de un bloque es opcional; ambas formas son válidas:

```luna
fn saluda(a):
    print(a)

fn despide(a):
    print(a)
```

## Sentencias

Las sentencias simples terminan con salto de línea. El carácter `;` se trata como salto de línea. Entre las sentencias simples están `let`, `const`, `return`, `import`, `break`, `continue`, `throw` y las expresiones.

## Declaraciones

```luna
let x = 10
const pi = 3.14159
```

`pub` marca una declaración como pública para exportación:

```luna
pub let version = 1
pub const nombre = "luna"
pub fn principal():
    return 0
```

> [!NOTE]
> `const` se registra en el ámbito y el compilador rechaza cualquier reasignación con un error que incluye archivo, línea y columna.

Continúa con [tipos de datos](03-tipos-de-datos.md).
