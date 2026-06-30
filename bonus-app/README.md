# CompiLab — Aplicación del bonus (+3)

IDE web que demuestra, de forma integrada, las distintas fases del compilador x86-64 y las
características del lenguaje. Cumple los **cinco componentes mínimos** que pide el enunciado para
reclamar el bonus:

| # | Componente exigido | En la app |
|---|--------------------|-----------|
| 1 | **Editor de código** para el lenguaje diseñado | Panel izquierdo, con resaltado de sintaxis, numeración de líneas y tabulación inteligente |
| 2 | **Visualización del AST** | Pestaña **AST**: árbol interactivo (pan/zoom), nodos coloreados por categoría (declaración / sentencia / expresión / campo) |
| 3 | **Generación de código ensamblador x86** | Pestaña **Ensamblador x86**: salida AT&T con resaltado; descargable como `.s` |
| 4 | **Ejecución o simulación del programa compilado** | El backend ensambla con `gcc -no-pie`, enlaza y **ejecuta el binario nativo** |
| 5 | **Visualización de resultados de ejecución** | Pestaña **Resultado**: salida del programa, código de salida y estado |

Extra: un **breadcrumb del pipeline** (Léxico → Sintáctico → Semántico → Generación x86 →
Ejecución) que marca en verde/ámbar/rojo en qué fase está o en cuál falló, y una pestaña de
**Tokens** (análisis léxico).

## Arquitectura

- **Backend** (`server.py`): servidor HTTP de la librería estándar de Python (sin dependencias).
  Compila el compilador del proyecto y, por cada petición, invoca:
  - `compilador --tokens` → tokens (léxico),
  - `compilador --ast` → árbol de sintaxis (se convierte a JSON),
  - `compilador` → ensamblador `.s` (semántico + generación),
  - `gcc -no-pie` + ejecución → salida del programa.
- **Frontend** (`static/`): SPA sin frameworks ni dependencias externas (HTML + CSS + JS vanilla).
  Editor con resaltado propio, renderizador del AST en SVG y resaltado de ensamblador. Diseño
  propio (sistema de tokens, glassmorphism, tema claro/oscuro).

> La app vive en su **propia carpeta** y no modifica los archivos del compilador. Solo usa el
> binario que compila a partir de las fuentes del proyecto y los modos `--tokens` / `--ast` ya
> presentes en `main.cpp`.

## Cómo ejecutarla

Requisitos: `python3`, `g++` y `gcc` (ya necesarios para el compilador).

```bash
cd bonus-app
python3 server.py
# abrir http://localhost:8000
```

El servidor compila el compilador automáticamente la primera vez. Para cambiar el puerto:
`COMPILAB_PORT=9000 python3 server.py`.

## Uso

1. Escribe o carga un **ejemplo** (botón «✦ Ejemplos» — hay uno por cada característica del
   lenguaje: strings, punteros, float, genéricos, lambdas, structs, matrices…).
2. Pulsa **«▶ Compilar y ejecutar»**.
3. Revisa cada fase en sus pestañas: Tokens, AST, Ensamblador x86 y Resultado.
