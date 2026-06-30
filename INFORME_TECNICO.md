# Informe Técnico — Diseño e Implementación de un Compilador x86-64 con Optimización

**Curso:** CS3402 / CS3025 — Compiladores (UTEC)
**Proyecto 2, opción 3:** *Diseño e Implementación de un Compilador x86 con Optimización y Evaluación Comparativa*
**Profesor:** Julio Eduardo Yarasca Moscol

---

## 0. Cómo leer este informe

Este documento explica **parte por parte** cómo está construido el compilador y, para cada
componente, indica **en qué clase del curso** se enseñó el concepto y **qué archivo/función** lo
implementa. La estructura sigue las **unidades del curso**:

| Unidad del curso | Semanas | Fase del compilador | Componente en el código |
|------------------|---------|---------------------|-------------------------|
| (Unidad 2) Análisis léxico | — | Lexer / scanner | `scanner.cpp`, `token.cpp` |
| Unidad 3 — Análisis de sintaxis | Sem2, Sem3, Sem4, Sem5, Sem6 | Parser (descenso recursivo) + AST | `parser.cpp`, `ast.h` |
| Unidad 4 — Análisis semántico | Sem7, Sem9 | Type Checker + entornos | `visitor.cpp` (`TypeCheckerVisitor`), `environment.h` |
| Unidad 5 — Generación de código | Sem10, Sem11 | Backend x86-64 | `visitor.cpp` (`GenCodeVisitor`) |
| Optimización | Sem12, Sem13 | Optimización | `visitor.cpp` (`Opt1Visitor`, `Opt2Visitor`, *peephole*) |

> **Nota sobre el patrón de diseño.** Todo el compilador se organiza alrededor del **patrón
> Visitor (familia GoF — Gang of Four)** que el profesor presentó en **Sem4**: un único Árbol de
> Sintaxis Abstracta (AST) es recorrido por varios *visitors* distintos (verificación de tipos,
> optimización, generación de código). Esta es la decisión arquitectónica central del proyecto.

---

## 1. Arquitectura general (pipeline)

El compilador traduce un programa fuente (`.txt`) a ensamblador **x86-64 en sintaxis AT&T**
(`.s`) que luego se ensambla y enlaza con `gcc` para producir un ejecutable nativo.

```
  archivo.txt
      │
      ▼
 ┌─────────────┐   tokens    ┌──────────────┐   AST    ┌──────────────────────┐
 │  Scanner    │ ──────────► │   Parser     │ ───────► │  Árbol de Sintaxis    │
 │ (léxico)    │             │ (descenso    │          │  Abstracta (AST)      │
 │ scanner.cpp │             │  recursivo)  │          │  ast.h                │
 └─────────────┘             └──────────────┘          └──────────┬───────────┘
                                                                   │  accept(Visitor*)
            ┌──────────────────────────────────────────────────────┼─────────────────┐
            ▼                          ▼                            ▼                  ▼
   ┌──────────────────┐   ┌───────────────────┐   ┌───────────────────┐   ┌──────────────────┐
   │ TypeChecker      │   │ Opt1 (plegado de  │   │ Opt2 (Sethi-      │   │ GenCodeVisitor    │
   │ (semántico)      │   │ constantes)       │   │ Ullman, etiquetas)│   │ (x86-64 AT&T)     │
   └──────────────────┘   └───────────────────┘   └───────────────────┘   └────────┬─────────┘
                                                                                    │
                                                                                    ▼
                                                                              archivo.s ──► gcc ──► ejecutable
```

El **orden real** de las pasadas está en `main.cpp`:

```cpp
// main.cpp
program = parser.parseProgram();   // léxico + sintáctico  →  AST
Opt1Visitor opt1; opt1.Opt1(program);   // optimización: plegado de constantes
Opt2Visitor opt2; opt2.Opt2(program);   // optimización: etiquetado Sethi-Ullman
GenCodeVisitor codegen(outfile);
codegen.generar(program);          // semántico (dentro) + generación de código
```

El `GenCodeVisitor::generar` ejecuta primero el `TypeCheckerVisitor` (análisis semántico) y luego
emite el ensamblador, de modo que la generación de código dispone de la tabla de símbolos,
*offsets* y conteos ya calculados.

---

## 2. Análisis léxico — Scanner (`scanner.cpp`, `token.cpp`)

> *Fase previa a la Unidad 3. Convierte el texto fuente en una secuencia de tokens, la entrada del
> analizador sintáctico (Sem2: "recibe los tokens del analizador léxico").*

### Diseño
- **`Token`** (`token.h`): un token tiene `type` (un valor del `enum Type`) y `text` (el lexema).
  El enum cubre operadores (`PLUS`, `MUL`, `POW`, `LEQ`, `AND`…), delimitadores, literales (`NUM`,
  `TRUE`, `FALSE`), identificadores (`ID`) y todas las **palabras reservadas** del lenguaje (`if`,
  `then`, `endif`, `while`, `fun`, `struct`, `new`, `return`, …).
