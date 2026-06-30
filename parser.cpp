#include "parser.h"
#include "ast.h"
#include "scanner.h"
#include "token.h"
#include <iostream>
#include <stdexcept>
#include <string>

Parser::Parser(Scanner *sc) : scanner(sc), previous(nullptr) {
  current = scanner->nextToken();
  if (current->type == Token::ERR) {
    throw std::runtime_error("Error léxico: carácter no reconocido '" +
                             current->text + "'");
  }
}

bool Parser::isAtEnd() { return current->type == Token::END; }

bool Parser::check(Token::Type ttype) {
  if (isAtEnd())
    return false;
  return current->type == ttype;
}

bool Parser::advance() {
  if (!isAtEnd()) {
    Token *temp = current;
    if (previous)
      delete previous;
    current = scanner->nextToken();
    previous = temp;
    if (current->type == Token::ERR) {
      throw std::runtime_error("Error léxico: carácter no reconocido '" +
                               current->text + "'");
    }
    return true;
  }
  return false;
}

bool Parser::match(Token::Type ttype) {
  if (check(ttype)) {
    advance();
    return true;
  }
  return false;
}

void Parser::error(const std::string &expected) {
  std::string found;
  if (isAtEnd()) {
    found = "fin de entrada";
  } else {
    found = Token::typeName(current->type);
    if (!current->text.empty())
      found += " '" + current->text + "'";
  }
  throw std::runtime_error("Error sintáctico: se esperaba " + expected +
                           ", pero se encontró " + found);
}

void Parser::expect(Token::Type ttype) {
  if (!match(ttype))
    error(Token::typeName(ttype));
}

Program *Parser::parseProgram() {
  Program *p = new Program();

  while (check(Token::STRUCT)) {
    p->sdlist.push_back(parseStructDec());
    match(Token::SEMICOL);
  }

  while (check(Token::VAR)) {
    p->vdlist.push_back(parseVarDec());
    if (check(Token::FUN) || isAtEnd())
      break;
    expect(Token::SEMICOL);
  }

  while (check(Token::FUN)) {
    p->fdlist.push_back(parseFunDec());
  }

  if (!isAtEnd()) {
    error("declaracion de estructura ('struct'), variable ('var'), funcion ('fun') o fin de entrada");
  }

  // Izar las funciones lambda como funciones globales.
  for (auto l : lambdas)
    p->fdlist.push_back(l);

  std::cout << "Parser exitoso" << std::endl;
  return p;
}

StructDec *Parser::parseStructDec() {
  StructDec *sd = new StructDec();

  expect(Token::STRUCT);

  if (!match(Token::ID))
    error("nombre de estructura despues de 'struct'");
  sd->name = previous->text;

  expect(Token::LBRACE);

  while (!check(Token::RBRACE)) {
    if (!check(Token::VAR))
      error("declaracion de campo ('var') o '}' en la estructura '" + sd->name + "'");

    sd->fields.push_back(parseVarDec());

    if (check(Token::RBRACE))
      break;
    expect(Token::SEMICOL);
  }

  expect(Token::RBRACE);
  return sd;
}

VarDec *Parser::parseVarDec() {
  expect(Token::VAR);

  bool isStruct = false;
  if (match(Token::STRUCT)) {
    if (!match(Token::ID))
      error("nombre de estructura despues de 'struct'");
    isStruct = true;
  } else if (!match(Token::ID)) {
    error("nombre de tipo o variable despues de 'var'");
  }
  std::string firstId = previous->text; // puede ser el tipo o el nombre

  VarDec *vd = new VarDec();

  // Inferencia de tipos: 'var x = expr' (no se escribió un tipo explícito).
  if (!isStruct && check(Token::ASSIGN)) {
    advance(); // consume '='
    vd->type = "";          // el tipo se infiere en el análisis semántico
    vd->vars.push_back(firstId);
    vd->init = parseCE();
    return vd;
  }

  // Forma con tipo explícito: 'var TIPO id {, id}'
  vd->type = firstId;

  if (!match(Token::ID))
    error("nombre de variable después del tipo '" + vd->type + "'");
  vd->vars.push_back(previous->text);

  while (match(Token::COMA)) {
    if (!match(Token::ID))
      error("nombre de variable después de ','");
    vd->vars.push_back(previous->text);
  }
  return vd;
}

