// Ejemplos que demuestran las características del lenguaje. Todos compilan y
// ejecutan correctamente con el compilador del proyecto.
window.EXAMPLES = [
  {
    id: "hola",
    title: "Hola + aritmética",
    desc: "Variables, aritmética entera y print.",
    tags: ["básico", "print"],
    code: `fun int main()
    var int x;
    var int y;
    x = 5;
    y = 10;
    print(x + y);
    return(0)
endfun`,
  },
  {
    id: "fib",
    title: "Fibonacci recursivo",
    desc: "Funciones, recursión, if y while.",
    tags: ["funciones", "recursión", "bucles"],
    code: `fun int fib(int n)
    if n < 2 then
        return(n)
    endif;
    return(fib(n - 1) + fib(n - 2))
endfun

fun int main()
    var int i;
    i = 0;
    while i < 10 do
        print(fib(i));
        i = i + 1
    endwhile;
    return(0)
endfun`,
  },
  {
    id: "strings",
    title: "Cadenas de caracteres",
    desc: "Literales string y variables de tipo string.",
    tags: ["strings"],
    code: `fun int main()
    var string s;
    s = "Hola, compilador";
    print(s);
    print("x86-64");
    return(0)
endfun`,
  },
  {
    id: "inferencia",
    title: "Inferencia + menos unario",
    desc: "Declaración con 'var x = expr' y negación.",
    tags: ["inferencia", "unario"],
    code: `fun int main()
    var x = 10;
    var y = x + 5;
    var s = "inferida";
    print(y);
    print(-y);
    print(s);
    return(0)
endfun`,
  },
  {
    id: "punteros",
    title: "Punteros",
    desc: "Dirección-de (&) y desreferencia (*).",
    tags: ["punteros", "memoria"],
    code: `fun int main()
    var int x;
    var list p;
    x = 7;
    p = &x;
    print(*p);
    *p = 42;
    print(x);
    return(0)
endfun`,
  },
  {
    id: "float",
    title: "Float, promoción y conversión",
    desc: "Punto flotante (SSE), promoción int→float y truncado.",
    tags: ["float", "promoción"],
    code: `fun float promedio(float a, float b)
    return((a + b) / 2.0)
endfun

fun int main()
    var float pi;
    var int n;
    pi = 3.14159;
    print(pi * 2.0);
    print(pi + 1);
    n = pi;
    print(n);
    print(promedio(3.0, 8.0));
    return(0)
endfun`,
  },
  {
    id: "genericos",
    title: "Genéricos / plantillas",
    desc: "Función genérica fun T mayor<T> por borrado de tipos.",
    tags: ["genéricos"],
    code: `fun T mayor<T>(T a, T b)
    if a > b then
        return(a)
    endif;
    return(b)
endfun

fun int main()
    print(mayor(7, 3));
    print(mayor(100, 250));
    return(0)
endfun`,
  },
  {
    id: "lambdas",
    title: "Funciones lambda",
    desc: "Lambda asignada a variable y pasada como argumento.",
    tags: ["lambda", "punteros a función"],
    code: `fun int aplica(list f, int x)
    return(f(x))
endfun

fun int main()
    var list cuadrado;
    cuadrado = lambda(int n) return(n * n) endlambda;
    print(cuadrado(6));
    print(aplica(cuadrado, 9));
    return(0)
endfun`,
  },
  {
    id: "struct",
    title: "Structs y campos",
    desc: "Tipos definidos por el usuario con struct.",
    tags: ["struct"],
    code: `struct Punto {
    var int x;
    var int y
};

fun int main()
    var Punto p;
    p = new Punto {};
    p.x = 3;
    p.y = 4;
    print(p.x + p.y);
    return(0)
endfun`,
  },
  {
    id: "matriz",
    title: "Matrices (arreglos 2D)",
    desc: "Arreglos multidimensionales y bucles anidados.",
    tags: ["matrices", "memoria dinámica"],
    code: `fun int main()
    var matrix m;
    var int f, c, total;
    m = new int[3][3] {11, 22, 33, 44, 55, 66, 77, 88, 99};
    f = 0;
    total = 0;
    while f < 3 do
        c = 0;
        while c < 3 do
            total = total + m[f][c];
            c = c + 1
        endwhile;
        f = f + 1
    endwhile;
    print(total);
    return(0)
endfun`,
  },
];