- **`Scanner::nextToken`** (`scanner.cpp`) es un autómata a mano que:
  1. Salta espacios en blanco.
  2. Si ve un dígito → lee un **número** completo (`NUM`).
  3. Si ve una letra → lee un **identificador** y comprueba contra la tabla de palabras reservadas;
     si no coincide, es un `ID`.
  4. Si ve un símbolo → reconoce operadores de uno o dos caracteres con *lookahead* de un carácter
     (`*` vs `**`, `=` vs `==`, `<` vs `<=`, `&` → `&&`, etc.).
  5. Cualquier carácter no válido produce un token `ERR` → **error léxico**.

### Manejo de errores léxicos
Coincide con la clasificación de errores de **Sem2** ("errores léxicos / sintácticos / semánticos").
Un carácter inválido (p. ej. `@`) o un `&` suelto generan `Token::ERR`; el parser lo detecta y
lanza `Error léxico: carácter no reconocido`.

---

## 3. Unidad 3 — Análisis sintáctico (Parser + AST)

### 3.1 El lenguaje fuente como Gramática Independiente de Contexto — **Sem2**

> *Sem2: "Gramática independiente de contexto G = (V, T, P, S)", derivaciones, precedencia,
> asociatividad, ambigüedad.*

La gramática del lenguaje, escrita en **EBNF** (notación de **Sem3**, con `{ }` = repetición y
`[ ]` = opcional), es la que implementa el parser:

```ebnf
Program   → { StructDec ";" } { VarDec ";" } { FunDec }
StructDec → "struct" id "{" VarDec { ";" VarDec } "}"
VarDec    → "var" ("struct" id | id) id { "," id }
FunDec    → "fun" id id "(" [ Param { "," Param } ] ")" Body "endfun"
Param     → id id
Body      → { VarDec ";" } { Stm [ ";" ] }
Stm       → id [ "[" CE "]" [ "[" CE "]" ] | "." id ] "=" CE
          | "print" "(" CE ")"
          | "return" "(" CE ")"
          | "if" CE "then" Body [ "else" Body ] "endif"
          | "while" CE "do" Body "endwhile"
          | "do" Body "dowhile" CE "enddo"
          | "break"
          | "switch" CE { "case" num { Stm } } [ "default" { Stm } ] "endswitch"
CE        → AndExp { "||" AndExp }            (* expresión condicional *)
AndExp    → RelExp { "&&" RelExp }
RelExp    → BE   { ("<" | ">" | "<=" | ">=" | "==" | "!=") BE }
BE        → E    { ("+" | "-") E }
E         → T    { ("*" | "/") T }
T         → F    [ "**" F ]
F         → "!" F | num | "true" | "false" | "(" CE ")"
          | "new" id ( ListaOMatriz )
          | id ( "(" [ CE { "," CE } ] ")" | "[" CE "]" [ "[" CE "]" ] | "." id | ε )
```

**Precedencia y asociatividad (Sem2).** El profesor enseñó que la precedencia se modela con *"una
regla distinta por cada nivel de precedencia"* y la asociatividad izquierda con la forma
`exp → exp opsuma term`. Aquí cada nivel de precedencia es **un método** del parser, del menos al
más prioritario: `||` → `&&` → relacionales → `+ -` → `* /` → `**` → factor. La asociatividad
**izquierda** se obtiene con un bucle `while` (que es exactamente la repetición `{ }` de EBNF), de
modo que `a - b - c` se agrupa como `(a - b) - c`.

**Ambigüedad y *dangling else* (Sem2 / Sem6).** El problema del *else* ambiguo **no aparece** en
este lenguaje porque las estructuras usan **terminadores explícitos** (`endif`, `endwhile`,
`endfun`, `endswitch`). Esa es una decisión de diseño de la gramática que elimina la ambigüedad de
raíz, en línea con la "reescritura de la gramática" que vimos en Sem2.

### 3.2 Estrategia de análisis: descenso recursivo predictivo — **Sem3, Sem4**

> *Sem3: "Método descendente recursivo… cada regla de un no terminal se traduce en un
> procedimiento/función recursiva; es predictivo; requiere gramática sin recursión por la
> izquierda". Análisis LL(1): izquierda-a-derecha, derivación más a la izquierda, 1 token de
> anticipación.*

El parser (`parser.cpp`) es un **analizador descendente recursivo predictivo LL(1)**:

- **Un método por no terminal:** `parseProgram`, `parseStructDec`, `parseVarDec`, `parseFunDec`,
  `parseBody`, `parseStm`, y la cascada de expresiones `parseCE → parseLogicalAnd → parseRelExp →
  parseBE → parseE → parseT → parseF`. Esto es la traducción directa de los **diagramas de
  sintaxis** de Sem3.
