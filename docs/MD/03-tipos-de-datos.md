# 03 — Tipos de datos

Los tipos del runtime son `nil`, `boolean`, `integer`, `float`, `string`, `list`, `table`, `function` y `userdata`.

Puedes comprobarlos con `type`:

```luna
print(type(nil))
print(type(true))
print(type(42))
print(type(3.5))
print(type("hola"))
print(type([]))
print(type({}))
print(type(print))
```

Salida:

```text
nil
boolean
integer
float
string
list
table
function
```

## nil

```luna
let vacio = nil
print(vacio)
```

Salida: `nil`.

Solo `nil` y `false` son falsy. Cualquier otro valor —incluidos `0` y `""`— es truthy:

```luna
if nil:
    print("sí")
else:
    print("no")

if 0:
    print("cero truthy")
```

Salida:

```text
no
cero truthy
```

## Booleanos

```luna
let activo = true
let inactivo = false
print(activo)
print(inactivo)
```

Salida:

```text
true
false
```

## Números

Enteros decimales, hexadecimales (`0x` / `0X`) y binarios (`0b` / `0B`); flotantes con punto decimal y/o exponente:

```luna
let a = 42
let b = 0x2A
let c = 0b101010
let d = 3.14
let e = 1e-3

print(a)
print(b)
print(c)
print(d)
print(e)
```

Salida:

```text
42
42
42
3.14
0.001
```

El lexer rechaza números demasiado largos con el mensaje `number too long`. Los floats se imprimen con formato `%g`.

## Cadenas

Se usan **comillas dobles**; no existen literales con comilla simple:

```luna
let saludo = "hola"
let multilinea = "línea\nsegunda"

print(saludo)
print(multilinea)
```

Salida:

```text
hola
línea
segunda
```

Escapes admitidos: `\n`, `\t`, `\r`, `\0`, `\\` y `\"`. Un escape desconocido se conserva literalmente como `\` seguido del carácter. No se permiten saltos de línea literales dentro de una cadena. `+` no concatena strings: usa `string.concat` o `string.join`.

## Listas

Las listas literales están disponibles. La coma final es opcional:

```luna
let numeros = [1, 2, 3]
print(numeros[0])
numeros[1] += 5
print(numeros[1])
```

Salida:

```text
1
7
```

La indexación y las asignaciones a índices están compiladas. `len(numeros)` devuelve la longitud.

## Tablas

Las claves son identificadores y la coma final es opcional:

```luna
let persona = { nombre = "Ada", edad = 36 }
print(persona.nombre)
persona.edad += 1
print(persona["edad"])
```

Salida:

```text
Ada
37
```

Se puede acceder con `persona.edad` o `persona["edad"]` y asignar a cualquiera de las dos formas.

## Funciones y userdata

Las funciones se crean con `fn` o se registran desde C, y pueden pasarse como valores. `userdata` es un valor opaco gestionado desde la API C. Las declaraciones FFI devuelven una función envolvente que guarda el manejador nativo como userdata.

Continúa con [operadores](04-operadores.md).
