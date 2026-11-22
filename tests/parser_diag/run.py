#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "tests" / "parser_diag" / "fixtures"
LUNA = ROOT / "build" / "luna"


def main() -> int:
    if not LUNA.exists():
        print("build/luna no existe; ejecuta ./luna.py build", file=sys.stderr)
        return 1

    failures = []
    fixtures = sorted(FIXTURES.glob("*.luna"))
    for source in fixtures:
        expected_file = source.with_suffix(".expected")
        expected = expected_file.read_text()
        result = subprocess.run(
            [str(LUNA), "--no-color", str(source.relative_to(ROOT))],
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        if result.returncode == 0 or result.stdout != expected:
            failures.append(source.name)
            print(f"FAIL {source.name} (exit={result.returncode})")
            print("--- esperado ---")
            print(expected, end="")
            print("--- obtenido ---")
            print(result.stdout, end="")
        else:
            print(f"ok   {source.name}")

    if failures:
        print(f"\n{len(failures)}/{len(fixtures)} pruebas fallaron")
        return 1
    print(f"\n{len(fixtures)}/{len(fixtures)} pruebas pasaron")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