FunDec *Parser::parseFunDec() {
  FunDec *fd = new FunDec();

  expect(Token::FUN);

  if (!match(Token::ID))
    error("tipo de retorno después de 'fun'");
  fd->tipo = previous->text;

  if (!match(Token::ID))
    error("nombre de función después del tipo '" + fd->tipo + "'");
  fd->nombre = previous->text;

  // Parámetros de tipo genéricos opcionales: fun T nombre<T, U>(...)
  if (match(Token::LE)) {
    if (!match(Token::ID))
      error("nombre de parámetro de tipo después de '<'");
    fd->typeParams.push_back(previous->text);
    while (match(Token::COMA)) {
      if (!match(Token::ID))
        error("nombre de parámetro de tipo después de ','");
      fd->typeParams.push_back(previous->text);
    }
    if (!match(Token::GT))
      error("'>' para cerrar los parámetros de tipo de '" + fd->nombre + "'");
  }

  expect(Token::LPAREN);
  while (check(Token::ID)) {
    match(Token::ID);
    fd->Ptipos.push_back(previous->text);

    if (!match(Token::ID))
      error("nombre de parámetro después del tipo '" + previous->text + "'");
    fd->Pnombres.push_back(previous->text);

    if (check(Token::RPAREN))
      break;
    if (!match(Token::COMA))
      error("',' o ')' en la lista de parámetros de '" + fd->nombre + "'");
  }
  expect(Token::RPAREN);

  fd->cuerpo = parseBody();

  if (!match(Token::ENDFUN) && !isAtEnd()) {
    error("'endfun' para cerrar la función '" + fd->nombre + "'");
  }

  return fd;
}

Body *Parser::parseBody() {
  Body *b = new Body();

  while (check(Token::VAR)) {
    b->declarations.push_back(parseVarDec());
    if (!match(Token::SEMICOL))
      break;
  }

  auto isBodyTerminator = [&]() {
    return check(Token::ENDIF) || check(Token::ENDWHILE) ||
           check(Token::ENDFUN) || check(Token::ELSE) ||
           check(Token::ENDSWITCH) || check(Token::ENDDO) ||
           check(Token::ENDLAMBDA) || isAtEnd();
  };

  auto isStmStart = [&]() {
    return check(Token::ID) || check(Token::PRINT) || check(Token::RETURN) ||
           check(Token::IF) || check(Token::WHILE) || check(Token::BREAK) ||
           check(Token::SWITCH) || check(Token::DO) || check(Token::MUL);
  };

  if (!isBodyTerminator()) {
    b->StmList.push_back(parseStm());
  }

  while (true) {
    match(Token::SEMICOL);

    if (isBodyTerminator())
      break;

    if (isStmStart()) {
      b->StmList.push_back(parseStm());
    } else {
      break;
    }
  }

  return b;
}

