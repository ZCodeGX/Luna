#!/usr/bin/env python3
"""
Luna Build System
Sistema de build declarativo para el lenguaje Luna.
Similar a moz.build de Mozilla.
"""

import os
import sys
import subprocess
import json
import time
from pathlib import Path
from typing import Dict, List, Set, Optional
from dataclasses import dataclass, field
import argparse
import re

# ---------- Configuración ----------

def read_version() -> str:
    header = Path(__file__).resolve().parent / "include" / "luna" / "luna.h"
    match = re.search(r'#define\s+LUNA_VERSION_STRING\s+"([^"]+)"', header.read_text(encoding="utf-8"))
    return match.group(1) if match else "0.0.0"


VERSION = read_version()
BUILD_DIR = Path("build")
SRC_DIR = Path("src")
INCLUDE_BASE = SRC_DIR

def enable_windows_ansi() -> bool:
    if os.name != "nt":
        return sys.stdout.isatty()
    try:
        import ctypes

        kernel32 = ctypes.windll.kernel32
        enabled = True
        for handle_id in (-11, -12):
            handle = kernel32.GetStdHandle(handle_id)
            if handle in (-1, 0):
                enabled = False
                continue
            mode = ctypes.c_uint32()
            if not kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
                enabled = False
                continue
            flag = 0x0004
            if not (mode.value & flag):
                if not kernel32.SetConsoleMode(handle, mode.value | flag):
                    enabled = False
        return enabled
    except Exception:
        return False


def configure_stdio() -> None:
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            try:
                stream.reconfigure(encoding="utf-8", errors="replace")
            except (ValueError, OSError):
                pass


configure_stdio()
ANSI_AVAILABLE = enable_windows_ansi()
USE_COLOR = (
    os.environ.get("NO_COLOR") is None
    and (ANSI_AVAILABLE or os.environ.get("FORCE_COLOR") is not None)
)

RESET = "\033[0m"
BOLD = "\033[1m"
RED = "\033[31m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
BLUE = "\033[34m"
CYAN = "\033[36m"
GRAY = "\033[90m"


def paint(text: str, *codes: str) -> str:
    if not USE_COLOR or not codes:
        return text
    return "".join(codes) + str(text) + RESET


def ok(message: str) -> None:
    print(paint(message, GREEN, BOLD))


def note(message: str) -> None:
    print(paint(message, CYAN))


def warn(message: str) -> None:
    print(paint(message, YELLOW), file=sys.stderr)


def fail(message: str) -> None:
    print(paint(message, RED, BOLD), file=sys.stderr)


def muted(message: str) -> None:
    print(paint(message, GRAY))


def colored_diagnostics(output: str) -> str:
    lines = []
    for line in output.rstrip().splitlines():
        lowered = line.lower()
        if "error:" in lowered or "fatal error:" in lowered:
            lines.append(paint(line, RED, BOLD))
        elif "warning:" in lowered:
            lines.append(paint(line, YELLOW))
        elif "note:" in lowered:
            lines.append(paint(line, CYAN))
        else:
            lines.append(line)
    return "\n".join(lines)


def print_error_steps(output: str) -> None:
    fail("\n¿Qué hacer ahora?")
    lowered = output.lower()
    steps = []

    location = re.search(r"([^\s:]+):(\d+):\d+:.*(?:error|warning):", output)
    if location:
        steps.append(f"Revisa {location.group(1)} en la línea {location.group(2)}.")
    if "no such file or directory" in lowered or "file not found" in lowered:
        steps.append("Verifica SOURCES/HEADERS en luna.build y las rutas de INCLUDES.")
    if "implicit declaration" in lowered:
        steps.append("Añade el header que declara la función o declara su prototipo.")
    if "redefinition" in lowered or "redeclared" in lowered:
        steps.append("Busca definiciones duplicadas o includes sin guardas de inclusión.")
    if "undefined reference" in lowered or "ld returned" in lowered:
        steps.append("Revisa DEPENDS, SOURCES y las librerías del enlazado.")
    if "permission denied" in lowered:
        steps.append("Comprueba permisos del archivo o usa 'python3 luna.py build'.")

    if not steps:
        steps.append("Corrige el error señalado por el compilador y vuelve a ejecutar el build.")
    steps.append("Si el error persiste, ejecuta 'python3 luna.py build -v' para ver el comando exacto.")

    for index, step in enumerate(steps, 1):
        muted(f"  {index}. {step}")

