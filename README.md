<p align="center">
  <img src="assets/luna-logo.svg" alt="Luna" width="360">
</p>

**Luna** es un lenguaje de programación pequeño, embebible y con CLI estilo Python.

**Versión actual: `0.7.6`** (`MAJOR.MINOR.PATCH`: cambios grandes, mejoras y parches).

Documentación: [guía web](docs/Web/index.html) · [inicio rápido en Markdown](docs/MD/01-inicio-rapido.md) · [índice completo](docs/MD/README.md).

## Instalación

### Linux y macOS

```bash
curl -fsSL https://raw.githubusercontent.com/ZCodeGX/Luna/main/install.sh | bash
```

Si ya clonaste el repositorio:

```bash
./install.sh
```

El instalador muestra versiones, rutas y progreso animado; detecta instalaciones previas para actualizar; instala Python 3, git y un compilador C11 si faltan; compila Luna y deja `luna` en `~/.local/bin`.

Opciones útiles:

```bash
./install.sh --info
./install.sh --prefix ~/.local --source-dir ./luna-src
./install.sh --no-path
./install.sh --help
```

Para desinstalar completamente Luna en Linux/macOS:

```bash
curl -fsSL https://raw.githubusercontent.com/ZCodeGX/Luna/main/uninstall.sh | bash
```

O, desde el repositorio:

```bash
./uninstall.sh --dry-run
./uninstall.sh --keep-source
./uninstall.sh --help
```

El desinstalador elimina el binario, las cabeceras, las entradas del `PATH` y el repositorio de fuentes usado por el instalador.

### Windows (PowerShell)

```powershell
irm https://raw.githubusercontent.com/ZCodeGX/Luna/main/install.ps1 | iex
```

Si ya clonaste el repositorio:

```powershell
.\install.ps1
```

El script instala Python 3, Git y MinGW-w64 con winget o Chocolatey, y deja `luna.exe` en `%LOCALAPPDATA%\Luna\bin`.

> [!NOTE]
> Los comandos remotos requieren que `install.sh`, `uninstall.sh` e `install.ps1` estén publicados en la rama `main`. `install.sh` y `uninstall.sh` son solo para Linux/macOS; Windows usa `install.ps1`.

## Inicio rápido

```bash
./luna.py build
./build/luna -c 'print("Hola, Luna")'
```

Salida:

```text
Hola, Luna
```

Crea `hola.luna`:

```luna
let nombre = "Luna"
print("Hola,", nombre)
```

```bash
./build/luna hola.luna
```

Salida:

```text
Hola, Luna
```

Si instalaste el binario, sustituye `./build/luna` por `luna`. Recorre la [ruta guiada](docs/MD/README.md) para seguir aprendiendo.

> [!TIP]
> `luna` sin argumentos abre el REPL y `-` lee el programa desde stdin. Para compilar sin instalar: `./luna.py build` y `./build/luna`.

> [!IMPORTANT]
> Luna usa GPL-3.0: cualquier modificación o distribución debe mantener el código abierto y compartirse bajo la misma licencia.

> [!WARNING]
> La sintaxis y la API C aún pueden cambiar entre versiones.

> [!CAUTION]
> No lo uses en producción: el runtime y la biblioteca estándar siguen en desarrollo.

## Licencia

[GPL-3.0](LICENSE). Las contribuciones se aceptan bajo los mismos términos.
