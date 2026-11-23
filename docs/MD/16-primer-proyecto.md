# 16 — Primer proyecto: Mini agenda

Este proyecto reúne en un solo archivo las piezas básicas de Luna: listas, tablas, funciones, `if`, `while`, `try`/`catch` y biblioteca estándar. Es ejecutable con Luna `0.7.6`.

## Código completo

Guarda este contenido como `agenda.luna`:

```luna
import list
import string

fn nueva_tarea(titulo, prioridad):
    return { titulo = titulo, prioridad = prioridad, hecha = false }

fn mostrar(tareas):
    let i = 0
    while i < len(tareas):
        let tarea = tareas[i]
        let estado = "pendiente"
        if tarea.hecha:
            estado = "hecha"
        print(i, tarea.titulo, string.upper(tarea.prioridad), estado)
        i += 1

fn marcar(tareas, indice):
    if indice < 0 or indice >= len(tareas):
        throw "índice fuera de rango"
    let tarea = tareas[indice]
    tarea.hecha = true

fn main():
    let tareas = []
    list.push(tareas, nueva_tarea("comprar leche", "alta"))
    list.push(tareas, nueva_tarea("leer", "media"))

    print("Agenda")
    mostrar(tareas)

    try:
        marcar(tareas, 0)
        marcar(tareas, 9)
    catch error:
        print("error:", error)

    print("Estado final")
    mostrar(tareas)

main()
```

Ejecútalo desde la raíz del repositorio (con `Lib/` accesible):

```bash
./build/luna agenda.luna
```

Salida esperada:

```text
Agenda
0 comprar leche ALTA pendiente
1 leer MEDIA pendiente
error: índice fuera de rango
Estado final
0 comprar leche ALTA hecha
1 leer MEDIA pendiente
```

## Cómo está construido

| Parte | Qué hace |
| --- | --- |
| `import list`, `import string` | Cargan `list.push` y `string.upper` desde la biblioteca estándar. |
| `{ titulo = ..., prioridad = ..., hecha = false }` | Crea una tabla; `tarea.hecha` accede al campo. |
| `[]` y `list.push` | Crean la lista y añaden tareas. |
| `while i < len(tareas)` | Recorre la lista; `for` todavía no está compilado. |
| `tarea.hecha = true` | Modifica la tabla referenciada por `tareas[indice]`. |
| `throw` y `catch error:` | `marcar` lanza un error y `main` lo captura sin abortar. |
| `i += 1` | Asignación compuesta sobre una variable local. |

Fíjate en los `:`: los exigen `fn`, `if`, `while`, `try` y `catch`. `mod nombre`, visto más abajo, es una declaración de una línea y no lleva `:`.

## Variante interactiva con `input`

`input(mensaje)` escribe el mensaje sin salto de línea, lee una línea de entrada y devuelve un string.

```luna
import list

let titulo = input("Tarea: ")
let tareas = []
list.push(tareas, titulo)
print(tareas[0])
```

Sesión de ejemplo:

```text
Tarea: escribir informe
escribir informe
```

## Ejercicios

### 1. Contar tareas pendientes

Problema: devolver cuántas tareas tienen `hecha` igual a `false`.

Solución, añadida al programa principal:

```luna
fn pendientes(tareas):
    let total = 0
    let i = 0
    while i < len(tareas):
        if tareas[i].hecha == false:
            total += 1
        i += 1
    return total
```

Con las dos tareas iniciales, `print(pendientes(tareas))` imprime `2`.

### 2. Buscar por título

Problema: devolver el índice de la primera tarea cuyo título contiene un texto.

```luna
import string

fn buscar(tareas, texto):
    let i = 0
    while i < len(tareas):
        if string.contains(tareas[i].titulo, texto):
            return i
        i += 1
    return -1
```

Con las dos tareas iniciales, `print(buscar(tareas, "leer"))` imprime `1`.

### 3. Dividirlo en módulos

Problema: reutilizar las funciones desde otro archivo. Guarda `agenda_mod.luna` con los símbolos públicos:

```luna
mod agenda_mod

pub fn nueva_tarea(titulo, prioridad):
    return { titulo = titulo, prioridad = prioridad, hecha = false }

pub fn mostrar(tareas):
    let i = 0
    while i < len(tareas):
        print(tareas[i].titulo)
        i += 1
```

Y crea `principal.luna` en el mismo directorio:

```luna
import agenda_mod
import list

let tareas = []
list.push(tareas, agenda_mod.nueva_tarea("comprar leche", "alta"))
agenda_mod.mostrar(tareas)
```

Salida:

```text
comprar leche
```

`pub` publica la función en la tabla de exports del módulo; `import agenda_mod` enlaza esa tabla al nombre local. La resolución prueba `agenda_mod/mod.luna` antes que `agenda_mod.luna`.

## Siguiente paso

Más ejemplos cortos en [recetas](17-recetas.md) y profundidad en [módulos y visibilidad](07-modulos-y-visibilidad.md).

Volver al [índice de documentación](README.md).
