# 06 — Funciones

## Declaración

```luna
fn suma(a, b):
    return a + b
```

La sintaxis es `fn IDENT ( [parámetros] )` seguida de un bloque. Los parámetros son identificadores separados por comas.

## Llamadas

```luna
print(suma(2, 3))
```

Los argumentos se evalúan y se separan por comas. Los parámetros omitidos se rellenan con `nil`:

```luna
fn f(a):
    print(a)

f()  # imprime nil
```

## retorno

`return` acepta una expresión opcional:

```luna
fn nada():
    return

fn algo():
    return 42
```

Sin valor, el resultado es `nil` según el protocolo de la VM.

## Ámbito y closures

El compilador mantiene ámbitos con variables locales y upvalues. La resolución busca primero en el ámbito actual, después en el padre inmediato como upvalue y, por último, en globals.

La gramática admite lambdas anónimas y la VM tiene opcodes para closures (`OP_CLOSURE`, `GET_UPVALUE`, `SET_UPVALUE`), pero el compilador aún no genera código para el nodo lambda.

## Funciones nativas

Desde C se registran con `luna_register_function`. Una función nativa devuelve el número de resultados que deja en la pila; un valor negativo indica error y el mensaje debe estar en el tope de la pila.

La única función nativa registrada por defecto es `print`:

```luna
print("hola", 42, true, nil)
```

Imprime los argumentos separados por espacios y termina con salto de línea. Los strings se muestran sin comillas, los booleanos como `true`/`false`, `nil` como `nil` y los floats con `%g`.

Continúa con [módulos y visibilidad](07-modulos-y-visibilidad.md).
