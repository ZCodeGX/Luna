#!/usr/bin/env bash
set -euo pipefail

REPO_URL="https://github.com/ZCodeGX/Luna.git"
PREFIX="${LUNA_PREFIX:-$HOME/.local}"
BIN_DIR="$PREFIX/bin"
SOURCE_DIR="${LUNA_SOURCE_DIR:-${XDG_DATA_HOME:-$HOME/.local/share}/luna}"

say() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
ok() { printf '\033[1;32m OK \033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m AVISO \033[0m %s\n' "$*" >&2; }
die() { printf '\033[1;31m ERROR \033[0m %s\n' "$*" >&2; exit 1; }

have() { command -v "$1" >/dev/null 2>&1; }

as_root() {
  if [[ $EUID -eq 0 ]]; then
    "$@"
  elif have sudo; then
    sudo "$@"
  else
    die "Se necesitan permisos de root para instalar: $*"
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

ensure_deps() {
  local need_python=0 need_git=0 need_cc=0
  have python3 || need_python=1
  if (( NEED_GIT )) && ! have git; then
    need_git=1
  fi
  have cc || have gcc || have clang || need_cc=1

  if (( need_python == 0 && need_git == 0 && need_cc == 0 )); then
    return 0
  fi

  local pm=""
  local manager
  for manager in apt-get dnf yum pacman apk brew; do
    if have "$manager"; then
      pm="$manager"
      break
    fi
  done
  [[ -n "$pm" ]] || die "Faltan dependencias y no se encontró un gestor de paquetes. Instala python3, git y un compilador C11."

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

  if (( ${#pkgs[@]} == 0 )); then
    die "No se pudieron resolver las dependencias para $pm. Instálalas manualmente."
  fi

  say "Instalando dependencias: ${pkgs[*]}"
  case "$pm" in
    apt-get)
      as_root apt-get update -qq
      as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y -qq "${pkgs[@]}"
      ;;
    dnf|yum) as_root "$pm" install -y "${pkgs[@]}" ;;
    pacman) as_root pacman -Syu --noconfirm "${pkgs[@]}" ;;
    apk) as_root apk add --no-cache "${pkgs[@]}" ;;
    brew) brew install "${pkgs[@]}" ;;
  esac

  if ! have cc && ! have gcc && ! have clang; then
    die "No se pudo instalar el compilador C. Instálalo manualmente y vuelve a ejecutar el script."
  fi
}

prepare_source() {
  if [[ -n "$LOCAL_SOURCE" ]]; then
    SOURCE="$LOCAL_SOURCE"
    say "Usando el repositorio local: $SOURCE"
    return 0
  fi

  mkdir -p -- "$(dirname -- "$SOURCE_DIR")"
  if [[ -d "$SOURCE_DIR/.git" ]]; then
    say "Actualizando el repositorio en $SOURCE_DIR"
    if [[ -n "$(git -C "$SOURCE_DIR" status --porcelain 2>/dev/null)" ]]; then
      warn "Hay cambios locales en $SOURCE_DIR; se conservarán sin actualizar."
    elif ! git -C "$SOURCE_DIR" pull --ff-only --quiet; then
      warn "No se pudo actualizar; se compilará la copia existente."
    fi
  elif [[ -e "$SOURCE_DIR" ]]; then
    die "$SOURCE_DIR ya existe y no es un repositorio git. Usa LUNA_SOURCE_DIR para otra ruta."
  else
    say "Descargando Luna en $SOURCE_DIR"
    git clone --depth 1 --quiet "$REPO_URL" "$SOURCE_DIR"
  fi
  SOURCE="$SOURCE_DIR"
}

build_luna() {
  if ! have cc; then
    TMP_BIN="$(mktemp -d)"
    local compiler=""
    if have gcc; then
      compiler="$(command -v gcc)"
    elif have clang; then
      compiler="$(command -v clang)"
    fi
    [[ -n "$compiler" ]] || die "No hay un compilador C disponible."
    ln -sf -- "$compiler" "$TMP_BIN/cc"
    export PATH="$TMP_BIN:$PATH"
  fi

  say "Compilando Luna en $SOURCE"
  (
    cd -- "$SOURCE"
    python3 ./luna.py clean >/dev/null
    python3 ./luna.py build
  )
  [[ -x "$SOURCE/build/luna" ]] || die "La compilación no produjo build/luna."
}

install_luna() {
  mkdir -p -- "$BIN_DIR"
  install -m 0755 "$SOURCE/build/luna" "$BIN_DIR/luna"
  if [[ -d "$SOURCE/include/luna" ]]; then
    mkdir -p -- "$PREFIX/include/luna"
    install -m 0644 "$SOURCE/include/luna/"* "$PREFIX/include/luna/"
  fi
  ok "Binario instalado en $BIN_DIR/luna"
}

setup_path() {
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

cleanup() {
  rm -rf -- "$SOURCE/build"
  if [[ -n "${TMP_BIN:-}" ]]; then
    rm -rf -- "$TMP_BIN"
  fi
}

verify() {
  local version
  version="$("$BIN_DIR/luna" --version)"
  local result
  result="$("$BIN_DIR/luna" -c 'print(1 + 2)')"
  [[ "$result" == "3" ]] || die "La prueba rápida falló: se esperaba 3, se obtuvo $result"
  ok "$version — instalación verificada"
}

LOCAL_SOURCE="$(detect_local_source || true)"
SOURCE=""
TMP_BIN=""
NEED_GIT=0
if [[ -z "$LOCAL_SOURCE" ]]; then
  NEED_GIT=1
fi

if ! have python3; then
  say "Se necesita Python 3 para el sistema de build"
fi
if (( NEED_GIT )) && ! have git; then
  say "Se necesita git para descargar el repositorio"
fi

ensure_deps
prepare_source
[[ -f "$SOURCE/luna.py" && -f "$SOURCE/cli/main.c" ]] || die "$SOURCE no parece el repositorio de Luna."
build_luna
install_luna
setup_path
verify
cleanup

printf '\n'
ok "Luna listo. Abre una terminal nueva o ejecuta: source ~/.bashrc"
printf '  luna --version\n  luna programa.luna\n  luna -c '\''print(1 + 2)'\''\n'
