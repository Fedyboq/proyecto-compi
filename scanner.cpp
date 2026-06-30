#include "scanner.h"
#include "token.h"
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>

Scanner::Scanner(const char *s) : input(s), first(0), current(0) {}

Scanner::~Scanner() {}

static bool is_white_space(char c) {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

Token *Scanner::nextToken() {
  while (current < input.length() && is_white_space(input[current]))
    current++;

  if (current >= input.length())
    return new Token(Token::END);

  char c = input[current];
  first = current;

  if (isdigit(c)) {
    current++;
    while (current < input.length() && isdigit(input[current]))
      current++;
    // Parte decimal: número en punto flotante (p. ej. 3.14)
    if (current + 1 < input.length() && input[current] == '.' &&
        isdigit(input[current + 1])) {
      current++; // '.'
      while (current < input.length() && isdigit(input[current]))
        current++;
      return new Token(Token::FLOATNUM, input, first, current - first);
    }
    return new Token(Token::NUM, input, first, current - first);
  }

  if (c == '"') {
    current++; // comilla de apertura
    size_t start = current;
    while (current < input.length() && input[current] != '"') {
      if (input[current] == '\\' && current + 1 < input.length())
        current += 2; // saltar la secuencia de escape (p. ej. \" o \n)
      else
        current++;
    }
    if (current >= input.length()) {
      return new Token(Token::ERR, '"'); // cadena sin cerrar
    }
    std::string content = input.substr(start, current - start);
    current++; // comilla de cierre
    Token *t = new Token(Token::STRING);
    t->text = content;
    return t;
  }

  if (isalpha(c)) {
    current++;
    while (current < input.length() && isalnum(input[current]))
      current++;
    std::string lexema = input.substr(first, current - first);

    if (lexema == "sqrt")
      return new Token(Token::SQRT, input, first, current - first);
    if (lexema == "print")
      return new Token(Token::PRINT, input, first, current - first);
    if (lexema == "if")
      return new Token(Token::IF, input, first, current - first);
    if (lexema == "then")
      return new Token(Token::THEN, input, first, current - first);
    if (lexema == "else")
      return new Token(Token::ELSE, input, first, current - first);
    if (lexema == "endif")
      return new Token(Token::ENDIF, input, first, current - first);
    if (lexema == "while")
      return new Token(Token::WHILE, input, first, current - first);
    if (lexema == "do")
      return new Token(Token::DO, input, first, current - first);
    if (lexema == "enddo")
      return new Token(Token::ENDDO, input, first, current - first);
    if (lexema == "dowhile")
      return new Token(Token::DOWHILE, input, first, current - first);
    if (lexema == "endwhile")
      return new Token(Token::ENDWHILE, input, first, current - first);
    if (lexema == "break")
      return new Token(Token::BREAK, input, first, current - first);
    if (lexema == "switch")
      return new Token(Token::SWITCH, input, first, current - first);
    if (lexema == "case")
      return new Token(Token::CASE, input, first, current - first);
    if (lexema == "default")
      return new Token(Token::DEFAULT, input, first, current - first);
    if (lexema == "endswitch")
      return new Token(Token::ENDSWITCH, input, first, current - first);
    if (lexema == "var")
      return new Token(Token::VAR, input, first, current - first);
    if (lexema == "struct")
      return new Token(Token::STRUCT, input, first, current - first);
    if (lexema == "true")
      return new Token(Token::TRUE, input, first, current - first);
    if (lexema == "false")
      return new Token(Token::FALSE, input, first, current - first);
    if (lexema == "fun")
      return new Token(Token::FUN, input, first, current - first);
    if (lexema == "endfun")
      return new Token(Token::ENDFUN, input, first, current - first);
    if (lexema == "new")
      return new Token(Token::NEW, input, first, current - first);
    if (lexema == "return")
      return new Token(Token::RETURN, input, first, current - first);
    if (lexema == "lambda")
      return new Token(Token::LAMBDA, input, first, current - first);
    if (lexema == "endlambda")
      return new Token(Token::ENDLAMBDA, input, first, current - first);

    return new Token(Token::ID, input, first, current - first);
  }

  if (strchr("+/-*()[]{};=<>!&|,.", c)) {
    Token *token = nullptr;
    switch (c) {
    case '+':
      token = new Token(Token::PLUS, c);
      current++;
      break;
    case '-':
      token = new Token(Token::MINUS, c);
      current++;
      break;
    case '/':
      token = new Token(Token::DIV, c);
      current++;
      break;
    case '(':
      token = new Token(Token::LPAREN, c);
      current++;
      break;
    case ')':
      token = new Token(Token::RPAREN, c);
      current++;
      break;
    case '[':
      token = new Token(Token::LBRACKET, c);
      current++;
      break;
    case ']':
      token = new Token(Token::RBRACKET, c);
      current++;
      break;
    case '{':
      token = new Token(Token::LBRACE, c);
      current++;
      break;
    case '}':
      token = new Token(Token::RBRACE, c);
      current++;
      break;
    case ';':
      token = new Token(Token::SEMICOL, c);
      current++;
      break;
    case ',':
      token = new Token(Token::COMA, c);
      current++;
      break;
    case '.':
      token = new Token(Token::DOT, c);
      current++;
      break;
    case '*':
      if (current + 1 < input.length() && input[current + 1] == '*') {
        token = new Token(Token::POW, input, first, 2);
        current += 2;
      } else {
        token = new Token(Token::MUL, c);
        current++;
      }
      break;
    case '=':
      if (current + 1 < input.length() && input[current + 1] == '=') {
        token = new Token(Token::EQ, input, first, 2);
        current += 2;
      } else {
        token = new Token(Token::ASSIGN, c);
        current++;
      }
      break;
    case '<':
      if (current + 1 < input.length() && input[current + 1] == '=') {
        token = new Token(Token::LEQ, input, first, 2);
        current += 2;
      } else {
        token = new Token(Token::LE, c);
        current++;
      }
      break;
    case '>':
      if (current + 1 < input.length() && input[current + 1] == '=') {
        token = new Token(Token::GEQ, input, first, 2);
        current += 2;
      } else {
        token = new Token(Token::GT, c);
        current++;
      }
      break;
    case '!':
      if (current + 1 < input.length() && input[current + 1] == '=') {
        token = new Token(Token::NE, input, first, 2);
        current += 2;
      } else {
        token = new Token(Token::NOT, c);
        current++;
      }
      break;
    case '&':
      if (current + 1 < input.length() && input[current + 1] == '&') {
        token = new Token(Token::AND, input, first, 2);
        current += 2;
      } else {
        token = new Token(Token::AMP, c); // dirección-de (&x)
        current++;
      }
      break;
    case '|':
      if (current + 1 < input.length() && input[current + 1] == '|') {
        token = new Token(Token::OR, input, first, 2);
        current += 2;
      } else {
        token = new Token(Token::ERR, c);
        current++;
      }
      break;
    }
    return token;
  }

  Token *err = new Token(Token::ERR, c);
  current++;
  return err;
}

int ejecutar_scanner(Scanner *scanner, const std::string &InputFile) {
  std::string outputName = InputFile;
  size_t pos = outputName.find_last_of('.');
  if (pos != std::string::npos)
    outputName = outputName.substr(0, pos);
  outputName += "_tokens.txt";

  std::ofstream outFile(outputName);
  if (!outFile.is_open()) {
    std::cerr << "Error: no se pudo abrir el archivo de salida: " << outputName
              << std::endl;
    return 1;
  }

  outFile << "Scanner\n" << std::endl;

  Token *tok;
  while (true) {
    tok = scanner->nextToken();

    if (tok->type == Token::ERR) {
      outFile << *tok << std::endl;
      outFile << "Error léxico: carácter inválido '" << tok->text << "'\n";
      outFile << "\nScanner no exitoso\n";
      delete tok;
      outFile.close();
      return 1;
    }

    outFile << *tok << std::endl;

    if (tok->type == Token::END) {
      outFile << "\nScanner exitoso\n";
      delete tok;
      outFile.close();
      return 0;
    }

    delete tok;
  }
}
