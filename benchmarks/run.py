#!/usr/bin/env python3
import argparse
import json
import math
import os
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path
from string import Template

ROOT = Path(__file__).resolve().parent.parent
HERE = Path(__file__).resolve().parent
TMP = HERE / ".tmp"
RESULTS = HERE / "results" / "latest.json"

CASES = {
    "loop": {
        "iterations": 20_000_000,
        "kind": "int",
        "luna": Template("""let i = 0
let total = 0
while i < $N
    total = total + ((i * 3 + 7) % 1009)
    i = i + 1
print(total)
"""),
        "python": Template("""i = 0
total = 0
while i < $N:
    total += (i * 3 + 7) % 1009
    i += 1
print(total)
"""),
        "c": Template("""#include <stdio.h>
int main(void) {
    long long total = 0;
    for (long long i = 0; i < $N; i++) total += (i * 3 + 7) % 1009;
    printf("%lld\\n", total);
    return 0;
}
"""),
    },
    "fib": {
        "iterations": 28,
        "kind": "int",
        "luna": Template("""fn fib(n)
    if n < 2
        return n
    return fib(n - 1) + fib(n - 2)
print(fib($N))
"""),
        "python": Template("""def fib(n):
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)
print(fib($N))
"""),
        "c": Template("""#include <stdio.h>
long long fib(long long n) {
    if (n < 2) return n;
    return fib(n - 1) + fib(n - 2);
}
int main(void) {
    printf("%lld\\n", fib($N));
    return 0;
}
"""),
    },
    "math": {
        "iterations": 50_000_000,
        "kind": "float",
        "luna": Template("""let x = 1.0
let i = 0
while i < $N
    x = x + 0.000001
    x = x * 1.0000001
    x = x - 0.000001
    i = i + 1
print(x)
"""),
        "python": Template("""x = 1.0
i = 0
while i < $N:
    x = x + 0.000001
    x = x * 1.0000001
    x = x - 0.000001
    i += 1
print(x)
"""),
        "c": Template("""#include <stdio.h>
int main(void) {
    double x = 1.0;
    for (long long i = 0; i < $N; i++) {
        x = x + 0.000001;
        x = x * 1.0000001;
        x = x - 0.000001;
    }
    printf("%.17g\\n", x);
    return 0;
}
"""),
    },
}


def find_cc():
    configured = os.environ.get("CC")
    if configured:
        return configured
    for candidate in ("cc", "gcc", "clang"):
        if shutil.which(candidate):
            return candidate
    raise RuntimeError("No se encontró un compilador C (CC=cc/gcc/clang)")


def version_of(command):
    try:
        output = subprocess.check_output(command, text=True, stderr=subprocess.STDOUT)
        return output.strip().splitlines()[0]
    except (OSError, subprocess.CalledProcessError):
        return "desconocida"


def prepare(args):
    luna_bin = Path(args.luna)
    if not luna_bin.exists() and not args.no_build:
        subprocess.run([sys.executable, str(ROOT / "luna.py"), "build"], cwd=ROOT, check=True)
    if not luna_bin.exists():
        raise RuntimeError(f"No se encontró {luna_bin}")

    if TMP.exists():
        shutil.rmtree(TMP)
    TMP.mkdir(parents=True)
    RESULTS.parent.mkdir(parents=True, exist_ok=True)
    return luna_bin


def write_source(path, template, n):
    path.write_text(template.substitute(N=n), encoding="utf-8")
    return path


def measure(command, runs, warmup):
    for _ in range(warmup):
        subprocess.run(command, capture_output=True, text=True, check=True)

    times = []
    output = ""
    for _ in range(runs):
        start = time.perf_counter()
        proc = subprocess.run(command, capture_output=True, text=True)
        elapsed = time.perf_counter() - start
        if proc.returncode != 0:
            raise RuntimeError(f"Falló {command[0]}: {proc.stderr.strip()}")
        lines = proc.stdout.strip().splitlines()
        if not lines:
            raise RuntimeError(f"Sin salida de {command[0]}")
        times.append(elapsed)
        output = lines[-1]
    return times, output


def normalize(output, kind):
    return float(output) if kind == "float" else int(output)


