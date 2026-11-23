#!/usr/bin/env bash
set -euo pipefail

REPO_URL="https://github.com/ZCodeGX/Luna.git"
PREFIX="${LUNA_PREFIX:-$HOME/.local}"
SOURCE_DIR="${LUNA_SOURCE_DIR:-${XDG_DATA_HOME:-$HOME/.local/share}/luna}"
BIN_DIR="$PREFIX/bin"
INCLUDE_DIR="$PREFIX/include/luna"

ASSUME_YES=0
VERBOSE=0
COLOR=1
ANIMATE=0
SKIP_PATH=0
SHOW_INFO=0
TMP_BIN=""
SOURCE=""
BUILT=0

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
Uso: install.sh [opciones]

Instala Luna desde este repositorio local o desde GitHub en Linux/macOS.

Opciones:
  --prefix RUTA       Prefijo de instalación (por defecto ~/.local)
  --source-dir RUTA   Directorio del código fuente
  -y, --yes           No pedir confirmación
  --no-path           No modificar archivos de inicio del shell
  --info              Mostrar información de la instalación y salir
  --verbose           Mostrar salida completa de cada paso
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
    -y|--yes) ASSUME_YES=1; shift ;;
    --no-path) SKIP_PATH=1; shift ;;
    --info) SHOW_INFO=1; shift ;;
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
      _     ___  _   _   ____  _   _ _____
     | |   / _ \| \ | | / ___|| \ | | ____|
     | |  | | | |  \| | \___ \|  \| |  _|
     | |__| |_| | |\  |  ___) | |\  | |___
     |_____\___/|_| \_| |____/|_| \_|_____|
EOF
  printf '  %s\n' "$(paint '2;36' '  instalador lunar · Linux/macOS · shell')"
}

have() { command -v "$1" >/dev/null 2>&1; }

as_root() {
  if [[ $EUID -eq 0 ]]; then
    "$@"
  elif have sudo; then
    sudo "$@"
  else
    die "Se necesitan permisos de root para: $*"
  fi
}

detect_local_source() {
  local script="${BASH_SOURCE[0]:-}"
  [[ -n "$script" && -f "$script" ]] || return 0
  local dir
  dir="$(cd -- "$(dirname -- "$script")" 2>/dev/null && pwd)" || return 0
  if [[ -f "$dir/luna.py" && -d "$dir/src" && -f "$dir/cli/main.c" ]]; then
    printf '%s' "$dir"
  fi
}

installed_version() {
  if [[ -x "$BIN_DIR/luna" ]]; then
    "$BIN_DIR/luna" --version 2>/dev/null | head -n 1 || true
  fi
}

source_version() {
  local src="$1"
  [[ -f "$src/include/luna/luna.h" ]] || return 0
  sed -n 's/^#define LUNA_VERSION_STRING "\(.*\)"/\1/p' "$src/include/luna/luna.h" | head -n 1
}

show_plan() {
  local current target mode
  current="$(installed_version)"
  target="$(source_version "$SOURCE")"
  if [[ -z "$current" ]]; then mode="Instalación nueva"; else mode="Actualización"; fi

  section "Plan de instalación"
  kv "Sistema" "$(uname -s) $(uname -m)"
  kv "Modo" "$mode"
  kv "Versión instalada" "${current:-ninguna}"
  kv "Versión destino" "${target:-desconocida}"
  kv "Origen" "$SOURCE"
  kv "Binario" "$BIN_DIR/luna"
  kv "Cabeceras" "$INCLUDE_DIR"

  if [[ -n "$current" && -n "$target" && "$current" == *"$target"* ]]; then
    warn "La versión destino coincide con la instalada; se reinstalará de todos modos."
  elif [[ -n "$current" && -n "$target" ]]; then
    ok "Se actualizará de $current a $target"
  fi
}

ensure_deps() {
  local need_python=0 need_git=0 need_cc=0
  have python3 || need_python=1
  if (( NEED_GIT )) && ! have git; then need_git=1; fi
  have cc || have gcc || have clang || need_cc=1

  if (( need_python == 0 && need_git == 0 && need_cc == 0 )); then
    ok "Dependencias ya disponibles"
    return 0
  fi

  local pm=""
  local manager
  for manager in apt-get dnf yum pacman apk brew; do
    if have "$manager"; then pm="$manager"; break; fi
  done
  [[ -n "$pm" ]] || die "Faltan dependencias y no se encontró un gestor de paquetes."

  local pkgs=()
  case "$pm" in
    apt-get|dnf|yum|apk)
      if (( need_python )); then pkgs+=(python3); fi
      if (( need_git )); then pkgs+=(git); fi
      if (( need_cc )); then pkgs+=(gcc); fi
      ;;
    pacman)
      if (( need_python )); then pkgs+=(python); fi
      if (( need_git )); then pkgs+=(git); fi
      if (( need_cc )); then pkgs+=(gcc); fi
      ;;
    brew)
      if (( need_python )); then pkgs+=(python); fi
      if (( need_git )); then pkgs+=(git); fi
      ;;
  esac

  (( ${#pkgs[@]} > 0 )) || die "No se pudieron resolver las dependencias para $pm."
  run_step "Instalando dependencias: ${pkgs[*]}" install_packages "$pm" "${pkgs[@]}"

  if ! have cc && ! have gcc && ! have clang; then
    die "No se pudo instalar el compilador C. Instálalo manualmente."
  fi
}

install_packages() {
  local pm="$1"
  shift
  case "$pm" in
    apt-get)
      as_root apt-get update -qq
      as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y -qq "$@"
      ;;
    dnf|yum) as_root "$pm" install -y "$@" ;;
    pacman) as_root pacman -Syu --noconfirm "$@" ;;
    apk) as_root apk add --no-cache "$@" ;;
    brew) brew install "$@" ;;
  esac
}

