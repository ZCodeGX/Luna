$ErrorActionPreference = 'Stop'

$RepoUrl = 'https://github.com/ZCodeGX/Luna.git'
$InstallRoot = if ($env:LUNA_PREFIX) { $env:LUNA_PREFIX } else { Join-Path $env:LOCALAPPDATA 'Luna' }
$BinDir = Join-Path $InstallRoot 'bin'
$IncludeDir = Join-Path $InstallRoot 'include\luna'
$SourceDir = if ($env:LUNA_SOURCE_DIR) { $env:LUNA_SOURCE_DIR } else { Join-Path $InstallRoot 'source' }
$ShimDir = Join-Path $env:TEMP ("luna-cc-{0}" -f $PID)

function Say {
  param([string]$Message)
  Write-Host '==> ' -ForegroundColor DarkCyan -NoNewline
  Write-Host $Message
}

function Good {
  param([string]$Message)
  Write-Host ' OK  ' -ForegroundColor DarkGreen -NoNewline
  Write-Host $Message
}

function Refresh-Path {
  $machine = [Environment]::GetEnvironmentVariable('Path', 'Machine')
  $user = [Environment]::GetEnvironmentVariable('Path', 'User')
  if (-not $machine) { $machine = '' }
  if (-not $user) { $user = '' }
  $env:Path = "$machine;$user"
}

function Install-Winget {
  param([string]$Id, [string]$Name)
  if (-not (Get-Command winget -ErrorAction SilentlyContinue)) { return $false }
  Say "Instalando $Name con winget..."
  winget install --exact --id $Id --accept-source-agreements --accept-package-agreements --silent --disable-interactivity
  if ($LASTEXITCODE -ne 0) { throw "No se pudo instalar $Name con winget." }
  Refresh-Path
  return $true
}

function Resolve-Python {
  $candidates = @(
    [pscustomobject]@{ Cmd = 'python'; Prefix = @() },
    [pscustomobject]@{ Cmd = 'py'; Prefix = @('-3') }
  )
  foreach ($candidate in $candidates) {
    if (-not (Get-Command $candidate.Cmd -ErrorAction SilentlyContinue)) { continue }
    $prefix = $candidate.Prefix
    try {
      $output = & $candidate.Cmd @prefix --version 2>&1 | Out-String
      if ($LASTEXITCODE -eq 0 -and $output -match 'Python\s+3') {
        $script:Python = $candidate
        return
      }
    } catch {
      continue
    }
  }
}

function Invoke-Python {
  param([string[]]$Arguments)
  $prefix = $script:Python.Prefix
  & $script:Python.Cmd @prefix @Arguments
  if ($LASTEXITCODE -ne 0) { throw "Python falló al ejecutar: $($Arguments -join ' ')" }
}

function Find-Gcc {
  $inPath = Get-Command gcc.exe -ErrorAction SilentlyContinue
  if ($inPath) { return $inPath.Source }

  $roots = @(
    'C:\msys64',
    (Join-Path $env:LOCALAPPDATA 'MSYS2'),
    (Join-Path $env:ProgramFiles 'MSYS2'),
    'C:\Program Files (x86)\MSYS2',
    'C:\tools\mingw64'
  )
  foreach ($root in $roots) {
    if (-not $root) { continue }
    foreach ($relative in @('mingw64\bin\gcc.exe', 'bin\gcc.exe', 'mingw32\bin\gcc.exe')) {
      $candidate = Join-Path $root $relative
      if (Test-Path $candidate) { return $candidate }
    }
  }
  return $null
}

function Find-MsysBash {
  $roots = @(
    'C:\msys64',
    (Join-Path $env:LOCALAPPDATA 'MSYS2'),
    (Join-Path $env:ProgramFiles 'MSYS2'),
    'C:\Program Files (x86)\MSYS2'
  )
  foreach ($root in $roots) {
    if (-not $root) { continue }
    $candidate = Join-Path $root 'usr\bin\bash.exe'
    if (Test-Path $candidate) { return $candidate }
  }
  return $null
}

function Install-Compiler {
  $gcc = Find-Gcc
  if ($gcc) { return $gcc }

  Say 'Instalando un compilador C11 (MinGW-w64)...'
  if (Get-Command winget -ErrorAction SilentlyContinue) {
    Install-Winget 'MSYS2.MSYS2' 'MSYS2'
  } elseif (Get-Command choco -ErrorAction SilentlyContinue) {
    Say 'Instalando MinGW con Chocolatey...'
    choco install mingw -y --no-progress
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo instalar MinGW con Chocolatey.' }
    Refresh-Path
  } else {
    throw 'Se necesita winget o Chocolatey para instalar un compilador C11. Instala MSYS2/MinGW-w64 manualmente y vuelve a ejecutar el script.'
  }

  $bash = Find-MsysBash
  if ($bash) {
    Say 'Instalando gcc dentro de MSYS2...'
    & $bash -lc 'pacman -Sy --noconfirm && pacman -S --noconfirm mingw-w64-x86_64-gcc'
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo instalar mingw-w64-x86_64-gcc con pacman.' }
  }

  $gcc = Find-Gcc
  if (-not $gcc) { throw 'No se encontró gcc tras la instalación. Instala MinGW-w64 manualmente y vuelve a ejecutar el script.' }
  return $gcc
}

$LocalSource = ''
if ($PSScriptRoot -and (Test-Path (Join-Path $PSScriptRoot 'luna.py')) -and (Test-Path (Join-Path $PSScriptRoot 'cli/main.c'))) {
  $LocalSource = $PSScriptRoot
}

