from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "build" / "luna"
TESTS = Path(__file__).resolve().parent
FIXTURES = ["recursion", "exceptions", "upvalues"]


def run(name, cwd):
    source = TESTS / f"{name}.luna"
    expected = (TESTS / f"{name}.expected").read_text()
    result = subprocess.run(
        [str(BIN), str(source)],
        cwd=cwd,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return f"{name}.luna (exit={result.returncode}): {result.stderr.strip()}"
    if result.stdout != expected:
        return f"{name}.luna (salida distinta): {result.stdout!r} != {expected!r}"
    return None


def main():
    if not BIN.exists():
        print(f"No se encontró {BIN}. Ejecuta ./luna.py build primero.")
        return 1

    failures = []
    with tempfile.TemporaryDirectory() as cwd:
        for name in FIXTURES:
            error = run(name, cwd)
            if error:
                failures.append(name)
                print(f"FAIL {error}")
            else:
                print(f"ok   {name}.luna")

        source = TESTS / "compile_error.luna"
        result = subprocess.run(
            [str(BIN), str(source)],
            cwd=cwd,
            capture_output=True,
            text=True,
        )
        if result.returncode == 0 or "cannot reassign 'const' variable" not in result.stderr:
            failures.append("compile_error")
            print(f"FAIL compile_error.luna: {result.stderr.strip()}")
        else:
            print("ok   compile_error.luna")

    if failures:
        print(f"\n{len(failures)}/{len(FIXTURES) + 1} pruebas fallaron")
        return 1
    print(f"\n{len(FIXTURES) + 1}/{len(FIXTURES) + 1} pruebas pasaron")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
