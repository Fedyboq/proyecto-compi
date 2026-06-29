#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include "token.h"

class Scanner {
private:
    std::string input;
    std::size_t first;
    std::size_t current;

public:
    Scanner(const char* in_s);
    Token* nextToken();
    ~Scanner();
};

int ejecutar_scanner(Scanner* scanner, const std::string& InputFile);

#endif
