from pathlib import Path
import functools
import http.server
import os
import subprocess
import sys
import tempfile
import threading

ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "build" / "luna"
TESTS = Path(__file__).resolve().parent
FIXTURES = ["core", "math_string", "collections", "system", "http"]


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, fmt, *args):
        pass


def main():
    if not BIN.exists():
        print(f"No se encontró {BIN}. Ejecuta ./luna.py build primero.")
        return 1

    failures = []
    with tempfile.TemporaryDirectory() as cwd:
        (Path(cwd) / "index.html").write_text("<hola>", encoding="utf-8")
        handler = functools.partial(QuietHandler, directory=cwd)
        httpd = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
        thread = threading.Thread(target=httpd.serve_forever, daemon=True)
        thread.start()
        env = os.environ.copy()
        env["LUNA_HTTP_TEST_URL"] = f"http://127.0.0.1:{httpd.server_address[1]}/index.html"
        try:
            for name in FIXTURES:
                source = TESTS / f"{name}.luna"
                expected = (TESTS / f"{name}.expected").read_text()
                result = subprocess.run(
                    [str(BIN), str(source)],
                    cwd=cwd,
                    env=env,
                    capture_output=True,
                    text=True,
                )

                if result.returncode != 0:
                    failures.append(name)
                    print(f"FAIL {name}.luna (exit={result.returncode})")
                    print(result.stderr, end="")
                    continue

                if result.stdout != expected:
                    failures.append(name)
                    print(f"FAIL {name}.luna (salida distinta)")
                    print("--- esperado ---")
                    print(expected, end="")
                    print("--- obtenido ---")
                    print(result.stdout, end="")
                    continue

                print(f"ok   {name}.luna")
        finally:
            httpd.shutdown()
            thread.join()

    if failures:
        print(f"\n{len(failures)}/{len(FIXTURES)} pruebas fallaron")
        return 1
    print(f"\n{len(FIXTURES)}/{len(FIXTURES)} pruebas pasaron")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
