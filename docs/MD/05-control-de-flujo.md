# 05 — Control de flujo

## if / else if / else

Los condicionales usan la verdad del programa: solo `nil` y `false` son falsy.

```luna
let x = 15

if x > 20:
    print("grande")
else if x > 10:
    print("mediano")
else:
    print("pequeño")
```

Los `else if` se recogen en una lista plana en el AST, no como condicionales anidados.

## while

```luna
let n = 3

while n > 0:
    print(n)
    n = n - 1
```

La condición se evalúa antes de cada iteración.

## for

La gramática prevé:

```luna
for clave, valor in coleccion:
    print(clave, valor)
```

> [!WARNING]
> `for` está implementado en el lexer, el parser y el AST, pero el compilador emite `for loops not yet implemented`.

## break y continue

```luna
while true:
    break
```

> [!WARNING]
> `break` y `continue` se parsean como sentencias, pero el compilador aún no genera código para ellas.

## Bloques

Todo bloque requiere una nueva línea y una indentación mayor. Se admite `:` opcional antes del salto de línea:

```luna
if true::
    print("uno")
```

Continúa con [funciones](06-funciones.md).