- **1 token de anticipación (LL(1)):** el parser mantiene `current` (el token de *lookahead*) y
  `previous`. Los métodos auxiliares implementan exactamente las primitivas de Sem3/Sem4:
  - `check(t)` — ¿el *lookahead* es del tipo `t`? (consulta sin consumir).
  - `match(t)` — si coincide, **avanza** (consume) y devuelve `true`.
  - `expect(t)` — exige `t`; si no, error sintáctico.
  - `advance()` — pide el siguiente token al scanner.
- **Sin recursión por la izquierda:** la gramática usa la forma `BE → E { (+|-) E }` (repetición),
  no `BE → BE + E`. Esto cumple el requisito de Sem3 ("sin recursión a la izquierda") necesario
  para el descenso recursivo, y es la **factorización/eliminación de recursividad izquierda** que
  el profesor desarrolló.

**¿Por qué descenso recursivo y no LR (Sem5/Sem6)?** El profesor enseñó también el análisis
**ascendente** (shift-reduce: LR(0), SLR(1), LR(1), LALR(1) — el método de *Yacc*). Para este
proyecto elegimos el enfoque **descendente recursivo (Sem3/Sem4)** porque: (a) se escribe a mano
sin generadores, (b) produce mensajes de error muy claros, y (c) la gramática, al tener
terminadores explícitos y estar factorizada por niveles de precedencia, es naturalmente **LL(1)**.
Ambas estrategias son válidas y fueron presentadas en la Unidad 3.

### 3.3 Manejo y recuperación de errores — **Sem2, Sem4**

> *Sem4: estrategias de recuperación "Extraer" y "Explorar"; reportar de forma clara.*

El parser usa el modo **"reportar y abortar con mensaje preciso"**: `Parser::error` construye
mensajes como *"Error sintáctico: se esperaba 'then', pero se encontró …"*, indicando el token
esperado y el encontrado. Cada `parse*` valida sus terminadores (`expect(ENDIF)`, etc.). Es la
forma más estricta del manejo de errores de Sem2/Sem4. (La recuperación con pánico — *Extraer /
Explorar* — quedó documentada como mejora futura.)

### 3.4 El Árbol de Sintaxis Abstracta (AST) — **Sem2, Sem4**

> *Sem4: "AST… representa la estructura principal de un programa en forma de árbol; 'abstracto'
> porque omite detalles concretos (paréntesis, comas); punto de partida del análisis semántico".*

El AST está en `ast.h`. Cada constructo del lenguaje es una clase. La jerarquía respeta la
clasificación de Sem4 ("declaraciones, sentencias, expresiones"):

- **Expresiones** (`Exp`): `BinaryExp`, `UnaryExp`, `NumberExp`, `IdExp`, `IndexExp` (`a[i]`),
  `MatrixExp` (`a[i][j]`), `FieldExp` (`p.x`), `FcallExp` (llamadas), y los constructores de
  memoria dinámica `ExpListSize/Vals`, `ExpMatrixSize/Vals` (`new int[…]`, `new T{…}`).
- **Sentencias** (`Stm`): `AssignStm`, `PrintStm`, `ReturnStm`, `IfStm`, `WhileStm`, `DoWhileStm`,
  `BreakStm`, `SwitchStm`.
- **Declaraciones:** `VarDec`, `StructDec`, `FunDec`, y la raíz `Program`.

Los detalles concretos (paréntesis, comas, `;`) **no** aparecen como nodos — exactamente la
definición de "abstracto" de Sem4.

### 3.5 El patrón Visitor — **Sem4** (decisión arquitectónica central)

> *Sem4: "Patrón Visitor… patrón de comportamiento de la familia GoF; separa los algoritmos de las
> estructuras de datos; permite añadir operaciones sin modificar las clases; aplicación típica =
> recorridos en AST… un mismo AST sirve para imprimir, evaluar o generar código máquina".*

Esto es **literalmente** lo que hace el proyecto:

- Cada nodo del AST tiene un método `int accept(Visitor *v)` que llama a `v->visit(this)`
  (`visitor.cpp`, líneas 8–32) — el **doble despacho** del patrón Visitor.
- La clase abstracta `Visitor` (`visitor.h`) declara un `visit(...)` por cada tipo de nodo.
- Sobre **el mismo AST** operan cuatro *visitors* concretos, igual que el ejemplo del profesor
  ("imprimir / evaluar / generar código"):
  - `TypeCheckerVisitor` — verificación semántica.
  - `Opt1Visitor` — plegado de constantes.
  - `Opt2Visitor` — etiquetado de Sethi-Ullman.
  - `GenCodeVisitor` — generación de código x86-64.

Añadir una nueva operación = añadir un nuevo *visitor*, **sin tocar** las clases del AST. Ese es el
beneficio de "Separación / Extensibilidad" que destaca Sem4.

---

## 4. Unidad 4 — Análisis semántico

### 4.1 Traducción dirigida por sintaxis y atributos — **Sem7**

