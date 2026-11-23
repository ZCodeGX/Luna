# 01 — Inicio rápido

Luna `0.7.6` se compila desde el repositorio con Python 3 y un compilador C11. Esta guía es la ruta más corta para escribir y ejecutar tu primer programa.

## Requisitos y compilación

```bash
python3 --version
cc --version
./luna.py build
```

Desde la raíz del repositorio, `./luna.py build` genera `build/luna`. Para verificar la instalación:

```bash
./build/luna -V
./luna.py test
```

También puedes instalar el binario con [`install.sh`](../../install.sh) en Linux/macOS o [`install.ps1`](../../install.ps1) en Windows. En Linux/macOS, `./install.sh --info` muestra versión instalada, versión destino y rutas antes de instalar. Para actualizar, vuelve a ejecutar `./install.sh`; para desinstalar, usa [`uninstall.sh`](../../uninstall.sh) (con `--dry-run` para previsualizar y `--keep-source` para conservar el repositorio).

## Cuatro formas de ejecutar

| Forma | Comando | Uso típico |
| --- | --- | --- |
| Cadena | `./build/luna -c 'let x = 1'` | Pruebas de una línea |
| Archivo | `./build/luna programa.luna` | Programas completos |
| Entrada estándar | `printf 'print("hola")\n' | ./build/luna -` | Tuberías desde otros programas |
| REPL | `./build/luna` | Explorar sin crear archivos |

El REPL muestra el prompt `>>>`. En los modos `-c` y `-`, la global `argv` contiene el indicador en el índice 1. Consulta la [referencia de CLI](09-cli-y-build.md) para todas las opciones.

## Tu primer programa

Crea `hola.luna` con este contenido:

```luna
let nombre = "Luna"
print("Hola,", nombre)
```

Ejecútalo:

```bash
./build/luna hola.luna
```

Salida:

```text
Hola, Luna
```

`print` separa sus argumentos con espacios y añade un salto de línea. Los strings van siempre entre comillas dobles.

## Un programa con función

Crea `cuadrado.luna`:

```luna
fn cuadrado(x):
    return x * x

print(cuadrado(5))
```

Salida:

```text
25
```

La cabecera `fn` termina en `:` y el cuerpo se indenta con espacios. Lo mismo exigen `if`, `else`, `while`, `try` y `catch`. Los tabuladores en la indentación son un error.

## Si algo falla

Si falta el `:`, el diagnóstico es parecido a:

```text
SyntaxError: expected ':' after function signature

    help: insert ':' at the end of the line
```

Más ejemplos en [diagnósticos](13-diagnosticos.md) y en [solución de problemas](../Ayuda/extra/solucion-de-problemas.md).

## Siguiente paso

Sigue la ruta guiada:

1. [Sintaxis básica](02-sintaxis-basica.md) y [tipos de datos](03-tipos-de-datos.md).
2. [Control de flujo](05-control-de-flujo.md) y [funciones](06-funciones.md).
3. [Primer proyecto](16-primer-proyecto.md) para juntar listas, tablas, funciones y errores.
4. [FFI](15-ffi.md) y [biblioteca estándar](14-biblioteca-estandar.md) para ampliar el alcance.

Volver al [índice de documentación](README.md).
