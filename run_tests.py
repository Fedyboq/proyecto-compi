#!/usr/bin/env python3
"""
Banco de pruebas del compilador.

Para cada inputN.txt:
  1. Genera el ensamblador (.s) con el compilador.
  2. Lo compara con el .s de referencia en outputs/  (regresion de codegen).
  3. Lo ensambla, enlaza y EJECUTA, comparando la salida real con la esperada
     (validacion de correctitud semantica, no solo textual).

Funciona en Linux y Windows (detecta g++/gcc automaticamente).
"""
import os
import shutil
import glob
import sys
import subprocess

# ---------------------------------------------------------------------------
# Salida esperada en tiempo de EJECUCION de cada programa de prueba.
# (numeros impresos por 'print', separados por espacios)
# ---------------------------------------------------------------------------
EXPECTED_RUN = {
    "1":  "15",
    "2":  "2 3",
    "3":  "2 3",
    "4":  "2 3",
    "5":  "2 3",
    "6":  "2 3",
    "7":  "495",
    "8":  "12",
    "9":  "6 8 10 12",
    "10": "24 9",
}

SOURCES = ["main.cpp", "scanner.cpp", "token.cpp",
           "parser.cpp", "ast.cpp", "visitor.cpp"]


def find_tool(*names):
    for n in names:
        p = shutil.which(n)
        if p:
            return p
    return None


def normalize(text):
    return text.replace("\r\n", "\n").strip()


def run_program(path):
    """Ensambla+enlaza un .s y devuelve (ok, salida_normalizada)."""
    gcc = find_tool("gcc", "cc")
    if gcc is None:
        return None, "(gcc no encontrado: se omite ejecucion)"
    exe = path[:-2] + (".exe" if os.name == "nt" else ".out")
    link = subprocess.run([gcc, "-no-pie", "-o", exe, path],
                          capture_output=True, text=True)
    if link.returncode != 0:
        return False, "ASM/LINK FAIL:\n" + link.stderr
    try:
        run = subprocess.run([exe], capture_output=True, text=True, timeout=10)
    except subprocess.TimeoutExpired:
        return False, "TIMEOUT (posible bucle infinito)"
    nums = " ".join(run.stdout.split())
    return True, nums


def main():
    compiler = "compilador.exe" if os.name == "nt" else "./compilador"
    gpp = find_tool("g++", "c++")
    if gpp is None:
        print("ERROR: no se encontro g++/c++ en el PATH.")
        sys.exit(1)

    print("Compilando compilador...")
    build = subprocess.run([gpp, "-std=c++17", "-O2", "-o",
                            compiler.lstrip("./")] + SOURCES,
                           capture_output=True, text=True)
    if build.returncode != 0:
        print("Error de compilacion:\n" + build.stderr)
        sys.exit(1)
    print("Compilacion exitosa.\n")

    input_dir, output_dir, temp_dir = "inputs", "outputs", "temp_outputs"
    os.makedirs(temp_dir, exist_ok=True)
    ok_all = True

    for filepath in sorted(glob.glob(os.path.join(input_dir, "input*.txt")),
                           key=lambda p: int("".join(filter(str.isdigit,
                                             os.path.basename(p))))):
        stem = os.path.splitext(os.path.basename(filepath))[0]
        num = stem.replace("input", "")
        print(f"== {stem} ==")

        gen = subprocess.run([compiler, filepath], capture_output=True, text=True)
        if gen.returncode != 0:
            print("  compilador FALLO:", gen.stderr.strip())
            ok_all = False
            continue

        generated_s = os.path.join(input_dir, f"{stem}.s")
        expected_s = os.path.join(output_dir, f"input_{num}.s")
        if not os.path.isfile(generated_s):
            print("  ERROR: no se genero", generated_s)
            ok_all = False
            continue

        # (2) regresion de codegen
        gen_txt = normalize(open(generated_s, encoding="utf-8", errors="ignore").read())
        if os.path.isfile(expected_s):
            exp_txt = normalize(open(expected_s, encoding="utf-8", errors="ignore").read())
            print("  codegen:", "OK" if gen_txt == exp_txt else "DIFIERE del .s de referencia")
            if gen_txt != exp_txt:
                ok_all = False
        else:
            print("  codegen: (sin .s de referencia)")

        # (3) validacion por ejecucion
        ran, out = run_program(generated_s)
        if ran is None:
            print("  ejecucion:", out)
        elif ran:
            exp = EXPECTED_RUN.get(num)
            if exp is None:
                print(f"  ejecucion: salida='{out}' (sin esperado definido)")
            elif out == exp:
                print(f"  ejecucion: OK -> '{out}'")
            else:
                print(f"  ejecucion: ERROR got '{out}' expected '{exp}'")
                ok_all = False
        else:
            print("  ejecucion:", out)
            ok_all = False

        shutil.move(generated_s, os.path.join(temp_dir, f"input_{num}.s"))

    print("\n" + ("Todos los tests pasaron." if ok_all else "Hubo fallas."))
    sys.exit(0 if ok_all else 1)


if __name__ == "__main__":
    main()