> *Sem7: "Atributo: cualquier propiedad… (tipo de dato, valor de una expresión, ubicación en
> memoria)". Atributos **sintetizados**: dependen de los hijos, fluyen de hijo a padre.*

El recorrido del AST con *visitors* es una **traducción dirigida por sintaxis**. Cada `visit`
calcula **atributos** de Sem7 y los propaga de los hijos hacia el padre (atributos
**sintetizados**), porque `accept`/`visit` devuelven un valor (`int`) y consultan primero a los
sub-nodos. Ejemplos concretos de atributos sintetizados en el proyecto:

- el **tipo** de cada expresión y la validez semántica (`TypeCheckerVisitor`),
- el **valor constante** de una subexpresión (`Opt1Visitor`, atributo `isConstant`/`constantValue`),
- la **etiqueta de registros** de Sethi-Ullman (`Opt2Visitor`, atributo `label`),
- la **ubicación en memoria** / *offset* de cada variable (Sem7 menciona "ubicación en memoria"
  como atributo).

Es una gramática esencialmente **S-atribuida** (todos los atributos son sintetizados), el caso que
Sem7 indica como evaluable en un solo recorrido del árbol.

### 4.2 Type Checker, tabla de símbolos, alcance y entornos — **Sem9**

> *Sem9: "Type Checker… verifica la corrección semántica de los tipos antes de generar código";
> tabla de símbolos; alcance/scope; **estructura de entornos (Environment)**; y las tareas de
> preparación para generación de código: **conteo de variables locales, conteo de parámetros,
> conteo de funciones, asignación de offsets respecto a %rbp**.*

Implementado en `TypeCheckerVisitor` (`visitor.cpp`) apoyado en `Environment<T>` (`environment.h`).

**La estructura de entornos (`environment.h`).** Es la *"estructura de entornos"* de Sem9: una
pila de *ribs* (un `unordered_map` por nivel de alcance). `add_level`/`remove_level` abren y cierran
**ámbitos** (Sem9: "alcance y visibilidad"); `lookup` busca del nivel más interno al más externo,
implementando el **scope léxico** anidado. El proyecto usa dos entornos en paralelo: `entorno`
(¿está declarada la variable?) y `tiposVar` (¿de qué tipo es?).

**Qué verifica el `TypeCheckerVisitor` (Sem9):**
- **Variable no declarada:** `visit(IdExp)` exige `entorno.check(nombre)`; si no, lanza
  *"Variable no declarada"*.
- **Funciones:** registra la **aridad** (`funAridad`) y comprueba que cada llamada (`FcallExp`)
  use el número correcto de argumentos y que la función exista.
- **Estructuras (`struct`):** registra los campos y sus **offsets** (`structFieldOffsets`),
  detecta estructuras y campos duplicados, y valida `p.x` (que `p` sea struct y tenga el campo `x`).
- **Redeclaraciones:** advierte si una variable ya fue declarada en el mismo ámbito.

**Tareas de preparación para generación de código (Sem9).** El profesor remarcó que el type
checker, además de validar, *prepara* el backend. El proyecto lo hace exactamente:
- **conteo de variables locales y parámetros** por función → `funcontador[f] = parámetros + locales`,
  que luego se usa para reservar la pila (`subq $N, %rsp`);
- **asignación de offsets** de los campos de cada struct (múltiplos de 8, tamaño de un `quad`);
- la información se traspasa al `GenCodeVisitor` en `generar()` (`funcontador`, `structFields`,
  `structFieldOffsets`).

---

## 5. Unidad 5 — Generación de código x86-64

### 5.1 Modelo de la arquitectura y formato del ensamblador — **Sem10**

> *Sem10: ensamblador **x86-64 sintaxis AT&T** (fuente→destino, prefijo `%`, sufijos de tamaño
> `b/w/l/q`); registros; **marco de pila** (`%rbp` base, `%rsp` tope); secciones `.data`/`.text`;
> `.globl`; **PLT** y `printf@PLT`; **RIP-relative** (`%rip`); `.section .note.GNU-stack`;
> direccionamiento `base + índice*escala + desplazamiento`.*

El `GenCodeVisitor` (`visitor.cpp`) emite exactamente este formato. `visit(Program)` genera el
esqueleto que enseñó Sem10:

```asm
.data
print_fmt: .string "%ld \n"      # cadena de formato para printf (función variádica)
...                              # variables globales como  nombre: .quad 0
.text
... (una etiqueta y bloque por función) ...
.section .note.GNU-stack,"",@progbits   # marca la pila como no ejecutable (seguridad en Linux)
```

- **`%rax`** es el acumulador donde toda expresión deja su resultado (convención del curso).
- **Direccionamiento de memoria** (Sem10): el acceso a un arreglo `a[i]` usa el modo
  `base + índice*escala`: `movq (%rax, %rdi, 8), %rax` (escala 8 = tamaño de un `quad`).
