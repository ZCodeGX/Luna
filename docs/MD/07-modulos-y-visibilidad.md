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

El cargador mantiene una caché de módulos, detecta ciclos y ejecuta los módulos `.luna` estableciendo la global `__exports__`. Los módulos nativos se registran con `luna_register_module`.

## Visibilidad

El modificador `pub` se admite en `fn`, `let`, `const` y `mod`:

```luna
pub const VERSION = "0.1.0"

pub fn valor()
    return 42
```

El sistema comprueba la presencia de los símbolos en la tabla `exports` y permite exportar símbolos con `modules_export_symbol`.

> [!WARNING]
> `import` y `mod` están en la gramática y el cargador sabe resolverlos, pero el compilador actual no emite código para estas sentencias.

Continúa con [manejo de errores](08-manejo-de-errores.md).