def check_outputs(outputs, kind):
    values = list(outputs.values())
    if kind == "float":
        reference = values[0]
        for value in values[1:]:
            if not math.isclose(value, reference, rel_tol=1e-3, abs_tol=1e-6):
                raise RuntimeError(f"Resultados distintos: {outputs}")
    elif len(set(values)) != 1:
        raise RuntimeError(f"Resultados distintos: {outputs}")


def run_case(name, case, args, luna_bin, cc):
    n = case["iterations"]
    if name in ("loop", "math") and args.iterations:
        n = args.iterations
    if name == "fib" and args.fib_n:
        n = args.fib_n
    case_dir = TMP / name
    case_dir.mkdir()

    luna_src = write_source(case_dir / "case.luna", case["luna"], n)
    py_src = write_source(case_dir / "case.py", case["python"], n)
    c_src = write_source(case_dir / "case.c", case["c"], n)
    c_bin = case_dir / ("case.exe" if os.name == "nt" else "case")
    subprocess.run([cc, "-O2", "-DNDEBUG", str(c_src), "-o", str(c_bin)], check=True)

    commands = {
        "luna": [str(luna_bin), str(luna_src)],
        "python": [sys.executable, str(py_src)],
        "c": [str(c_bin)],
    }

    times = {}
    outputs = {}
    for engine, command in commands.items():
        times[engine], outputs[engine] = measure(command, args.runs, args.warmup)
    check_outputs({k: normalize(v, case["kind"]) for k, v in outputs.items()}, case["kind"])

    summary = {}
    luna_median = statistics.median(times["luna"])
    for engine in commands:
        median = statistics.median(times[engine])
        summary[engine] = {
            "times": times[engine],
            "median": median,
            "min": min(times[engine]),
            "ratio_vs_luna": median / luna_median,
            "output": outputs[engine],
        }
    return {"iterations": n, "kind": case["kind"], "engines": summary}


def show(results):
    print(f"{'caso':<10} {'motor':<8} {'mediana':>10} {'min':>10} {'vs Luna':>10}")
    for name, data in results.items():
        for engine, values in data["engines"].items():
            print(
                f"{name:<10} {engine:<8} {values['median']:>9.4f}s "
                f"{values['min']:>9.4f}s {values['ratio_vs_luna']:>9.2f}x"
            )
        print()


def main():
    parser = argparse.ArgumentParser(description="Benchmark reproducible de Luna")
    parser.add_argument("--runs", type=int, default=5, help="ejecuciones medidas")
    parser.add_argument("--warmup", type=int, default=1, help="ejecuciones de calentamiento")
    parser.add_argument("--iterations", type=int, default=None, help="sobrescribe N de loop y math")
    parser.add_argument("--fib-n", type=int, default=None, help="sobrescribe N del caso fib")
    parser.add_argument("--cases", default=",".join(CASES), help="casos a ejecutar, separados por coma")
    default_luna = ROOT / "build" / ("luna.exe" if os.name == "nt" else "luna")
    parser.add_argument("--luna", default=str(default_luna), help="ruta al binario de Luna")
    parser.add_argument("--no-build", action="store_true", help="no recompilar Luna")
    parser.add_argument("--json", default=str(RESULTS), help="archivo de resultados")
    args = parser.parse_args()

    if args.runs < 1 or args.warmup < 0:
        parser.error("--runs debe ser >= 1 y --warmup >= 0")

    names = [n.strip() for n in args.cases.split(",") if n.strip()]
    unknown = [n for n in names if n not in CASES]
    if unknown:
        parser.error(f"casos desconocidos: {', '.join(unknown)}")

    cc = find_cc()
    luna_bin = prepare(args)
    results = {}
    for name in names:
        results[name] = run_case(name, CASES[name], args, luna_bin, cc)

    payload = {
        "generated_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
        "git_commit": version_of(["git", "-C", str(ROOT), "rev-parse", "--short", "HEAD"]),
        "versions": {
            "luna": version_of([str(luna_bin), "--version"]),
            "python": version_of([sys.executable, "--version"]),
            "cc": version_of([cc, "--version"]),
        },
        "config": {"runs": args.runs, "warmup": args.warmup, "cases": names},
        "results": results,
    }
    output_path = Path(args.json)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    show(results)
    print(f"Resultados guardados en {output_path}")


if __name__ == "__main__":
    main()