- **`leaq print_fmt(%rip), %rdi`** y **`call printf@PLT`** son el *RIP-relative addressing* y la
  *Procedure Linkage Table* de Sem10; antes de `printf` se hace `movq $0, %rax` porque es una
  **función variádica** (Sem10).

### 5.2 Funciones: marco de pila, llamada y retorno — **Sem11**

> *Sem11: **offset** = desplazamiento desde una base; **prólogo** `pushq %rbp; movq %rsp,%rbp;
> subq $N,%rsp`; copia de argumentos por **registros** (`%rdi,%rsi,%rdx,%rcx,%r8,%r9`); **epílogo**
> `.end_<nombre>: leave; ret`; **retorno** = valor en `%rax` + `jmp .end_<nombre>`; etiquetas
> `else_0`, `while_0`, `endwhile_1`.*

`visit(FunDec)` reproduce literalmente el esquema de Sem11:

```asm
.globl  <nombre>
<nombre>:
  pushq %rbp                 # prólogo: guarda el marco anterior
  movq  %rsp, %rbp           #          establece el nuevo marco
  subq  $N, %rsp             #          reserva N bytes para locales (N = funcontador*8)
  movq  %rdi, -8(%rbp)       # copia de parámetros desde los registros de argumentos
  ...                        # cuerpo
.end_<nombre>:               # epílogo (destino de todos los return)
  leave                      # deshace el marco de pila
  ret                        # retorna al llamador
```

- **Variables locales por offset (Sem11):** cada local/parámetro vive en `offset(%rbp)` con
  *offsets* negativos (`-8`, `-16`, …) calculados en `visit(VarDec)`.
- **Llamada (`visit(FcallExp)`):** evalúa cada argumento, lo coloca en su registro
  (`%rdi, %rsi, …`) y ejecuta `call <nombre>`; el resultado regresa en `%rax`.
- **Retorno (`visit(ReturnStm)`):** evalúa la expresión (queda en `%rax`) y hace
  `jmp .end_<nombre>`, exactamente el patrón de Sem11.
- **Recursión:** funciona sin cambios (cada llamada crea su propio marco), como en los ejemplos
  `fib`/`fac` de Sem11.

### 5.3 Traducción de estructuras de control — **Sem10, Sem11**

> *Sem11: traducción de `if/then/else` (`cmpq`, `setl`, `je else_0`, `jmp endif_0`) y de
> `while…do…endwhile` (etiquetas `while_0`, `endwhile_1`).*

Las etiquetas que genera el proyecto coinciden **con las mismas que el profesor mostró en clase**:

- `visit(IfStm)` → `cmpq $0, %rax; je else_N; … jmp endif_N; else_N: … ; endif_N:`.
- `visit(WhileStm)` → `while_N: … cmpq $0,%rax; je endwhile_N; … jmp while_N; endwhile_N:`.
- `visit(DoWhileStm)`, `visit(SwitchStm)` (con `break` saltando a la etiqueta de fin) y
  `visit(BreakStm)` siguen el mismo estilo.
- Las **comparaciones** (`<`, `>`, `==`, …) usan `cmpq` + `setl/setg/sete/...` + `movzbq`,
  la secuencia exacta de Sem10/Sem11.

### 5.4 Características del lenguaje en el backend

- **Arreglos y matrices** (memoria dinámica): `new int[n]` y `new int[f][c]` emiten `call
  malloc@PLT`; el acceso `a[i]` / `m[i][j]` usa el direccionamiento `base+índice*8`. Para matrices
  se guarda el número de columnas para linealizar `(fila*cols + col)`.
- **Structs:** `new T{}` reserva `nº_campos * 8` bytes; `p.x` accede al campo por su *offset*.
- **Tipos `int` y `bool`:** los booleanos se representan como enteros `0`/`1`.

---

## 6. Optimización — **Sem12 y Sem13**

El proyecto implementa **tres optimizaciones**, cada una correspondiente a un concepto del curso.

### 6.1 Plegado de constantes (*Constant Folding*) — **Sem12**

> *Sem12: "Constant Folding: evalúa expresiones de operandos constantes en compilación.
> Ej. 24×60×60 → 86400".*

`Opt1Visitor` (`visitor.cpp`) recorre el AST y, cuando ambos operandos de un `BinaryExp` son
constantes, **calcula el resultado en tiempo de compilación** y marca el nodo
(`isConstant = true`, `constantValue = …`). El backend, al ver un nodo constante, emite un único
`movq $valor, %rax` en vez de generar el código de la operación. Se pliegan todas las operaciones
enteras: `+ - * / **`, las comparaciones y `&& ||` (la división por cero se deja para tiempo de
ejecución). Ejemplo: `print(10 / 2)` se compila como `movq $5, %rax`.

### 6.2 Asignación de registros con Sethi-Ullman — **Sem12**

