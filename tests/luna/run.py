#!/usr/bin/env python3
from pathlib import Path
import os
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "build" / "luna"
CASES = Path(__file__).resolve().parent


def read_metadata(source):
    meta = {}
    for raw in source.read_text().splitlines():
        line = raw.strip()
        if not line:
            continue
        if not line.startswith("#"):
            break
        if not line.startswith("# @"):
            continue
        key, sep, value = line[3:].partition(":")
        meta[key.strip()] = value.strip() if sep else True
    return meta


def collect_cases():
    cases = []
    for source in sorted(CASES.rglob("*.luna")):
        if "_lib" in source.relative_to(CASES).parts:
            continue
        meta = read_metadata(source)
        if meta.get("helper") or meta.get("skip"):
            continue
        cases.append((source, meta))
    return cases


def run_case(source, meta):
    label = source.relative_to(CASES).as_posix()
    args = shlex.split(str(meta.get("args", "")))
    timeout = int(meta.get("timeout", 20))
    cmd = [str(BIN), "--no-color", str(source.resolve())] + args
    env = os.environ.copy()
    lib_path = str(CASES / "_lib")
    existing = env.get("LUNA_PATH", "")
    env["LUNA_PATH"] = lib_path + (os.pathsep + existing if existing else "")
    try:
        result = subprocess.run(
            cmd,
            cwd=ROOT,
            env=env,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired:
        return f"{label}: timeout tras {timeout}s"

    expected_error = meta.get("expect-error")
    if expected_error:
        output = result.stderr + result.stdout
        if result.returncode == 0:
            return f"{label}: esperaba error, pero terminó con 0"
        if str(expected_error) not in output:
            return f"{label}: esperaba '{expected_error}' en: {output.strip()[:300]!r}"
        return None

    if result.returncode != 0:
        return f"{label}: exit={result.returncode}: {result.stderr.strip()[:300]}"
    if result.stderr.strip():
        return f"{label}: stderr inesperado: {result.stderr.strip()[:300]}"

    expected_file = source.with_suffix(".expected")
    expected = expected_file.read_text() if expected_file.exists() else ""
    if result.stdout != expected:
        return (
            f"{label}: salida distinta\n"
            f"  esperado: {expected[:300]!r}\n"
            f"  obtenido: {result.stdout[:300]!r}"
        )
    return None


def main():
    if not BIN.exists():
        print(f"No se encontró {BIN}. Ejecuta ./luna.py build primero.", file=sys.stderr)
        return 1

    cases = collect_cases()
    if not cases:
        print("No se encontraron casos .luna en tests/luna", file=sys.stderr)
        return 1

    failures = []
    for source, meta in cases:
        label = source.relative_to(CASES).as_posix()
        error = run_case(source, meta)
        if error:
            failures.append(label)
            print(f"FAIL {error}")
        else:
            print(f"ok   {label}")

    total = len(cases)
    passed = total - len(failures)
    if failures:
        print(f"\n{len(failures)}/{total} pruebas fallaron")
        return 1
    print(f"\n{passed}/{total} pruebas pasaron")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