# ---------- Estructuras de datos ----------

@dataclass
class Module:
    """Representa un módulo del proyecto."""
    name: str
    path: Path
    sources: List[str] = field(default_factory=list)
    headers: List[str] = field(default_factory=list)
    includes: List[str] = field(default_factory=list)
    depends: List[str] = field(default_factory=list)
    cflags: List[str] = field(default_factory=list)
    
    @property
    def full_include_path(self) -> str:
        return str(self.path)

# ---------- Carga de módulos ----------

def load_module(build_file: Path) -> Module:
    """Carga un archivo luna.build y retorna un Module."""
    module_dir = build_file.parent
    module_name = module_dir.name
    
    # Ejecutar el archivo .build como Python
    context = {
        'SOURCES': [],
        'HEADERS': [],
        'INCLUDES': [],
        'DEPENDS': [],
        'CFLAGS': [],
    }
    
    try:
        with open(build_file) as f:
            exec(f.read(), context)
    except Exception as e:
        fail(f"✗ Error cargando {build_file}: {e}")
        print_error_steps(str(e))
        sys.exit(1)
    
    return Module(
        name=module_name,
        path=module_dir,
        sources=context['SOURCES'],
        headers=context['HEADERS'],
        includes=context['INCLUDES'],
        depends=context['DEPENDS'],
        cflags=context['CFLAGS'],
    )

def discover_modules() -> Dict[str, Module]:
    """Descubre todos los luna.build en el proyecto."""
    modules = {}
    
    for build_file in SRC_DIR.rglob("luna.build"):
        module = load_module(build_file)
        modules[module.name] = module
    
    return modules

# ---------- Resolución de dependencias ----------

def resolve_dependencies(modules: Dict[str, Module]) -> List[Module]:
    """Resuelve el orden de compilación usando topological sort."""
    visited = set()
    order = []
    
    def visit(name: str, path: List[str]):
        if name in path:
            fail(f"✗ Dependencia circular detectada: {' -> '.join(path + [name])}")
            sys.exit(1)
        
        if name in visited:
            return
        
        if name not in modules:
            fail(f"✗ Módulo desconocido: {name}")
            warn("  Revisa la clave DEPENDS en los archivos luna.build.")
            sys.exit(1)
        
        module = modules[name]
        for dep in module.depends:
            visit(dep, path + [name])
        
        visited.add(name)
        order.append(module)
    
    for name in modules:
        visit(name, [])
    
    return order

# ---------- Compilación ----------

def get_all_includes(modules: Dict[str, Module], module: Module) -> List[str]:
    """Obtiene todos los includes necesarios para un módulo."""
    includes = set()
    
    # Include del módulo actual
    includes.add(module.full_include_path)
    
    # Includes declarados
    for inc in module.includes:
        includes.add(str(SRC_DIR / inc))
    
    # Includes de dependencias
    for dep_name in module.depends:
        if dep_name in modules:
            includes.add(modules[dep_name].full_include_path)
    
    return sorted(includes)

