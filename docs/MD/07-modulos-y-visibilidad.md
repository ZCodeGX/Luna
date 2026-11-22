# 07 — Módulos y visibilidad

## Declarar un módulo

```luna
mod mi_modulo
```

La forma pública es `pub mod mi_modulo`.

## Importar

```luna
import utilidades
import utilidades.texto
```

La ruta es una secuencia de identificadores separados por puntos.

## Resolución de archivos

Para `import a.b.c` se intenta, en este orden:

1. `<base>/a/b/c/mod.luna`
2. `<base>/a/b/c.luna`

Se buscan las rutas registradas mediante la API, el directorio del archivo actual y el directorio de trabajo. Si se detecta un ciclo, se emite el error `circular import detected: '<nombre>'`.

## Caché y exports

El cargador mantiene una caché de módulos, detecta ciclos y ejecuta los módulos `.luna`. El compilador crea una tabla de exports para cada módulo con símbolos públicos y la publica en la global `__exports__` al terminar; `import` enlaza esa tabla al nombre local. Los módulos nativos se registran con `luna_register_module`.

## Visibilidad

El modificador `pub` se admite en `fn`, `let` y `const`:

```luna
pub const VERSION = "0.1.0"

pub fn valor()
    return 42
```

Cada declaración `pub` escribe el símbolo en la tabla de exports del módulo. Desde otro archivo se accede con `modulo.VERSION` o `modulo.valor()`.

```luna
import utilidades

print(utilidades.VERSION)
print(utilidades.valor())
```

Continúa con [manejo de errores](08-manejo-de-errores.md).