Stm *Parser::parseStm() {
  if (match(Token::ID)) {
    std::string variable = previous->text;

    Exp *var = nullptr;
    if (match(Token::LBRACKET)) {
      Exp *idx = parseCE();
      expect(Token::RBRACKET);
      if (match(Token::LBRACKET)) {
        Exp *col = parseCE();
        expect(Token::RBRACKET);
        var = new MatrixExp(variable, idx, col);
      } else {
        var = new IndexExp(variable, idx);
      }
    } else if (match(Token::DOT)) {
      if (!match(Token::ID))
        error("nombre de campo despues de '.'");
      var = new FieldExp(variable, previous->text);
    } else {
      var = new IdExp(variable);
    }

    if (match(Token::ASSIGN)) {
      Exp *rhs = parseCE();
      return new AssignStm(var, rhs);
    } else {
      error("Operacion de asignacion no aceptada. Se espero =");
    }
  }

  if (match(Token::MUL)) { // asignación por desreferencia: *p = expr
    Exp *p = parseF();
    Exp *target = new DerefExp(p);
    if (!match(Token::ASSIGN))
      error("'=' después de '*expr'");
    Exp *rhs = parseCE();
    return new AssignStm(target, rhs);
  }

  if (match(Token::PRINT)) {
    expect(Token::LPAREN);
    Exp *e = parseCE();
    expect(Token::RPAREN);
    return new PrintStm(e);
  }

  if (match(Token::RETURN)) {
    ReturnStm *r = new ReturnStm();
    expect(Token::LPAREN);
    r->e = parseCE();
    expect(Token::RPAREN);
    return r;
  }

  if (match(Token::IF)) {
    Exp *cond = parseCE();
    Body *tb = nullptr;
    Body *fb = nullptr;

    if (!match(Token::THEN))
      error("'then' después de la condición del 'if'");
    tb = parseBody();

    if (match(Token::ELSE))
      fb = parseBody();

    if (!match(Token::ENDIF))
      error("'endif' para cerrar el bloque 'if'");

    return new IfStm(cond, tb, fb);
  }

  if (match(Token::WHILE)) {
    Exp *cond = parseCE();

    if (!match(Token::DO))
      error("'do' después de la condición del 'while'");

    Body *b = parseBody();

    if (!match(Token::ENDWHILE))
      error("'endwhile' para cerrar el bloque 'while'");

    return new WhileStm(cond, b);
  }

  if (match(Token::DO)) {
    Body *b = parseBody();

    if (!match(Token::DOWHILE))
      error("'dowhile' después del cuerpo de do-while");

    Exp *cond = parseCE();

    if (!match(Token::ENDDO))
      error("'enddo' para cerrar el do-while");

    return new DoWhileStm(b, cond);
  }

  if (match(Token::BREAK)) {
    return new BreakStm();
  }

  if (match(Token::SWITCH)) {
    Exp *expr = parseCE();

    SwitchStm *sw = new SwitchStm(expr);
    while (match(Token::CASE)) {
      if (!match(Token::NUM))
        error("número después de case");
      int value = std::stoi(previous->text);
      CaseStm *c = new CaseStm(value);
      auto isCaseEnd = [&]() {
        return check(Token::CASE) || check(Token::DEFAULT) ||
               check(Token::ENDSWITCH);
      };
      while (!isCaseEnd()) {
        c->body.push_back(parseStm());
        if (check(Token::SEMICOL))
          advance();
      }
      sw->cases.push_back(c);
    }
    if (match(Token::DEFAULT)) {
      while (!check(Token::ENDSWITCH)) {
        sw->default_body.push_back(parseStm());
        if (check(Token::SEMICOL))
          advance();
      }
    }
    expect(Token::ENDSWITCH);
    return sw;
  }

  error("inicio de sentencia: identificador, 'print', 'return', 'if', 'while', 'do', 'break' o 'switch'");
  return nullptr;
}

Exp *Parser::parseCE() {
  Exp *l = parseLogicalAnd();
  while (match(Token::OR)) {
    Exp *r = parseLogicalAnd();
    l = new BinaryExp(l, r, OR_OP);
  }
  return l;
}

Exp *Parser::parseLogicalAnd() {
  Exp *l = parseRelExp();
  while (match(Token::AND)) {
    Exp *r = parseRelExp();
    l = new BinaryExp(l, r, AND_OP);
  }
  return l;
}

Exp *Parser::parseRelExp() {
  Exp *l = parseBE();
  while (true) {
    if (match(Token::LE)) {
      Exp *r = parseBE();
      l = new BinaryExp(l, r, LE_OP);
    } else if (match(Token::GT)) {
      Exp *r = parseBE();
      l = new BinaryExp(l, r, GT_OP);
    } else if (match(Token::LEQ)) {
      Exp *r = parseBE();
      l = new BinaryExp(l, r, LEQ_OP);
    } else if (match(Token::GEQ)) {
      Exp *r = parseBE();
      l = new BinaryExp(l, r, GEQ_OP);
    } else if (match(Token::EQ)) {
      Exp *r = parseBE();
      l = new BinaryExp(l, r, EQ_OP);
    } else if (match(Token::NE)) {
      Exp *r = parseBE();
      l = new BinaryExp(l, r, NE_OP);
    } else {
      break;
    }
  }
  return l;
}

Exp *Parser::parseBE() {
  Exp *l = parseE();
  while (match(Token::PLUS) || match(Token::MINUS)) {
    BinaryOp op = (previous->type == Token::PLUS) ? PLUS_OP : MINUS_OP;
    Exp *r = parseE();
    l = new BinaryExp(l, r, op);
  }
  return l;
}

Exp *Parser::parseE() {
  Exp *l = parseT();
  while (match(Token::MUL) || match(Token::DIV)) {
    BinaryOp op = (previous->type == Token::MUL) ? MUL_OP : DIV_OP;
    Exp *r = parseT();
    l = new BinaryExp(l, r, op);
  }
  return l;
}