> *Sem12: "Algoritmo de Sethi-Ullman: asigna a cada nodo del árbol la cantidad mínima de registros
> necesarios… para usar la menor cantidad de registros y evitar almacenar resultados intermedios
> en memoria (stack)". Etiquetado: hoja → 1/0; nodo binario → máx(l₁,l₂) si l₁≠l₂, o l₁+1 si
> l₁=l₂. Algorithm 1 (Adaptación): casos r=0 / l>r / else.*

**Pase de etiquetado (`Opt2Visitor`).** Implementa la fórmula de Sem12: cada **hoja** recibe
etiqueta `1` (necesita un registro) y cada **nodo binario** recibe
`label = (l == r) ? l + 1 : max(l, r)`. La etiqueta indica el número mínimo de registros para
evaluar ese subárbol.

**Uso en el backend (`GenCodeVisitor::visit(BinaryExp)`).** Las etiquetas guían el **orden de
evaluación**: se evalúa **primero el subárbol más costoso** para minimizar el uso de la pila.

```cpp
if (left->label >= right->label) {     // izquierda igual o más costosa → evaluarla primero
   left;  push %rax;  right;  movq %rax,%rcx;  popq %rax;
} else {                               // derecha más costosa → evaluarla primero
   right; push %rax;  left;            popq %rcx;
}
emitBinOp(op, "%rcx");                 // %rax = izquierda <op> derecha
```

**Corrección respecto al pseudocódigo de clase.** El *Algorithm 1* de Sem12, en su caso `r = 0`,
deja los operandos como `op %rcx, %rax` con la **izquierda en `%rcx` y la derecha en `%rax`**, lo
que produce `derecha op izquierda`. Para operadores **conmutativos** (`+`, `*`) es correcto, pero
para los **no conmutativos** (`-`, `/`, y las comparaciones) invertiría el resultado (`a - b` daría
`b - a`). En nuestra implementación **garantizamos** que en todas las ramas la izquierda quede en
`%rax` y la derecha en `%rcx`, de modo que `op %rcx, %rax` calcule siempre `izquierda op derecha`.
Así, **la corrección no depende del etiquetado**: las etiquetas solo optimizan el orden, nunca
alteran el resultado.

### 6.3 Optimización de mirilla (*Peephole*) / selección de instrucción — **Sem13**

> *Sem13: "Peephole: examinan una pequeña ventana de instrucciones buscando patrones ineficientes".
> "Instruction Selection: aprovecha instrucciones complejas del ISA (CISC como x86) que combinan
> varias operaciones en una sola".*

El backend aplica una **selección de instrucción de mirilla** sobre el caso más común: cuando el
**operando derecho es una hoja** (un número o una variable), no hace falta cargarlo en un registro
temporal ni usar la pila; se **pliega como operando inmediato/memoria** de la propia instrucción
x86 (CISC permite `op mem/imm, reg`). Esto está en `leafOperand()` + `emitBinOp()`:

```asm
# x + y   (con el peephole)              # x + y   (sin él, 4 instrucciones)
movq -8(%rbp), %rax                       movq -8(%rbp), %rax
addq -16(%rbp), %rax                      movq %rax, %rcx
                                          movq -16(%rbp), %rax
                                          addq %rcx, %rax
```

Es exactamente la idea de Sem13: una instrucción CISC (`addq mem, reg`) reemplaza una secuencia de
varias, reduciendo el número total de instrucciones. (Las estructuras de análisis de flujo de
Sem13 —DAG, bloque básico, CFG— quedan documentadas como base teórica; no son necesarias para
estas optimizaciones locales.)

### 6.4 Resumen de optimizaciones vs. lo enseñado

| Optimización (curso) | Semana | ¿Implementada? | Dónde |
|----------------------|--------|----------------|-------|
| Plegado de constantes | Sem12 | ✅ Sí | `Opt1Visitor` |
| Sethi-Ullman (registros) | Sem12 | ✅ Sí | `Opt2Visitor` + `visit(BinaryExp)` |
| Peephole / selección de instrucción | Sem13 | ✅ Sí | `leafOperand`/`emitBinOp` |
| Reducción de fuerza, *loop unrolling*, *code hoisting*, *inlining*, *dead code elimination* | Sem12 | ⬜ Documentadas como mejora futura | — |
| DAG / Bloque básico / CFG | Sem13 | ⬜ Base teórica (análisis de flujo) | — |

---

## 7. Características avanzadas del lenguaje (requisitos del enunciado)

El enunciado exige un conjunto de características básicas y avanzadas. Todas las siguientes están
implementadas y **verificadas por ejecución** (tests `input11`–`input16`).

### 7.1 Cadenas de caracteres (strings) — *básica*
Literales `"..."` (token `STRING` en el lexer, nodo `StringExp`). Los literales se emiten en `.data`
como `.string` con una etiqueta, y la expresión carga su dirección (`leaq etiqueta(%rip), %rax`).
`print` detecta el tipo `string` y usa el formato `%s`. Variables `var string s`.

