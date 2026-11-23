# 06 — Funciones

## Declaración

```luna
fn suma(a, b):
    return a + b

print(suma(2, 3))
```

Salida:

```text
5
```

La sintaxis es `fn IDENT ( [parámetros] ) :` seguida de un bloque indentado. Los parámetros son identificadores separados por comas.

## Llamadas

Los argumentos se evalúan y se separan por comas. Los parámetros omitidos se rellenan con `nil`:

```luna
fn f(a):
    print(a)

f()
```

Salida:

```text
nil
```

## Retorno

`return` acepta una expresión opcional. Sin valor, el resultado es `nil`:

```luna
fn nada():
    return

fn algo():
    return 42

print(nada())
print(algo())
```

Salida:

```text
nil
42
```

## Ámbito y closures

El compilador mantiene ámbitos con variables locales y upvalues. La resolución busca primero en el ámbito actual, después en el padre inmediato como upvalue y, por último, en globals. Las funciones pueden declararse dentro de otras funciones:

```luna
fn make():
    let x = 10
    fn get():
        return x
    fn set(v):
        x = v
    return [get, set]

let p = make()
print(p[0]())
p[1](99)
print(p[0]())
```

Salida:

```text
10
99
```

`get` y `set` comparten la variable capturada `x`. Las lambdas anónimas `fn(...)`, en cambio, todavía no se compilan.

## Funciones nativas y stdlib

Desde C se registran con `luna_register_function`. Una función nativa devuelve el número de resultados que deja en la pila; un valor negativo indica error y el mensaje debe estar en el tope de la pila.

`print` **no** es la única función disponible. Al crear un estado se registra la biblioteca estándar: globales como `len`, `type`, `str`, `int`, `float` e `input`, y módulos como `math`, `string`, `list`, `table`, `io`, `os`, `time`, `path` y `ffi`.

```luna
import math
import string

print(math.sqrt(16))
print(string.upper("luna"))
print(len("hola"))
```

Salida:

```text
4
LUNA
4
```

`print` separa sus argumentos con espacios y termina con salto de línea. Los strings se muestran sin comillas, los booleanos como `true`/`false`, `nil` como `nil` y los floats con `%g`. La lista completa está en [biblioteca estándar](14-biblioteca-estandar.md); las declaraciones de funciones C en Luna están en [FFI](15-ffi.md).

Continúa con [módulos y visibilidad](07-modulos-y-visibilidad.md).
