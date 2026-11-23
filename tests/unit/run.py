from pathlib import Path
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "build"
TESTS = ROOT / "tests"
OUT = BUILD / "tests" / "unit"
FIXTURE = TESTS / "test.luna"

TOOLS = [
    ("compiler", "tests/unit/compiler/test.c", "Compilation successful!"),
    ("lexer", "tests/unit/lexer/dump.c", None),
    ("parser", "tests/unit/parser/dump.c", "Program"),
]


def build_tool(name, source):
    OUT.mkdir(parents=True, exist_ok=True)
    includes = ["-Iinclude"]
    for path in sorted((ROOT / "src").iterdir()):
        if (path / "luna.build").exists():
            includes.append(f"-I{path.relative_to(ROOT)}")

    objects = sorted(
        path for path in BUILD.rglob("*.o") if path.name != "main.o"
    )
    command = [
        os.environ.get("CC", "cc"),
        "-std=c11",
        "-g",
        "-O0",
        *includes,
        str(source),
        *[str(path) for path in objects],
        "-o",
        str(OUT / name),
        "-lm",
        "-ldl",
    ]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"FAIL compilar {name}: {result.stderr.strip()}")
        return False
    return True


def main():
    if not (BUILD / "luna").exists():
        print(f"No se encontró {BUILD / 'luna'}. Ejecuta ./luna.py build primero.")
        return 1

    failures = []
    for name, source, expected in TOOLS:
        if not build_tool(name, ROOT / source):
            failures.append(name)
            continue

        result = subprocess.run(
            [str(OUT / name), str(FIXTURE)],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            failures.append(name)
            print(f"FAIL {name} (exit={result.returncode}): {result.stderr.strip()}")
            continue
        if expected and expected not in result.stdout:
            failures.append(name)
            print(f"FAIL {name} (salida inesperada): {result.stdout.strip()[:120]}")
            continue
        print(f"ok   {name}")

    if failures:
        print(f"\n{len(failures)}/{len(TOOLS)} pruebas fallaron")
        return 1
    print(f"\n{len(TOOLS)}/{len(TOOLS)} pruebas pasaron")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
