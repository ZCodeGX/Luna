# 15 — FFI

La FFI (Foreign Function Interface) permite llamar funciones C desde Luna. Está disponible en `0.7.6` y se apoya en `dlopen`/`LoadLibrary`: no hace falta compilar una extensión para cada biblioteca.

## Sintaxis `extern`

```luna
extern nombre("librería", "firma")
```

`nombre` es el identificador que se usa en Luna y también el símbolo C por defecto. La librería y la firma son literales de cadena. `extern` es azúcar sintáctico:

```luna
extern abs("c", "int(int)")
```

equivale a:

```luna
let abs = __ffi_declare("abs", "c", "int(int)")
```

Ejemplo completo, tomado de la regresión `tests/regression/ffi.luna`:

```luna
extern strlen("c", "int(str)")
extern abs("c", "int(int)")
extern pow("m", "float(float,float)")

print(strlen("luna"))
print(abs(-7))
print(pow(2.0, 3.0))
```

Salida:

```text
4
7
8
```

`strlen` recibe una cadena C, `abs` un entero y `pow` dos floats. Fíjate en que los argumentos de `pow` son `2.0` y `3.0`: la firma exige floats.

## Firma y tipos

La firma es `"retorno(arg1,arg2,...)"`. El retorno `void` no produce valor utilizable en Luna.

| Firma | Tipo C | Uso desde Luna |
| --- | --- | --- |
| `void` | `void` | Funciones sin retorno |
| `int`, `i64` | entero de 64 bits | Números enteros |
| `float`, `f64`, `double` | `double` | Números con decimales |
| `str`, `string`, `cstr` | `const char *` | Cadenas terminadas en cero |
| `ptr`, `pointer` | puntero opaco | Valores manejados como enteros/punteros |

Ejemplos de firma: `"int(int)"`, `"float(float,float)"`, `"int(str)"`, `"void(ptr)"`.

## Alias de librería

| Valor | Significado |
| --- | --- |
| `""`, `"c"`, `"libc"`, `"self"`, `"current"` | Proceso actual o biblioteca C del sistema |
| `"m"`, `"libm"` | Intenta `libm.so.6` y `libm.so` |
| Ruta, por ejemplo `"./milibreria"` | Carga esa biblioteca; también se prueba añadiendo la extensión de plataforma |

La extensión se consulta con `ffi.extension()`.

## Módulo `ffi`

`import ffi` expone cuatro funciones. Es equivalente a importar la fachada `Lib/ffi/mod.luna` sobre el módulo nativo `ffi_native`.

| Función | Uso |
| --- | --- |
| `ffi.declare(nombre, librería, firma)` | Declara un símbolo con el mismo nombre en Luna y en C |
| `ffi.declare_symbol(nombre, símbolo, librería, firma)` | Permite que el nombre Luna y el símbolo C difieran |
| `ffi.load(librería)` | Carga una biblioteca; devuelve `true` si lo consigue |
| `ffi.extension()` | Devuelve `.so`, `.dylib` o `.dll` según la plataforma |

Ejemplo:

```luna
import ffi

let abs_local = ffi.declare("abs", "c", "int(int)")
let pow_local = ffi.declare_symbol("mi_pow", "pow", "m", "float(float,float)")

print(ffi.load("c"))
print(ffi.extension())
print(abs_local(-9))
print(pow_local(2.0, 4.0))
```

Salida en Linux:

```text
true
.so
9
16
```

En macOS la segunda línea es `.dylib` y en Windows `.dll`.

## Límites

| Límite | Valor |
| --- | --- |
| Parámetros declarados en una firma | 8 como máximo |
| Argumentos en una llamada | 4 como máximo |
| Mezcla de floats con enteros, punteros o strings | No permitida |
| ABI | Solo C. En C++ exporta con `extern "C"` o usa `declare_symbol` con el símbolo decorado |
| Estructuras por valor y funciones variádicas | Fuera de esta primera versión |

Si necesitas iterar sobre los argumentos de una llamada, usa `while`; `for` sigue sin estar compilado.

## Errores habituales

Falta la firma:

```text
File "tests/parser_diag/fixtures/extern_syntax.luna", line 1
  extern foo("c")
                ^
SyntaxError: expected ',' but found RPAREN
```

Firma que mezcla tipos:

```text
ffi: mixed float/integer arguments are not supported
```

Más de cuatro argumentos en la llamada:

```text
ffi: at most 4 arguments are supported
```

Símbolo no encontrado:

```text
ffi: symbol not found
```

También pueden aparecer `ffi: invalid signature '...'` y `ffi: cannot load '...': ...`. En ese caso revisa la firma, el alias y que la biblioteca esté instalada.

## Relación con la API C

`luna_new(NULL)` registra automáticamente `__ffi_declare` y el módulo `ffi_native`, por lo que no hay que hacer nada especial al incrustar Luna. Consulta la [API C](10-api-c.md), el [estado de implementación](11-estado-de-implementacion.md) y las [recetas](17-recetas.md).

Volver al [índice de documentación](README.md).
