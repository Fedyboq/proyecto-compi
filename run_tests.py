import os
import subprocess
import shutil
import glob
import sys

programa = ["main.cpp", "scanner.cpp", "token.cpp", "parser.cpp", "ast.cpp", "visitor.cpp"]

gpp_path = r"C:\msys64\mingw64\bin\g++.exe"
compile_cmd = [gpp_path, "-std=c++17", "-o", "compilador.exe"] + programa
print("Compilando compilador...")
result = subprocess.run(compile_cmd, capture_output=True, text=True)

if result.returncode != 0:
    print("Error de compilacion:")
    print(result.stderr)
    sys.exit(1)

print("Compilacion exitosa.")

input_dir = "inputs"
output_dir = "outputs"
temp_dir = "temp_outputs"
os.makedirs(temp_dir, exist_ok=True)

success = True
for filepath in sorted(glob.glob(os.path.join(input_dir, "input*.txt"))):
    filename = os.path.basename(filepath)
    stem = os.path.splitext(filename)[0]
    input_number = stem.replace("input", "")
    
    print(f"Ejecutando {filename}...")
    run_cmd = [os.path.join(".", "compilador.exe"), filepath]
    run_result = subprocess.run(run_cmd, capture_output=True, text=True)
    
    if run_result.returncode != 0:
        print(f"Error al compilar {filename}:")
        print(run_result.stderr)
        success = False
        continue
    
    generated_s = os.path.join(input_dir, f"{stem}.s")
    expected_s = os.path.join(output_dir, f"input_{input_number}.s")
    
    if os.path.isfile(generated_s):
        with open(generated_s, "r", encoding="utf-8", errors="ignore") as f1:
            gen_content = f1.read().strip().replace("\r\n", "\n")
        
        if os.path.isfile(expected_s):
            with open(expected_s, "r", encoding="utf-8", errors="ignore") as f2:
                exp_content = f2.read().strip().replace("\r\n", "\n")
            
            if gen_content == exp_content:
                print(f"  OK: {filename} coincide exactamente.")
            else:
                print(f"  ERROR: {filename} difiere del esperado.")
                success = False
        else:
            print(f"  ADVERTENCIA: esperado {expected_s} no existe.")
            success = False
            
        shutil.move(generated_s, os.path.join(temp_dir, f"input_{input_number}.s"))
    else:
        print(f"  ERROR: No se genero {generated_s}")
        success = False

if success:
    print("\nTodos los tests pasaron exitosamente!")
else:
    print("\nHubo fallas en algunos tests.")
    sys.exit(1)
