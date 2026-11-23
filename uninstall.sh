#!/usr/bin/env bash
set -euo pipefail

PREFIX="${LUNA_PREFIX:-$HOME/.local}"
SOURCE_DIR="${LUNA_SOURCE_DIR:-${XDG_DATA_HOME:-$HOME/.local/share}/luna}"
BIN_DIR="$PREFIX/bin"
INCLUDE_DIR="$PREFIX/include/luna"

ASSUME_YES=0
VERBOSE=0
COLOR=1
ANIMATE=0
DRY_RUN=0
KEEP_SOURCE=0

if [[ -t 1 && -z "${NO_COLOR:-}" ]]; then
  COLOR=1
  ANIMATE=1
else
  COLOR=0
  ANIMATE=0
fi

paint() {
  local code="$1" text="$2"
  if (( COLOR )); then
    printf '\033[%sm%s\033[0m' "$code" "$text"
  else
    printf '%s' "$text"
  fi
}

say() { printf '  %s %s\n' "$(paint '1;35' '◆')" "$*"; }
ok() { printf '  %s %s\n' "$(paint '1;32' 'OK')" "$*"; }
warn() { printf '  %s %s\n' "$(paint '1;33' 'AVISO')" "$*" >&2; }
die() { printf '  %s %s\n' "$(paint '1;31' 'ERROR')" "$*" >&2; exit 1; }
section() { printf '\n  %s\n' "$(paint '1;36' "── $1 ──")"; }
kv() { printf '    %-20s %s\n' "$1" "$2"; }

usage() {
  cat <<'EOF'
Uso: uninstall.sh [opciones]

Desinstala Luna instalado con install.sh en Linux/macOS.

Opciones:
  --prefix RUTA       Prefijo de instalación (por defecto ~/.local)
  --source-dir RUTA   Directorio del código fuente
  --keep-source       Conservar el repositorio de fuentes
  --dry-run           Mostrar qué se eliminaría sin borrar nada
  -y, --yes           No pedir confirmación
  --verbose           Mostrar detalles de cada paso
  --no-color          Desactivar colores y animaciones
  --color             Forzar colores
  -h, --help          Mostrar esta ayuda

Variables de entorno admitidas:
  LUNA_PREFIX, LUNA_SOURCE_DIR, XDG_DATA_HOME, NO_COLOR
EOF
}

while (($#)); do
  case "$1" in
    --prefix) [[ $# -ge 2 ]] || die "--prefix necesita una ruta"; PREFIX="$2"; shift 2 ;;
    --prefix=*) PREFIX="${1#*=}"; shift ;;
    --source-dir) [[ $# -ge 2 ]] || die "--source-dir necesita una ruta"; SOURCE_DIR="$2"; shift 2 ;;
    --source-dir=*) SOURCE_DIR="${1#*=}"; shift ;;
    --keep-source) KEEP_SOURCE=1; shift ;;
    --dry-run) DRY_RUN=1; shift ;;
    -y|--yes) ASSUME_YES=1; shift ;;
    --verbose) VERBOSE=1; shift ;;
    --no-color) COLOR=0; ANIMATE=0; shift ;;
    --color) COLOR=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) die "Opción desconocida: $1 (usa --help)" ;;
  esac
done

BIN_DIR="$PREFIX/bin"
INCLUDE_DIR="$PREFIX/include/luna"

