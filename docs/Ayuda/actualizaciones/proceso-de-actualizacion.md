# Proceso de actualización del sistema

Procedimiento para actualizar una copia de Luna y mantener el entorno consistente.

## 1. Verificar el estado local

```bash
git status --short
```

Si hay cambios propios, guárdalos antes de continuar. El repositorio ignora `build/`, por lo que los artefactos compilados no se versionan.

## 2. Obtener los cambios

```bash
git pull
```

Revisa el [registro de cambios](CHANGELOG.md) y las [notas de implementación](../../MD/11-estado-de-implementacion.md) para detectar cambios de sintaxis o de API.

## 3. Limpiar y recompilar

```bash
./luna.py clean
./luna.py build
```

`clean` elimina `build/`. La recompilación completa es obligatoria cuando cambian encabezados, opcodes o la versión del bytecode.

Para diagnóstico:

```bash
./luna.py build -v
./luna.py info
```

## 4. Comprobar la instalación

```bash
./build/luna --version
./build/luna -c 'print(1 + 2)'
./build/luna tests/test.luna
```

## 5. Verificar la compatibilidad de bytecode

El bytecode `.luac` usa la magia `LUNA` y la versión `0.1`. Si la versión del archivo no coincide con la del intérprete, recompila los programas desde su fuente `.luna`.

## 6. Actualizar la documentación

Tras un cambio de sintaxis o API, actualiza:

- `docs/MD/` — referencia del lenguaje.
- `docs/index.html` — resumen publicado; recursos en `docs/web/`.
- `docs/Ayuda/actualizaciones/CHANGELOG.md` — registro de cambios.
- `luna.ebnf` — gramática, si cambia la sintaxis.

## 7. Cambios en la ABI

Si cambia `LUNA_ABI_VERSION`, las extensiones nativas deben recompilarse. Revisa la sección de versiones de la [API C](../../MD/10-api-c.md).
