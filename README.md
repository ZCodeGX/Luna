<p align="center">
  <img src="assets/luna-logo.svg" alt="Luna" width="360">
</p>

**Luna** es un lenguaje de programación pequeño, embebible y con CLI estilo Python.

**Versión actual: `0.3.0`** (`MAJOR.MINOR.PATCH`: cambios grandes, mejoras y parches).

## Instalación

### Linux y macOS

```bash
curl -fsSL https://raw.githubusercontent.com/ZCodeGX/Luna/main/install.sh | bash
```

Si ya clonaste el repositorio:

```bash
./install.sh
```

El script instala Python 3, git y un compilador C11 si faltan, compila Luna y deja `luna` en `~/.local/bin`.

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
> Los comandos remotos requieren que `install.sh` e `install.ps1` estén publicados en la rama `main`.

## Inicio rápido

```bash
luna -c 'let x = 1'
luna programa.luna
```

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