$script:Python = $null
Resolve-Python
if (-not $script:Python) {
  if (-not (Install-Winget 'Python.Python.3.14' 'Python 3.14')) {
    throw 'Falta Python 3 y no hay winget disponible. Instala Python 3 manualmente y vuelve a ejecutar el script.'
  }
  Resolve-Python
}
if (-not $script:Python) { throw 'No se encontró Python 3 después de la instalación.' }
Good "Python detectado: $($script:Python.Cmd) $($script:Python.Prefix -join ' ')"

if (-not $LocalSource -and -not (Get-Command git.exe -ErrorAction SilentlyContinue)) {
  if (-not (Install-Winget 'Git.Git' 'Git')) {
    throw 'Falta git y no hay winget disponible. Instala Git manualmente y vuelve a ejecutar el script.'
  }
}

$Source = $LocalSource
try {
  if ($LocalSource) {
    $Source = $LocalSource
    Say "Usando el repositorio local: $Source"
  } else {
    $Source = $SourceDir
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Source) | Out-Null
    if (Test-Path (Join-Path $Source '.git')) {
      Say "Actualizando el repositorio en $Source"
      $dirty = & git -C $Source status --porcelain
      if ($dirty) {
        Write-Host ' AVISO  Hay cambios locales; se conservarán sin actualizar.' -ForegroundColor DarkYellow
      } else {
        & git -C $Source pull --ff-only
        if ($LASTEXITCODE -eq 0) {
          Good 'Repositorio actualizado'
        } else {
          Write-Host ' AVISO  No se pudo actualizar; se compilará la copia existente.' -ForegroundColor DarkYellow
        }
      }
    } elseif (Test-Path $Source) {
      throw "$Source ya existe y no es un repositorio git. Usa LUNA_SOURCE_DIR para otra ruta."
    } else {
      Say "Descargando Luna en $Source"
      & git clone --depth 1 $RepoUrl $Source
      if ($LASTEXITCODE -ne 0) { throw 'No se pudo clonar el repositorio.' }
    }
  }

  if (-not (Test-Path (Join-Path $Source 'luna.py')) -or -not (Test-Path (Join-Path $Source 'cli/main.c'))) {
    throw "$Source no parece el repositorio de Luna."
  }

  $gcc = Install-Compiler
  $gccDir = Split-Path -Parent $gcc
  New-Item -ItemType Directory -Force -Path $ShimDir | Out-Null
  $gccLiteral = $gcc.Replace('\', '\\')
  $wrapper = @"
#include <process.h>
#include <stdlib.h>
int main(int argc, char **argv) {
  char **args = (char **)malloc((size_t)(argc + 1) * sizeof(char *));
  if (!args) return 1;
  args[0] = "$gccLiteral";
  for (int i = 1; i < argc; i++) args[i] = argv[i];
  args[argc] = NULL;
  int code = _spawnv(_P_WAIT, args[0], args);
  free(args);
  return code;
}
"@
  Set-Content -Path (Join-Path $ShimDir 'cc.c') -Value $wrapper -Encoding ascii
  & $gcc (Join-Path $ShimDir 'cc.c') -o (Join-Path $ShimDir 'cc.exe')
  if ($LASTEXITCODE -ne 0 -or -not (Test-Path (Join-Path $ShimDir 'cc.exe'))) {
    throw 'No se pudo crear el puente cc.exe para el compilador.'
  }
  $env:Path = "$ShimDir;$gccDir;$env:Path"

  Say "Compilando Luna en $Source"
  Push-Location $Source
  try {
    Invoke-Python @('./luna.py', 'clean')
    Invoke-Python @('./luna.py', 'build')
  } finally {
    Pop-Location
  }

  $built = Join-Path $Source 'build\luna.exe'
  if (-not (Test-Path $built)) { $built = Join-Path $Source 'build\luna' }
  if (-not (Test-Path $built)) { throw 'La compilación no produjo build\luna.exe.' }

  New-Item -ItemType Directory -Force -Path $BinDir | Out-Null
  Copy-Item -Path $built -Destination (Join-Path $BinDir 'luna.exe') -Force
  if (Test-Path (Join-Path $Source 'include\luna')) {
    New-Item -ItemType Directory -Force -Path $IncludeDir | Out-Null
    Copy-Item -Path (Join-Path $Source 'include\luna\*') -Destination $IncludeDir -Force
  }
  Good "Binario instalado en $(Join-Path $BinDir 'luna.exe')"

  $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
  if (-not $userPath) { $userPath = '' }
  $parts = $userPath -split ';' | Where-Object { $_ }
  if ($parts -notcontains $BinDir) {
    $newPath = (@($BinDir) + @($parts)) -join ';'
    [Environment]::SetEnvironmentVariable('Path', $newPath, 'User')
  }
  $env:Path = "$BinDir;$env:Path"
  Good 'PATH de usuario configurado'

  $version = & (Join-Path $BinDir 'luna.exe') --version
  $result = (& (Join-Path $BinDir 'luna.exe') -c 'print(1 + 2)' | Out-String).Trim()
  if ($result -ne '3') { throw "La prueba rápida falló: se esperaba 3, se obtuvo $result" }
  Good "$version — instalación verificada"

  Write-Host ''
  Good 'Luna listo. Abre una terminal nueva o reinicia PowerShell.'
  Write-Host '  luna --version'
  Write-Host '  luna programa.luna'
  Write-Host "  luna -c 'print(1 + 2)'"
} finally {
  if ($Source -and (Test-Path (Join-Path $Source 'build'))) {
    Remove-Item -Path (Join-Path $Source 'build') -Recurse -Force -ErrorAction SilentlyContinue
  }
  if (Test-Path $ShimDir) {
    Remove-Item -Path $ShimDir -Recurse -Force -ErrorAction SilentlyContinue
  }
}
