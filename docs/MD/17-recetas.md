# 17 — Recetas

Recetas cortas y autocontenidas. Cada una indica el problema, el código y la salida. Todas usan la sintaxis válida de Luna `0.7.6`: las cabeceras de bloque llevan `:`.

## 1. Recursión

**Problema:** calcular un factorial.

```luna
fn factorial(n):
    if n <= 1:
        return 1
    return n * factorial(n - 1)

print(factorial(6))
```

**Salida:**

```text
720
```

## 2. Closure con estado

**Problema:** crear una función que recuerde un contador entre llamadas.

```luna
fn contador():
    let n = 0
    fn inc():
        n = n + 1
        return n
    return inc

let siguiente = contador()
print(siguiente())
print(siguiente())
```

**Salida:**

```text
1
2
```

`n` es un upvalue: la función interna `inc` captura la variable de `contador`.

## 3. Módulo con exports

**Problema:** reutilizar funciones desde otro archivo. Guarda `utiles.luna`:

```luna
mod utiles

pub let VERSION = 1

pub fn doble(x):
    return x * 2
```

Guarda `principal.luna` en el mismo directorio:

```luna
import utiles

print(utiles.VERSION)
print(utiles.doble(21))
```

**Salida:**

```text
1
42
```

`mod utiles` no lleva `:`: es una declaración de una línea, no un bloque.

## 4. Biblioteca estándar

**Problema:** duplicar una lista, sumarla y unirla en un string.

```luna
import math
import string
import list

fn doble(x):
    return x * 2

let valores = list.map([1, 2, 3], doble)
print(math.sum(valores))
print(string.join("-", valores))
```

**Salida:**

```text
12
2-4-6
```

`+` no concatena strings; para eso existe `string.concat` y `string.join`.

## 5. Tablas e indexación

**Problema:** leer y modificar campos de una tabla.

```luna
let persona = { nombre = "Ada", edad = 36 }
print(persona.nombre)
persona.edad += 1
print(persona["edad"])
```

**Salida:**

```text
Ada
37
```

## 6. FFI

**Problema:** llamar funciones de la biblioteca C y de `libm`.

```luna
extern strlen("c", "int(str)")
extern abs("c", "int(int)")
extern pow("m", "float(float,float)")

print(strlen("luna"))
print(abs(-7))
print(pow(2.0, 3.0))
```

**Salida:**

```text
4
7
8
```

Detalles, límites y errores en [FFI](15-ffi.md).

## 7. Patrón de error

**Problema:** validar datos y continuar después de un fallo.

```luna
fn dividir(a, b):
    if b == 0:
        throw "división por cero"
    return a / b

try:
    print(dividir(8, 2))
    print(dividir(8, 0))
catch error:
    print("error:", error)

print("fin")
```

**Salida:**

```text
4
error: división por cero
fin
```

`8 / 2` produce un float, que `print` muestra como `4` con formato `%g`.

Volver al [índice de documentación](README.md).
