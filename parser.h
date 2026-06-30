#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "scanner.h"

class Parser {
private:
  Scanner *scanner;
  Token *current;
  Token *previous;
  std::vector<FunDec *> lambdas; // funciones lambda izadas a nivel global
  int lambdaCounter = 0;

  bool match(Token::Type ttype);
  bool check(Token::Type ttype);
  bool advance();
  bool isAtEnd();

  void error(const std::string &expected);
  void expect(Token::Type ttype);

public:
  Parser(Scanner *scanner);

  Program *parseProgram();
  StructDec *parseStructDec();
  FunDec *parseFunDec();
  Body *parseBody();
  VarDec *parseVarDec();
  Stm *parseStm();
  Exp *parseCE();
  Exp *parseLogicalAnd();
  Exp *parseRelExp();
  Exp *parseBE();
  Exp *parseE();
  Exp *parseT();
  Exp *parseF();
};

#endif
