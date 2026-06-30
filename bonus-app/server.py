#!/usr/bin/env python3
"""
CompiLab — IDE web del compilador x86-64 (aplicación del bonus).

Servidor HTTP sin dependencias externas (solo la librería estándar de Python).
Construye el compilador si hace falta y expone un único endpoint /api/compile
que, para el código fuente recibido, devuelve las cinco fases que exige el
enunciado:

  1. Editor de código          -> (lo provee el frontend)
  2. Visualización del AST      -> árbol estructurado (JSON) del análisis sintáctico
  3. Generación de código x86   -> ensamblador AT&T
  4. Ejecución del programa      -> se ensambla, enlaza y ejecuta el binario nativo
  5. Visualización de resultados -> salida y estado de ejecución

Además devuelve los tokens (análisis léxico) para completar el panorama de fases.

Uso:
    python3 bonus-app/server.py          # abre en http://localhost:8000
"""
import http.server
import json
import os
import re
import shutil
import socketserver
import subprocess
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
STATIC = os.path.join(HERE, "static")
ROOT = os.path.dirname(HERE)              # carpeta del compilador
COMPILER = os.path.join(HERE, "compilador_bin")  # binario propio del IDE
PORT = int(os.environ.get("COMPILAB_PORT", "8000"))

SRCS = ["main.cpp", "scanner.cpp", "token.cpp", "parser.cpp", "ast.cpp", "visitor.cpp"]
NOISE = ("Parser exitoso", "Generando codigo", "Compilación exitosa", "Compilacion exitosa")
TOKEN_RE = re.compile(r'TOKEN\((\w+)(?:,\s*"(.*)")?\)')


# ───────────────────────── build ─────────────────────────
def ensure_compiler():
    gpp = shutil.which("g++") or shutil.which("c++")
    if not gpp:
        return False, "No se encontró g++/c++ en el sistema."
    r = subprocess.run([gpp, "-std=c++17", "-O2", "-o", COMPILER] + SRCS,
                       cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        return False, "Error compilando el compilador:\n" + r.stderr
    return True, ""


def run_compiler(src_path, *flags, timeout=15):
    r = subprocess.run([COMPILER, src_path, *flags],
                       capture_output=True, text=True, timeout=timeout)
    return r.returncode, r.stdout, r.stderr


def strip_noise(text):
    return "\n".join(l for l in text.splitlines()
                     if not any(l.startswith(n) for n in NOISE))


# ─────────────────── parsers de salida ───────────────────
def parse_tokens(text):
    """Convierte el volcado --tokens en una lista [{type, text}]."""
    out = []
    for line in text.splitlines():
        m = TOKEN_RE.search(line)
        if m:
            out.append({"type": m.group(1), "text": m.group(2) or ""})
    return out


def parse_ast(text):
    """Convierte el árbol indentado (--ast) en JSON anidado {label, children}."""
    lines = [l for l in strip_noise(text).splitlines() if l.strip()]
    root = {"label": "Program", "children": []}
    # pila de (nivel, nodo)
    stack = [(-1, root)]
    started = False
    for line in lines:
        indent = len(line) - len(line.lstrip(" "))
        level = indent // 2
        label = line.strip()
        if label.startswith("- "):
            label = label[2:]
        if label == "Program" and not started:
            started = True
            continue
        node = {"label": label, "children": []}
        while stack and stack[-1][0] >= level:
            stack.pop()
        parent = stack[-1][1] if stack else root
        parent["children"].append(node)
        stack.append((level, node))
    return root


def stage_of_error(stderr):
    """Determina en qué fase falló a partir del mensaje de error."""
    s = stderr or ""
    if "léxico" in s or "lexico" in s:
        return "lexico"
    if "[TypeChecker]" in s or "semántico" in s:
        return "semantico"
    if "sintáctico" in s or "sintactico" in s or "se esperaba" in s:
        return "sintactico"
    return "sintactico"


# ───────────────────── pipeline ─────────────────────
def compile_pipeline(code):
    res = {"tokens": [], "ast": None, "asm": "", "output": "",
           "error": "", "errorStage": "", "stages": {}, "exitCode": None}
    stages = res["stages"]

    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "prog.txt")
        with open(src, "w") as f:
            f.write(code)

        # 1) Análisis léxico
        rc, out, err = run_compiler(src, "--tokens")
        res["tokens"] = parse_tokens(out)
        if rc != 0:
            res["error"] = (err or out).strip() or "Error léxico."
            res["errorStage"] = "lexico"
            stages["lexico"] = "error"
            return res
        stages["lexico"] = "ok"

        # 2) Análisis sintáctico → AST
        rc, out, err = run_compiler(src, "--ast")
        if rc != 0:
            res["error"] = err.strip() or "Error de análisis sintáctico."
            res["errorStage"] = stage_of_error(err)
            stages[res["errorStage"]] = "error"
            return res
        res["ast"] = parse_ast(out)
        stages["sintactico"] = "ok"

        # 3) Análisis semántico + generación de código x86
        rc, out, err = run_compiler(src)
        if rc != 0:
            res["error"] = err.strip() or "Error de compilación."
            res["errorStage"] = stage_of_error(err)
            stages[res["errorStage"]] = "error"
            return res
        stages["semantico"] = "ok"
        s_path = os.path.join(tmp, "prog.s")
        if os.path.isfile(s_path):
            with open(s_path) as f:
                res["asm"] = f.read().strip()
            stages["generacion"] = "ok"

        # 4) Ensamblar, enlazar y ejecutar
        gcc = shutil.which("gcc") or shutil.which("cc")
        if gcc and res["asm"]:
            exe = os.path.join(tmp, "prog.out")
            link = subprocess.run([gcc, "-no-pie", "-o", exe, s_path],
                                  capture_output=True, text=True)
            if link.returncode != 0:
                res["error"] = "Error al ensamblar/enlazar:\n" + link.stderr.strip()
                res["errorStage"] = "generacion"
                stages["generacion"] = "error"
                return res
            try:
                run = subprocess.run([exe], capture_output=True, text=True, timeout=10)
                res["output"] = run.stdout
                res["exitCode"] = run.returncode
                stages["ejecucion"] = "ok" if run.returncode == 0 else "warn"
            except subprocess.TimeoutExpired:
                res["error"] = "Tiempo de ejecución agotado (posible bucle infinito)."
                res["errorStage"] = "ejecucion"
                stages["ejecucion"] = "error"
        return res


# ───────────────────── servidor ─────────────────────
class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **kw):
        super().__init__(*a, directory=STATIC, **kw)

    def log_message(self, *a):
        pass

    def _json(self, obj, code=200):
        payload = json.dumps(obj).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def do_POST(self):
        if self.path != "/api/compile":
            self.send_error(404)
            return
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length)
        try:
            code = json.loads(body).get("code", "")
            result = compile_pipeline(code)
        except subprocess.TimeoutExpired:
            result = {"error": "El compilador tardó demasiado.", "errorStage": "sintactico"}
        except Exception as e:
            result = {"error": str(e)}
        self._json(result)


def main():
    ok, msg = ensure_compiler()
    if not ok:
        print(msg)
        return
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"CompiLab — IDE del compilador x86-64")
        print(f"  →  http://localhost:{PORT}")
        print("  Ctrl+C para detener.")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nDetenido.")


if __name__ == "__main__":
    main()
