# 07 — Módulos y visibilidad

`import`, `mod` y `pub` funcionan de extremo a extremo en `0.7.6`: el compilador genera la tabla de exports y `import` enlaza esa tabla al nombre local.

## Declarar un módulo

```luna
mod mi_modulo
```

La forma pública es `pub mod mi_modulo`. `mod` es una declaración de una línea y **no** lleva `:`; los bloques (`fn`, `if`, `while`, `try`, `catch`) sí lo exigen.

## Importar

```luna
import utilidades
import utilidades.texto
```

La ruta es una secuencia de identificadores separados por puntos.

## Ejemplo completo

Guarda `saludo.luna`:

```luna
mod saludo

pub let VERSION = 1

pub fn hola():
    return "Hola desde saludo"
```

Guarda `main.luna` en el mismo directorio:

```luna
import saludo

print(saludo.hola())
print(saludo.VERSION)
```

Salida:

```text
Hola desde saludo
1
```

## Resolución de archivos

Para `import a.b.c` se intenta, en este orden:

1. `<base>/a/b/c/mod.luna`
2. `<base>/a/b/c.luna`

Se buscan las rutas registradas mediante la API, el directorio del archivo actual y el directorio de trabajo. Si se detecta un ciclo, se emite el error `circular import detected: '<nombre>'`.

## Caché y exports

El cargador mantiene una caché de módulos, detecta ciclos y ejecuta los módulos `.luna`. El compilador crea una tabla de exports para cada módulo con símbolos públicos y la publica en la global `__exports__` al terminar; `import` enlaza esa tabla al nombre local. Los módulos nativos se registran con `luna_register_module`.

## Visibilidad

El modificador `pub` se admite en `fn`, `let`, `const` y `mod`:

```luna
pub const VERSION = "0.7.6"

pub fn valor():
    return 42
```

Cada declaración `pub` escribe el símbolo en la tabla de exports del módulo. Desde otro archivo se accede con `modulo.VERSION` o `modulo.valor()`.

## Módulos nativos y FFI

La biblioteca estándar incluye módulos escritos en Luna (`math`, `string`, `list`, `table`, `io`, `os`, `time`, `path`) y envolturas de primitivas C con prefijo `_native`. El módulo `ffi` es una fachada sobre `ffi_native`; su uso se explica en [FFI](15-ffi.md).

Continúa con [manejo de errores](08-manejo-de-errores.md).
