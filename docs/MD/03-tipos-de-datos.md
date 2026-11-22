# 03 — Tipos de datos

Los tipos del runtime son `nil`, `boolean`, `integer`, `float`, `string`, `list`, `table`, `function` y `userdata`.

## nil

```luna
let vacio = nil
```

Solo `nil` y `false` son falsy. Cualquier otro valor —incluidos `0` y `""`— es truthy.

## Booleanos

```luna
let activo = true
let inactivo = false
```

## Números

Enteros decimales, hexadecimales (`0x` / `0X`) y binarios (`0b` / `0B`):

```luna
let a = 42
let b = 0x2A
let c = 0b101010
```

Flotantes con punto decimal y/o exponente:

```luna
let d = 3.14
let e = 1e-3
let f = 2.5E+4
```

El lexer rechaza números demasiado largos con el mensaje `number too long`.

## Cadenas

Se usan **comillas dobles**; no existen literales con comilla simple:

```luna
let saludo = "hola"
let multilinea = "línea\nsegunda"
```

Escapes admitidos: `\n`, `\t`, `\r`, `\0`, `\\` y `\"`. Un escape desconocido se conserva literalmente como `\` seguido del carácter. No se permiten saltos de línea literales dentro de una cadena.

## Listas

```luna
let numeros = [1, 2, 3,]
```

La coma final es opcional. El parser genera el nodo `NODE_LIST`; consulta el estado de implementación para su disponibilidad en código compilado.

## Tablas

```luna
let persona = { nombre = "Ada", edad = 36, }
```

Las claves son identificadores y la coma final es opcional.

## Funciones y userdata

Las funciones se crean con `fn` o se registran desde C. `userdata` es un valor opaco gestionado desde la API C.

Continúa con [operadores](04-operadores.md).