def compile_module(module: Module, modules: Dict[str, Module], verbose: bool = False) -> bool:
    """Compila un módulo. Retorna True si éxito."""
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    
    # Crear directorio de build para este módulo
    module_build_dir = BUILD_DIR / module.path
    module_build_dir.mkdir(parents=True, exist_ok=True)
    
    includes = get_all_includes(modules, module)
    include_flags = [f"-I{inc}" for inc in includes]
    
    base_cflags = [
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Wno-unused-parameter",
        "-g",
        "-O2",
    ]
    base_cflags += os.environ.get("LUNA_CFLAGS", "").split()
    
    all_cflags = base_cflags + include_flags + module.cflags
    
    success = True
    
    for source in module.sources:
        source_path = module.path / source
        object_path = module_build_dir / (source.replace('.c', '.o'))
        
        # Verificar si necesita recompilar
        if object_path.exists():
            source_mtime = source_path.stat().st_mtime
            object_mtime = object_path.stat().st_mtime
            
            # También verificar headers
            needs_rebuild = source_mtime > object_mtime
            
            for header in module.headers:
                header_path = module.path / header
                if header_path.exists() and header_path.stat().st_mtime > object_mtime:
                    needs_rebuild = True
                    break
            
            if not needs_rebuild:
                if verbose:
                    muted(f"  ⚬ {source} (sin cambios)")
                continue
        
        # Compilar
        cmd = ["cc"] + all_cflags + ["-c", str(source_path), "-o", str(object_path)]
        
        if verbose:
            note(f"  → {source}")
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=True
            )
            if verbose and result.stdout:
                print(colored_diagnostics(result.stdout))
            if verbose and result.stderr:
                print(colored_diagnostics(result.stderr), file=sys.stderr)
        except subprocess.CalledProcessError as e:
            fail(f"✗ Error compilando {source_path}:")
            output = ((e.stdout or "") + ("\n" if e.stdout and e.stderr else "") + (e.stderr or "")).strip()
            if output:
                print(colored_diagnostics(output), file=sys.stderr)
                print_error_steps(output)
            else:
                print_error_steps(str(e))
            success = False
    
    return success

def link_executable(modules: Dict[str, Module], target: str, main_source: Optional[str] = None):
    """Linkea todos los objetos en un ejecutable."""
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    
    objects = []
    for module in modules.values():
        module_build_dir = BUILD_DIR / module.path
        for source in module.sources:
            obj_path = module_build_dir / (source.replace('.c', '.o'))
            if obj_path.exists():
                objects.append(str(obj_path))
    
    if main_source:
        # Compilar main
        includes = ["-Iinclude"] + [f"-I{m.full_include_path}" for m in modules.values()]
        cmd = ["cc", "-std=c11", "-Wall", "-Wextra"] + includes + [
            "-c", main_source, "-o", str(BUILD_DIR / "main.o")
        ]
        subprocess.run(cmd, check=True)
        objects.append(str(BUILD_DIR / "main.o"))
    
    # Linkear
    output_path = BUILD_DIR / target
    libs = ["-lm"]
    if os.name != 'nt':
        libs.append("-ldl")
    cmd = ["cc"] + os.environ.get("LUNA_CFLAGS", "").split() + objects + \
          ["-o", str(output_path)] + libs
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        if result.stdout:
            print(colored_diagnostics(result.stdout))
        if result.stderr:
            print(colored_diagnostics(result.stderr), file=sys.stderr)
        ok(f"✓ Ejecutable generado: {output_path}")
    except subprocess.CalledProcessError as e:
        fail("✗ Error linkeando:")
        output = ((e.stdout or "") + ("\n" if e.stdout and e.stderr else "") + (e.stderr or "")).strip()
        if output:
            print(colored_diagnostics(output), file=sys.stderr)
            print_error_steps(output)
        else:
            print_error_steps(str(e))
        sys.exit(1)

# ---------- Comandos ----------

def cmd_build(args):
    """Comando: build"""
    note(paint(f"Luna Build System v{VERSION}", BOLD))
    print()
    
    start_time = time.time()
    
    # Descubrir módulos
    modules = discover_modules()
    ok(f"✓ {len(modules)} módulos descubiertos")
    
    # Resolver dependencias
    order = resolve_dependencies(modules)
    
    # Compilar
    note(f"\nCompilando {len(order)} módulos...")
    success_count = 0
    
    for module in order:
        print(paint(f"\n[{module.name}]", BOLD, BLUE))
        if compile_module(module, modules, verbose=args.verbose):
            success_count += 1
    
    elapsed = time.time() - start_time
    
    print(f"\n{'='*60}")
    status = ok if success_count == len(order) else warn
    status(f"✓ {success_count}/{len(order)} módulos compilados")
    muted(f"⏱  Tiempo: {elapsed:.2f}s")
    
    if success_count == len(order):
        # Linkear
        note("\nLinkeando ejecutable...")
        link_executable(modules, "luna", main_source=args.main)
    else:
        fail("\n✗ Build fallido")
        warn("Sigue los pasos indicados arriba para cada error de compilación.")
        sys.exit(1)