### 7.2 Menos unario y negación — *corrige una carencia*
`-x` (nodo `UnaryExp` con `NEG_OP`) genera `negq`; `!x` (negación lógica) se mantiene. Antes
`x = -5` daba error sintáctico.

### 7.3 Inferencia de tipos — *avanzada*
`var x = expr` deduce el tipo de `x` a partir del inicializador (`TypeCheckerVisitor::inferType`):
`"hola"`→string, `new int{..}`→list, `3.14`→float, aritmética→int. Es una forma de **atributo
sintetizado** (Sem7): el tipo de la variable se obtiene del tipo del subárbol de la derecha.

### 7.4 Punteros y memoria dinámica — *avanzada*
- `&x` (`AddrExp`) → `leaq` de la dirección de la variable.
- `*p` (`DerefExp`) → lee/escribe el valor apuntado (`movq (%rax), …`); soporta `*p = expr` como
  lvalue.
- **Memoria dinámica** ya presente: `new T[n]`, `new T[f][c]`, `new T{…}` emiten `malloc@PLT`.

### 7.5 Tipos float, promoción y conversión automática — *avanzada*
Tipo `float` (double de 64 bits) con generación de código **SSE**:
- Literales `3.14` (token `FLOATNUM`, nodo `FloatExp`) emitidos como `.double` en `.data`.
- Aritmética con `movsd/addsd/subsd/mulsd/divsd` y comparación con `ucomisd`, en `%xmm0/%xmm1`.
- **Promoción automática** int→float en operaciones mixtas (`cvtsi2sd`): `pi + 1` promueve `1`.
- **Conversión automática** float→int al asignar a un entero (`cvttsd2si`): `n = area` trunca.
- Float en **parámetros y retorno** de funciones (convención uniforme: los 64 bits viajan por
  registro entero y se mueven `%xmm0↔GPR` en los bordes).
- `print` de float usa el formato `%g`. El método `exprType` decide en cada expresión si se usa la
  ruta entera (`%rax`) o la de punto flotante (`%xmm0`).
- **Corrección de alineación de pila**: el marco se redondea a 16 bytes (la ABI x86-64 lo exige
  antes de cada `call`; sin ello, `printf` con argumentos float hace *segfault* por `movaps`).

### 7.6 Tipos genéricos / plantillas — *avanzada*
Funciones genéricas `fun T nombre<T>(T a, …)` con **borrado de tipos** (*type erasure*, como en
Java): como todos los valores ocupan 8 bytes y se pasan igual, una función genérica se compila una
sola vez y sirve para cualquier tipo. El parser reconoce la lista de parámetros de tipo `<T, …>`
(campo `typeParams`); el resto es transparente para el backend.

### 7.7 Funciones lambda — *avanzada*
`lambda(params) cuerpo endlambda` es una **función anónima** que el parser **iza** a una función
global con nombre generado (`__lambda_N`) y añade a la lista de funciones del programa. La expresión
evalúa a la **dirección de esa función** (puntero a función, `leaq`). Una variable puede guardarla y
**llamarse indirectamente** (`call *%r11`), o pasarse como argumento a otra función.

### 7.8 Resumen frente al enunciado

| Requisito del enunciado | Estado | Dónde |
|--------------------------|--------|-------|
| Tipos básicos, variables, scope, funciones, control | ✅ | parser + TypeChecker + GenCode |
| Struct, arreglos | ✅ | `StructDec`, `list`/`matrix` |
| **Cadenas de caracteres (strings)** | ✅ | `StringExp` |
| **Punteros, direccionamiento, memoria dinámica** | ✅ | `AddrExp`/`DerefExp` + `malloc` |
| **Tipos genéricos / plantillas** | ✅ | `<T>` (type erasure) |
| **Inferencia de tipos** | ✅ | `var x = expr` |
| **Conversión y promoción automática** | ✅ | float ↔ int (SSE) |
| **Arreglos multidimensionales** | ✅ | `matrix` (`new int[f][c]`) |
| **Funciones lambda** | ✅ | `lambda … endlambda` |

---

## 8. La aplicación y las pruebas

- **Driver (`main.cpp`):** recibe un archivo fuente, ejecuta el *pipeline* y escribe el `.s`.
  Admite además `--tokens` (volcado del análisis léxico) y `--ast` (impresión del árbol de sintaxis
  abstracta, el *PrintVisitor* de Sem4).
- **Banco de pruebas (`run_tests.py`):** compila el compilador, y para cada `inputN.txt`
  (1) genera el `.s`, (2) lo compara con el `.s` de referencia en `outputs/` (regresión de
  codegen) y (3) lo **ensambla, enlaza y ejecuta**, comparando la salida real con la esperada
  (validación semántica). Es portable (Linux/Windows).
- **Casos cubiertos (`inputs/`):** 16 programas — variables y aritmética, `struct`, arreglos
  (`list`), matrices (por tamaño y por valores), bucles `while` anidados, funciones (incluida
  recursión), y un caso por cada característica avanzada (strings, inferencia + menos unario,
  punteros, float, genéricos, lambdas). **Los 16 pasan ejecución y codegen.**