SPIN=( '|' '/' '-' '\' )

run_step() {
  local label="$1"
  shift
  local log
  log="$(mktemp)"
  "$@" >"$log" 2>&1 &
  local pid=$!
  local idx=0

  if (( ANIMATE )); then
    while kill -0 "$pid" 2>/dev/null; do
      printf '\r    %s %s' "$(paint '1;34' "$label")" "${SPIN[$idx]}"
      idx=$(( (idx + 1) % 4 ))
      sleep 0.1
    done
    printf '\r\033[2K'
  fi

  local status=0
  wait "$pid" || status=$?
  if (( status != 0 )); then
    if [[ -s "$log" ]]; then
      printf '%s\n' "$(paint '1;31' '  Salida del paso fallido:')"
      cat "$log"
    fi
    rm -f "$log"
    die "$label falló (exit $status)"
  fi

  if (( VERBOSE )) && [[ -s "$log" ]]; then cat "$log"; fi
  rm -f "$log"
  ok "$label"
}

confirm() {
  local question="$1"
  if (( ASSUME_YES )) || [[ ! -t 0 ]]; then return 0; fi
  local answer=""
  printf '    %s [s/N] ' "$question"
  read -r answer || true
  [[ "$answer" == [sS] || "$answer" == [sS][iI] ]]
}

banner() {
  printf '\n'
  cat <<'EOF'
      _     ___  _   _   _   _ _____ ___ ___  _   _ _____
     | |   / _ \| \ | | | \ | |_   _|_ _/ _ \| \ | | ____|
     | |  | | | |  \| | |  \| | | |  | | | | |  \| |  _|
     | |__| |_| | |\  | | |\  | | |  | | |_| | |\  | |___
     |_____\___/|_| \_| |_| \_| |_| |___\___/|_| \_|_____|
EOF
  printf '  %s\n' "$(paint '2;36' '  desinstalador lunar · Linux/macOS · shell')"
}

looks_like_luna_source() {
  [[ -d "$SOURCE_DIR" && -f "$SOURCE_DIR/luna.py" && -d "$SOURCE_DIR/src" && -f "$SOURCE_DIR/cli/main.c" ]]
}

path_line() {
  printf 'export PATH="%s:$PATH"' "$BIN_DIR"
}

path_files() {
  local line rc
  line="$(path_line)"
  for rc in "$HOME/.profile" "$HOME/.bashrc" "$HOME/.zshrc"; do
    if [[ -f "$rc" ]] && grep -Fqx -- "$line" "$rc"; then
      printf '%s\n' "$rc"
    fi
  done
}

show_plan() {
  section "Elementos detectados"
  if [[ -f "$BIN_DIR/luna" ]]; then
    kv "Binario" "$(paint '1;32' 'presente')  $BIN_DIR/luna"
  else
    kv "Binario" "$(paint '2;90' 'ausente')   $BIN_DIR/luna"
  fi

  if [[ -d "$INCLUDE_DIR" ]]; then
    kv "Cabeceras" "$(paint '1;32' 'presente')  $INCLUDE_DIR"
  else
    kv "Cabeceras" "$(paint '2;90' 'ausente')   $INCLUDE_DIR"
  fi

  if looks_like_luna_source; then
    kv "Fuentes" "$(paint '1;32' 'presente')  $SOURCE_DIR"
  else
    kv "Fuentes" "$(paint '2;90' 'ausente')   $SOURCE_DIR"
  fi

  local rc found=0
  while IFS= read -r rc; do
    [[ -n "$rc" ]] || continue
    found=1
    kv "PATH" "$(paint '1;33' 'entrada')   $rc"
  done < <(path_files)
  if (( found == 0 )); then
    kv "PATH" "$(paint '2;90' 'sin entradas')"
  fi
}

remove_binary() {
  [[ -f "$BIN_DIR/luna" ]] || return 0
  rm -f -- "$BIN_DIR/luna"
  rmdir -- "$BIN_DIR" 2>/dev/null || true
}

remove_headers() {
  [[ -d "$INCLUDE_DIR" ]] || return 0
  rm -rf -- "$INCLUDE_DIR"
  rmdir -- "$PREFIX/include" 2>/dev/null || true
}

remove_source() {
  (( KEEP_SOURCE )) && return 0
  looks_like_luna_source || return 0
  rm -rf -- "$SOURCE_DIR"
}

clean_path() {
  local line rc tmp
  line="$(path_line)"
  while IFS= read -r rc; do
    [[ -n "$rc" ]] || continue
    tmp="$(mktemp)"
    grep -Fvx -- "$line" "$rc" >"$tmp" || true
    if cmp -s "$tmp" "$rc"; then
      rm -f "$tmp"
    else
      cat "$tmp" >"$rc"
      rm -f "$tmp"
    fi
  done < <(path_files)
}

verify_removal() {
  if [[ -f "$BIN_DIR/luna" ]]; then die "El binario sigue presente"; fi
  if [[ -d "$INCLUDE_DIR" ]]; then die "Las cabeceras siguen presentes"; fi
  ok "Luna ya no está instalado en $PREFIX"
}

banner

section "Desinstalación de Luna"
kv "Sistema" "$(uname -s) $(uname -m)"
kv "Prefijo" "$PREFIX"
show_plan

if (( DRY_RUN )); then
  warn "Modo simulación: no se eliminará nada"
  exit 0
fi

if ! confirm "¿Eliminar los elementos detectados?"; then
  warn "Desinstalación cancelada"
  exit 0
fi

section "Eliminando"
run_step "Eliminando binario" remove_binary
run_step "Eliminando cabeceras" remove_headers
run_step "Limpiando PATH" clean_path
if (( KEEP_SOURCE )); then
  warn "Fuentes conservadas (--keep-source)"
else
  run_step "Eliminando fuentes" remove_source
fi

section "Verificando"
verify_removal
printf '\n'
ok "Desinstalación completada"
printf '    %s\n' "$(paint '2;36' 'Para volver a instalar: ./install.sh')"
