# 01 — Inicio rápido

## Requisitos

- Python 3 para el sistema de build `luna.py`.
- Compilador C11 (`cc`, `gcc` o `clang`).

## Compilar

```bash
./luna.py build
```

El binario resultante se crea en `build/luna`. La compilación es incremental y tiene en cuenta cambios en fuentes y encabezados. Otras opciones:

```bash
./luna.py build -v
./luna.py clean
./luna.py info
./luna.py graph > deps.dot
```

## Ejecutar

```bash
./build/luna -c 'let x = 1'
./build/luna programa.luna
cat programa.luna | ./build/luna -
./build/luna
```

La última forma abre el REPL, con prompt `>>>`.

## Primer programa

```luna
fn main()
    let nombre = "Luna"
    print("Hola", nombre)

main()
```

## Convenciones del proyecto

- Los archivos de programa usan la extensión `.luna`.
- Los módulos se resuelven como `<ruta>/mod.luna` o `<ruta>.luna`.
- Los ejemplos ejecutables del repositorio están en `tests/`.
- El bytecode compilado usa la extensión `.luac` (magia `LUNA`, versión `0.1`).

## Siguiente paso

Continúa con [sintaxis básica](02-sintaxis-basica.md).
