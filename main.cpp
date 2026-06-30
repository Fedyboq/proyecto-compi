#include <fstream>
#include <iostream>
#include <string>
#include "ast.h"
#include "parser.h"
#include "scanner.h"
#include "token.h"
#include "visitor.h"

// Vuelca la lista de tokens del archivo a stdout (modo --tokens, usado por la
// aplicación para visualizar la fase de análisis léxico).
static int dumpTokens(const std::string &input) {
    Scanner scanner(input.c_str());
    Token *tok;
    do {
        tok = scanner.nextToken();
        std::cout << *tok << "\n";
        if (tok->type == Token::ERR) {
            std::cout << "Error léxico: carácter inválido '" << tok->text << "'\n";
            delete tok;
            return 1;
        }
        bool fin = (tok->type == Token::END);
        delete tok;
        if (fin) break;
    } while (true);
    return 0;
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <archivo_de_entrada> [--tokens|--ast]\n";
        return 1;
    }

    std::string mode = "asm";
    if (argc >= 3) {
        std::string flag = argv[2];
        if (flag == "--tokens") mode = "tokens";
        else if (flag == "--ast") mode = "ast";
    }

    std::ifstream infile(argv[1]);
    if (!infile.is_open()) {
        std::cerr << "Error: no se pudo abrir el archivo '" << argv[1] << "'\n";
        return 1;
    }

    std::string input, line;
    while (std::getline(infile, line))
        input += line + '\n';
    infile.close();

    // Modo --tokens: solo análisis léxico.
    if (mode == "tokens")
        return dumpTokens(input);

    Scanner scanner(input.c_str());
    Parser parser(&scanner);

    Program* program = nullptr;
    try {
        program = parser.parseProgram();
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    // Modo --ast: imprime el árbol de sintaxis abstracta y termina.
    if (mode == "ast") {
        printAst(program, std::cout);
        delete program;
        return 0;
    }

    std::string inputFile(argv[1]);
    size_t dotPos = inputFile.find_last_of('.');
    std::string baseName = (dotPos == std::string::npos) ? inputFile : inputFile.substr(0, dotPos);
    std::string outputFile = baseName + ".s";

    std::ofstream outfile(outputFile);
    if (!outfile.is_open()) {
        std::cerr << "Error: no se pudo crear el archivo de salida '" << outputFile << "'\n";
        delete program;
        return 1;
    }

    Opt1Visitor opt1;
    opt1.Opt1(program);

    Opt2Visitor opt2;
    opt2.Opt2(program);

    try {
        std::cout << "Generando codigo ensamblador en '" << outputFile << "'...\n";
        GenCodeVisitor codegen(outfile);
        codegen.generar(program);
        std::cout << "Compilación exitosa.\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        outfile.close();
        delete program;
        return 1;
    }

    outfile.close();
    delete program;
    return 0;
}
