from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "build" / "luna"
TESTS = Path(__file__).resolve().parent


def main():
    if not BIN.exists():
        print(f"No se encontró {BIN}. Ejecuta ./luna.py build primero.")
        return 1

    source = TESTS / "all.luna"
    expected = (TESTS / "all.expected").read_text()
    result = subprocess.run(
        [str(BIN), str(source)],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        print(f"FAIL all.luna (exit={result.returncode})")
        print(result.stderr, end="")
        return 1

    if result.stdout != expected:
        print("FAIL all.luna (salida distinta)")
        print("--- esperado ---")
        print(expected, end="")
        print("--- obtenido ---")
        print(result.stdout, end="")
        return 1

    print("ok   all.luna")
    print("\n1/1 pruebas pasaron")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