prepare_source() {
  if [[ -n "$LOCAL_SOURCE" ]]; then
    SOURCE="$LOCAL_SOURCE"
    ok "Usando el repositorio local"
    return 0
  fi

  mkdir -p -- "$(dirname -- "$SOURCE_DIR")"
  if [[ -d "$SOURCE_DIR/.git" ]]; then
    if [[ -n "$(git -C "$SOURCE_DIR" status --porcelain 2>/dev/null)" ]]; then
      warn "Hay cambios locales en $SOURCE_DIR; no se actualizará."
    elif git -C "$SOURCE_DIR" pull --ff-only --quiet; then
      ok "Repositorio actualizado"
    else
      warn "No se pudo actualizar; se compilará la copia existente."
    fi
  elif [[ -e "$SOURCE_DIR" ]]; then
    die "$SOURCE_DIR existe y no es un repositorio git. Usa --source-dir para otra ruta."
  else
    run_step "Descargando Luna" git clone --depth 1 --quiet "$REPO_URL" "$SOURCE_DIR"
  fi
  SOURCE="$SOURCE_DIR"
}

build_luna() {
  if ! have cc; then
    TMP_BIN="$(mktemp -d)"
    local compiler=""
    if have gcc; then compiler="$(command -v gcc)"; elif have clang; then compiler="$(command -v clang)"; fi
    [[ -n "$compiler" ]] || die "No hay un compilador C disponible."
    ln -sf -- "$compiler" "$TMP_BIN/cc"
    export PATH="$TMP_BIN:$PATH"
  fi

  BUILT=1
  run_step "Compilando Luna" build_in_source
  [[ -x "$SOURCE/build/luna" ]] || die "La compilación no produjo build/luna."
}

build_in_source() {
  cd -- "$SOURCE"
  python3 ./luna.py clean >/dev/null
  python3 ./luna.py build
}

install_luna() {
  mkdir -p -- "$BIN_DIR" "$PREFIX/include"
  install -m 0755 "$SOURCE/build/luna" "$BIN_DIR/luna"
  if [[ -d "$SOURCE/include/luna" ]]; then
    mkdir -p -- "$INCLUDE_DIR"
    install -m 0644 "$SOURCE/include/luna/"* "$INCLUDE_DIR/"
  fi
  ok "Binario instalado en $BIN_DIR/luna"
}

setup_path() {
  if (( SKIP_PATH )); then
    warn "PATH no modificado (--no-path)"
    return 0
  fi

  local line="export PATH=\"$BIN_DIR:\$PATH\""
  local rc
  for rc in "$HOME/.profile" "$HOME/.bashrc" "$HOME/.zshrc"; do
    if [[ "$rc" == "$HOME/.profile" || -f "$rc" ]]; then
      if ! touch "$rc" 2>/dev/null; then
        warn "No se pudo escribir en $rc"
        continue
      fi
      grep -Fqx -- "$line" "$rc" || printf '%s\n' "$line" >> "$rc"
    fi
  done
  export PATH="$BIN_DIR:$PATH"
  ok "PATH configurado para futuras terminales"
}

verify() {
  local version result
  version="$("$BIN_DIR/luna" --version)"
  result="$("$BIN_DIR/luna" -c 'print(1 + 2)')"
  [[ "$result" == "3" ]] || die "Prueba rápida falló: esperaba 3, se obtuvo $result"
  ok "$version — instalación verificada"
}

cleanup() {
  [[ -n "$TMP_BIN" ]] && rm -rf -- "$TMP_BIN"
  if (( BUILT )) && [[ -n "$SOURCE" && -d "$SOURCE/build" ]]; then
    rm -rf -- "$SOURCE/build"
  fi
}
trap cleanup EXIT

banner

if (( SHOW_INFO )); then
  LOCAL_SOURCE="$(detect_local_source || true)"
  if [[ -n "$LOCAL_SOURCE" ]]; then SOURCE="$LOCAL_SOURCE"; fi
  section "Información"
  kv "Sistema" "$(uname -s) $(uname -m)"
  kv "Versión instalada" "$(installed_version | sed 's/^Luna //')"
  kv "Prefijo" "$PREFIX"
  kv "Binario" "$BIN_DIR/luna"
  kv "Fuentes" "${SOURCE:-$SOURCE_DIR}"
  printf '\n'
  exit 0
fi

section "Comprobando entorno"
LOCAL_SOURCE="$(detect_local_source || true)"
NEED_GIT=0
[[ -n "$LOCAL_SOURCE" ]] || NEED_GIT=1
ensure_deps

section "Preparando fuentes"
prepare_source
show_plan

if ! confirm "¿Continuar con la instalación?"; then
  warn "Instalación cancelada"
  exit 0
fi

section "Compilando"
build_luna

section "Instalando"
install_luna
setup_path
verify

section "Listo"
printf '\n'
ok "Luna instalado correctamente"
printf '    %s\n' "$(paint '2;36' 'Abre una terminal nueva o ejecuta: source ~/.bashrc')"
printf '    luna --version\n    luna programa.luna\n    luna -c '\''print(1 + 2)'\''\n'
printf '    %s\n' "$(paint '2;36' 'Para desinstalar: ./uninstall.sh')"