def cmd_clean(args):
    """Comando: clean"""
    if BUILD_DIR.exists():
        import shutil
        shutil.rmtree(BUILD_DIR)
        ok(f"✓ Directorio {BUILD_DIR} eliminado")
    else:
        muted("⚬ Nada que limpiar")

def cmd_info(args):
    """Comando: info"""
    modules = discover_modules()
    order = resolve_dependencies(modules)
    
    print(f"Luna Project Info")
    print(f"{'='*60}")
    print(f"Módulos: {len(modules)}")
    print()
    
    for module in order:
        print(f"[{module.name}]")
        print(f"  Path: {module.path}")
        print(f"  Sources: {len(module.sources)}")
        print(f"  Headers: {len(module.headers)}")
        if module.depends:
            print(f"  Depends: {', '.join(module.depends)}")
        print()

def cmd_graph(args):
    """Comando: graph - muestra dependencias en formato DOT"""
    modules = discover_modules()
    
    print("digraph Luna {")
    print("  rankdir=LR;")
    
    for name, module in modules.items():
        for dep in module.depends:
            print(f'  "{name}" -> "{dep}";')
    
    print("}")


def cmd_test(args):
    """Comando: test - ejecuta todas las suites del proyecto"""
    root = Path(__file__).resolve().parent
    binary = root / "build" / "luna"

    if not binary.exists():
        result = subprocess.run(
            [sys.executable, str(root / "luna.py"), "build"],
            cwd=root,
        )
        if result.returncode != 0:
            return result.returncode

    suites = [
        "tests/stdlib/run.py",
        "tests/parser_diag/run.py",
        "tests/regression/run.py",
        "tests/luna/run.py",
        "tests/unit/run.py",
    ]
    failed = []

    for suite in suites:
        print(f"\n== {suite} ==")
        result = subprocess.run(
            [sys.executable, str(root / suite)],
            cwd=root,
        )
        if result.returncode != 0:
            failed.append(suite)

    if failed:
        print(f"\n{len(failed)}/{len(suites)} suites fallaron")
        return 1
    print(f"\n{len(suites)}/{len(suites)} suites pasaron")
    return 0

# ---------- Main ----------

def main():
    parser = argparse.ArgumentParser(
        description="Luna Build System",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Comandos:
  build     Compila el proyecto
  clean     Elimina el directorio build/
  test      Ejecuta todas las suites de tests
  info      Muestra información del proyecto
  graph     Genera grafo de dependencias (formato DOT)

Ejemplos:
  ./luna.py build
  ./luna.py build -v
  ./luna.py clean
  ./luna.py test
  ./luna.py info
  ./luna.py graph > deps.dot
        """
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Comando a ejecutar')
    
    # build
    build_parser = subparsers.add_parser('build', help='Compila el proyecto')
    build_parser.add_argument('-v', '--verbose', action='store_true', help='Salida detallada')
    build_parser.add_argument('--main', default='cli/main.c', help='Archivo main (default: cli/main.c)')
    
    # clean
    subparsers.add_parser('clean', help='Elimina el directorio build/')

    # test
    subparsers.add_parser('test', help='Ejecuta todas las suites de tests')
    
    # info
    subparsers.add_parser('info', help='Muestra información del proyecto')
    
    # graph
    subparsers.add_parser('graph', help='Genera grafo de dependencias')
    
    args = parser.parse_args()
    
    if args.command == 'build':
        cmd_build(args)
    elif args.command == 'clean':
        cmd_clean(args)
    elif args.command == 'test':
        sys.exit(cmd_test(args))
    elif args.command == 'info':
        cmd_info(args)
    elif args.command == 'graph':
        cmd_graph(args)
    else:
        parser.print_help()
        sys.exit(1)

if __name__ == '__main__':
    main()
