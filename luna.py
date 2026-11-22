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

# ---------- Configuración ----------

VERSION = "0.1.0"
BUILD_DIR = Path("build")
SRC_DIR = Path("src")
INCLUDE_BASE = SRC_DIR

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
        print(f"✗ Error cargando {build_file}: {e}", file=sys.stderr)
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
            print(f"✗ Dependencia circular detectada: {' -> '.join(path + [name])}", file=sys.stderr)
            sys.exit(1)
        
        if name in visited:
            return
        
        if name not in modules:
            print(f"✗ Módulo desconocido: {name}", file=sys.stderr)
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
                    print(f"  ⚬ {source} (sin cambios)")
                continue
        
        # Compilar
        cmd = ["cc"] + all_cflags + ["-c", str(source_path), "-o", str(object_path)]
        
        if verbose:
            print(f"  → {source}")
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=not verbose,
                text=True,
                check=True
            )
        except subprocess.CalledProcessError as e:
            print(f"✗ Error compilando {source_path}:", file=sys.stderr)
            if not verbose:
                print(e.stdout, file=sys.stderr)
                print(e.stderr, file=sys.stderr)
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
    cmd = ["cc"] + objects + ["-o", str(output_path)] + libs
    
    try:
        subprocess.run(cmd, check=True)
        print(f"✓ Ejecutable generado: {output_path}")
    except subprocess.CalledProcessError as e:
        print(f"✗ Error linkeando:", file=sys.stderr)
        print(e.stderr, file=sys.stderr)
        sys.exit(1)

# ---------- Comandos ----------

def cmd_build(args):
    """Comando: build"""
    print(f"Luna Build System v{VERSION}")
    print()
    
    start_time = time.time()
    
    # Descubrir módulos
    modules = discover_modules()
    print(f"✓ {len(modules)} módulos descubiertos")
    
    # Resolver dependencias
    order = resolve_dependencies(modules)
    
    # Compilar
    print(f"\nCompilando {len(order)} módulos...")
    success_count = 0
    
    for module in order:
        print(f"\n[{module.name}]")
        if compile_module(module, modules, verbose=args.verbose):
            success_count += 1
    
    elapsed = time.time() - start_time
    
    print(f"\n{'='*60}")
    print(f"✓ {success_count}/{len(order)} módulos compilados")
    print(f"⏱  Tiempo: {elapsed:.2f}s")
    
    if success_count == len(order):
        # Linkear
        print(f"\nLinkeando ejecutable...")
        link_executable(modules, "luna", main_source=args.main)
    else:
        print(f"\n✗ Build fallido", file=sys.stderr)
        sys.exit(1)

def cmd_clean(args):
    """Comando: clean"""
    if BUILD_DIR.exists():
        import shutil
        shutil.rmtree(BUILD_DIR)
        print(f"✓ Directorio {BUILD_DIR} eliminado")
    else:
        print(f"⚬ Nada que limpiar")

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

# ---------- Main ----------

def main():
    parser = argparse.ArgumentParser(
        description="Luna Build System",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Comandos:
  build     Compila el proyecto
  clean     Elimina el directorio build/
  info      Muestra información del proyecto
  graph     Genera grafo de dependencias (formato DOT)

Ejemplos:
  ./luna.py build
  ./luna.py build -v
  ./luna.py clean
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
    
    # info
    subparsers.add_parser('info', help='Muestra información del proyecto')
    
    # graph
    subparsers.add_parser('graph', help='Genera grafo de dependencias')
    
    args = parser.parse_args()
    
    if args.command == 'build':
        cmd_build(args)
    elif args.command == 'clean':
        cmd_clean(args)
    elif args.command == 'info':
        cmd_info(args)
    elif args.command == 'graph':
        cmd_graph(args)
    else:
        parser.print_help()
        sys.exit(1)

if __name__ == '__main__':
    main()