Exp *Parser::parseT() {
  Exp *l = parseF();
  if (match(Token::POW)) {
    Exp *r = parseF();
    l = new BinaryExp(l, r, POW_OP);
  }
  return l;
}

Exp *Parser::parseF() {
  if (match(Token::NOT)) {
    Exp *operand = parseF();
    return new UnaryExp(operand, NOT_OP);
  }

  if (match(Token::MINUS)) {
    Exp *operand = parseF();
    return new UnaryExp(operand, NEG_OP);
  }

  if (match(Token::LAMBDA)) { // función anónima: lambda(params) cuerpo endlambda
    FunDec *fd = new FunDec();
    fd->nombre = "__lambda_" + std::to_string(lambdaCounter++);
    fd->tipo = "int";
    expect(Token::LPAREN);
    while (check(Token::ID)) {
      match(Token::ID);
      fd->Ptipos.push_back(previous->text);
      if (!match(Token::ID))
        error("nombre de parámetro en la lambda");
      fd->Pnombres.push_back(previous->text);
      if (check(Token::RPAREN))
        break;
      if (!match(Token::COMA))
        error("',' o ')' en los parámetros de la lambda");
    }
    expect(Token::RPAREN);
    fd->cuerpo = parseBody();
    if (!match(Token::ENDLAMBDA))
      error("'endlambda' para cerrar la lambda");
    lambdas.push_back(fd);
    return new LambdaExp(fd->nombre);
  }

  if (match(Token::AMP)) { // dirección-de: &x
    if (!match(Token::ID))
      error("identificador después de '&'");
    return new AddrExp(previous->text);
  }

  if (match(Token::MUL)) { // desreferencia: *p
    Exp *p = parseF();
    return new DerefExp(p);
  }

  if (match(Token::NUM))
    return new NumberExp(std::stoi(previous->text));

  if (match(Token::FLOATNUM))
    return new FloatExp(std::stod(previous->text));

  if (match(Token::STRING))
    return new StringExp(previous->text);

  if (match(Token::TRUE))
    return new NumberExp(1);
  if (match(Token::FALSE))
    return new NumberExp(0);

  if (match(Token::LPAREN)) {
    Exp *e = parseCE();
    expect(Token::RPAREN);
    return e;
  }

  if (match(Token::NEW)) {
    match(Token::ID);
    std::string type = previous->text;
    if (match(Token::LBRACKET)) {
      Exp *first = parseCE();
      expect(Token::RBRACKET);
      if (match(Token::LBRACKET)) {
        Exp *second = parseCE();
        expect(Token::RBRACKET);
        if (match(Token::LBRACE)) {
          ExpMatrixVals *e = new ExpMatrixVals(type, first, second);
          if (!check(Token::RBRACE)) {
            e->values.push_back(parseCE());
            while (match(Token::COMA))
              e->values.push_back(parseCE());
          }
          expect(Token::RBRACE);
          return e;
        }
        return new ExpMatrixSize(type, first, second);
      }
      return new ExpListSize(type, first);
    } else if (match(Token::LBRACE)) {
      ExpListVals *e = new ExpListVals(type);
      if (!check(Token::RBRACE)) {
        e->values.push_back(parseCE());
        while (match(Token::COMA))
          e->values.push_back(parseCE());
      }
      expect(Token::RBRACE);
      return e;
    }
  }

  if (match(Token::ID)) {
    std::string nom = previous->text;
    if (check(Token::LPAREN)) {
      advance();
      FcallExp *fcall = new FcallExp();
      fcall->nombre = nom;
      if (!check(Token::RPAREN)) {
        fcall->argumentos.push_back(parseCE());
        while (match(Token::COMA))
          fcall->argumentos.push_back(parseCE());
      }
      expect(Token::RPAREN);
      return fcall;
    } else if (match(Token::LBRACKET)) {
      Exp *t = parseCE();
      expect(Token::RBRACKET);
      if (match(Token::LBRACKET)) {
        Exp *col = parseCE();
        expect(Token::RBRACKET);
        return new MatrixExp(nom, t, col);
      }
      return new IndexExp(nom, t);
    } else if (match(Token::DOT)) {
      if (!match(Token::ID))
        error("nombre de campo despues de '.'");
      return new FieldExp(nom, previous->text);
    }
    return new IdExp(nom);
  }

  error("expresión: número, identificador, 'true', 'false' o '('");
  return nullptr;
}