---

## 9. Mapa final: clase del profesor → componente del compilador

| Semana | Tema de clase | Componente implementado |
|--------|---------------|--------------------------|
| **Sem2** | GIC, derivaciones, AST, ambigüedad, precedencia, asociatividad | Gramática del lenguaje; niveles de precedencia del parser; clases del AST |
| **Sem3** | EBNF, descenso recursivo, LL(1), eliminación de recursividad izquierda, FIRST/FOLLOW | `parser.cpp` (un método por no terminal, *lookahead* de 1 token) |
| **Sem4** | Recuperación de errores, AST, **patrón Visitor (GoF)** | `ast.h` (`accept`), `Visitor` y los 4 *visitors* concretos; manejo de errores del parser |
| **Sem5** | LR(0), SLR(1), shift-reduce | Alternativa **no** elegida (se optó por descenso recursivo) — discutida en el informe |
| **Sem6** | LR(1), LALR(1), Yacc, *dangling else* | Alternativa **no** elegida; *dangling else* evitado por terminadores explícitos |
| **Sem7** | Gramáticas de atributos, traducción dirigida por sintaxis, atributos sintetizados | Atributos sintetizados que calculan los *visitors* (tipo, valor constante, etiqueta, offset) |
| **Sem9** | **Type Checker**, tabla de símbolos, scope, **Environment**, conteo de locales/params, offsets | `TypeCheckerVisitor`, `environment.h` |
| **Sem10** | x86-64 AT&T, registros, pila, `.data/.text`, PLT, RIP-relative, direccionamiento | `GenCodeVisitor::visit(Program)`, accesos a memoria, `printf@PLT` |
| **Sem11** | Stack frames, llamada/retorno, prólogo/epílogo, `if/while` | `visit(FunDec)`, `visit(FcallExp)`, `visit(ReturnStm)`, `visit(IfStm/WhileStm)` |
| **Sem12** | Optimización alto nivel: **constant folding** + **Sethi-Ullman** | `Opt1Visitor`, `Opt2Visitor` |
| **Sem13** | Optimización bajo nivel: **peephole**, selección de instrucción, DAG/CFG | *peephole* `leafOperand`/`emitBinOp` |

---

## 10. Estado frente a la rúbrica y trabajo pendiente

**Cubierto y funcionando:** diseño del lenguaje y lexer; parser + AST + tabla de símbolos;
análisis semántico (tipos, scope, structs, aridad); generación x86-64 correcta (verificada por
ejecución de 16 casos); **tres optimizaciones** con mejora medible; patrón Visitor como
arquitectura; y **todas las características avanzadas del enunciado** (strings, punteros y memoria
dinámica, genéricos/plantillas, inferencia, conversión y promoción automática de tipos, arreglos
multidimensionales y funciones lambda — ver sección 7).

**Aplicación del bonus (+3) — implementada** en la carpeta `bonus-app/` (ver su `README.md`):
una IDE web (**CompiLab**) que integra los cinco componentes exigidos por el enunciado: editor de
código, visualización del AST, generación de ensamblador x86, ejecución del binario nativo y
visualización de resultados, más un breadcrumb del pipeline y un panel de tokens. No modifica el
compilador: solo usa su binario y los modos `--tokens` / `--ast`.

**Pendiente (pospuesto por decisión del equipo):**
- **Benchmarks y comparación experimental** contra GCC/Clang (criterio "Comparación Comercial").

---

## Apéndice A. Correcciones realizadas durante la auditoría

Durante la revisión del código se detectaron y corrigieron dos errores críticos de generación de
código (que hacían fallar 4 de los 10 casos, con resultados incorrectos o *segfault*):

1. **`visit(BinaryExp)` ignoraba el operador.** El atajo `if (right->label == 0)` emitía siempre
   `addq`, de modo que toda multiplicación/resta/división y **toda comparación** se compilaba como
   suma (los bucles nunca terminaban → *segfault*; `area = ancho*alto` daba `ancho+alto`).
   *Corrección:* se reescribió `visit(BinaryExp)` con `leafOperand()`/`emitBinOp()` y el orden de
   evaluación de Sethi-Ullman, garantizando `izquierda op derecha` en todas las ramas.
2. **Plegado de constantes incorrecto.** `Opt1` marcaba como constantes `/`, `**` y las
   comparaciones sin calcular su valor → se plegaban a `0` (`10/2`→`0`, `2**3`→`0`).
   *Corrección:* `Opt1` ahora calcula correctamente todas las operaciones enteras.
3. **Validación por ejecución.** `run_tests.py` solo comparaba texto del `.s` (que nunca se había
   validado ejecutando) y tenía la ruta de `g++` fija a Windows. Se reescribió para detectar el
   compilador automáticamente y **validar ejecutando** cada programa.

Resultado: **10/10 casos correctos** en ejecución y codegen.
