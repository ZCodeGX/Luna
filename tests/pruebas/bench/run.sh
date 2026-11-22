#!/usr/bin/env bash

set -u

cd "$(dirname "$0")"

echo "======================================"
echo "       LUNA vs PYTHON vs C"
echo "======================================"

gcc -O2 -DNDEBUG bench.c -o bench_c || exit 1

run_benchmark() {
    local name="$1"
    shift

    echo ""
    echo "========== $name =========="

    for i in 1 2 3 4 5; do
        echo "--- Ronda $i ---"
        /usr/bin/time -f 'Tiempo: %e s | RAM: %M KB' "$@"
    done
}

run_benchmark "LUNA" luna test.luna
run_benchmark "PYTHON" python3 bench.py
run_benchmark "C" ./bench_c

echo ""
echo "========== FINALIZADO =========="
