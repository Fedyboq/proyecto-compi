#include "token.h"

Token::Token(Type type) : type(type), text("") {}

Token::Token(Type type, char c) : type(type), text(std::string(1, c)) {}

Token::Token(Type type, const std::string &source, int first, int last)
    : type(type), text(source.substr(first, last)) {}

std::string Token::typeName(Type t) {
  switch (t) {
  case PLUS: return "'+'";
  case MINUS: return "'-'";
  case MUL: return "'*'";
  case DIV: return "'/'";
  case POW: return "'**'";
  case LPAREN: return "'('";
  case RPAREN: return "')'";
  case LBRACKET: return "'['";
  case RBRACKET: return "']'";
  case LBRACE: return "'{'";
  case RBRACE: return "'}'";
  case SEMICOL: return "';'";
  case COMA: return "','";
  case DOT: return "'.'";
  case LE: return "'<'";
  case GT: return "'>'";
  case LEQ: return "'<='";
  case GEQ: return "'>='";
  case EQ: return "'=='";
  case NE: return "'!='";
  case AND: return "'&&'";
  case OR: return "'||'";
  case NOT: return "'!'";
  case AMP: return "'&'";
  case ASSIGN: return "'='";
  case NUM: return "número";
  case FLOATNUM: return "número decimal";
  case STRING: return "cadena";
  case TRUE: return "'true'";
  case FALSE: return "'false'";
  case ID: return "identificador";
  case SQRT: return "'sqrt'";
  case PRINT: return "'print'";
  case IF: return "'if'";
  case THEN: return "'then'";
  case ELSE: return "'else'";
  case ENDIF: return "'endif'";
  case WHILE: return "'while'";
  case DO: return "'do'";
  case DOWHILE: return "'dowhile'";
  case ENDDO: return "'enddo'";
  case ENDWHILE: return "'endwhile'";
  case BREAK: return "'break'";
  case SWITCH: return "'switch'";
  case CASE: return "'case'";
  case DEFAULT: return "'default'";
  case ENDSWITCH: return "'endswitch'";
  case VAR: return "'var'";
  case STRUCT: return "'struct'";
  case FUN: return "'fun'";
  case ENDFUN: return "'endfun'";
  case RETURN: return "'return'";
  case LAMBDA: return "'lambda'";
  case ENDLAMBDA: return "'endlambda'";
  case ERR: return "<error léxico>";
  case END: return "fin de entrada";
  default: return "<desconocido>";
  }
}

std::ostream &operator<<(std::ostream &outs, const Token &tok) {
  switch (tok.type) {
  case Token::PLUS: outs << "TOKEN(PLUS, \"" << tok.text << "\")"; break;
  case Token::MINUS: outs << "TOKEN(MINUS, \"" << tok.text << "\")"; break;
  case Token::MUL: outs << "TOKEN(MUL, \"" << tok.text << "\")"; break;
  case Token::DIV: outs << "TOKEN(DIV, \"" << tok.text << "\")"; break;
  case Token::POW: outs << "TOKEN(POW, \"" << tok.text << "\")"; break;
  case Token::LPAREN: outs << "TOKEN(LPAREN, \"" << tok.text << "\")"; break;
  case Token::RPAREN: outs << "TOKEN(RPAREN, \"" << tok.text << "\")"; break;
  case Token::LBRACKET: outs << "TOKEN(LBRACKET, \"" << tok.text << "\")"; break;
  case Token::RBRACKET: outs << "TOKEN(RBRACKET, \"" << tok.text << "\")"; break;
  case Token::LBRACE: outs << "TOKEN(LBRACE, \"" << tok.text << "\")"; break;
  case Token::RBRACE: outs << "TOKEN(RBRACE, \"" << tok.text << "\")"; break;
  case Token::SEMICOL: outs << "TOKEN(SEMICOL, \"" << tok.text << "\")"; break;
  case Token::COMA: outs << "TOKEN(COMA, \"" << tok.text << "\")"; break;
  case Token::DOT: outs << "TOKEN(DOT, \"" << tok.text << "\")"; break;
  case Token::LE: outs << "TOKEN(LE, \"" << tok.text << "\")"; break;
  case Token::GT: outs << "TOKEN(GT, \"" << tok.text << "\")"; break;
  case Token::LEQ: outs << "TOKEN(LEQ, \"" << tok.text << "\")"; break;
  case Token::GEQ: outs << "TOKEN(GEQ, \"" << tok.text << "\")"; break;
  case Token::EQ: outs << "TOKEN(EQ, \"" << tok.text << "\")"; break;
  case Token::NE: outs << "TOKEN(NE, \"" << tok.text << "\")"; break;
  case Token::AND: outs << "TOKEN(AND, \"" << tok.text << "\")"; break;
  case Token::OR: outs << "TOKEN(OR, \"" << tok.text << "\")"; break;
  case Token::NOT: outs << "TOKEN(NOT, \"" << tok.text << "\")"; break;
  case Token::AMP: outs << "TOKEN(AMP, \"" << tok.text << "\")"; break;
  case Token::ASSIGN: outs << "TOKEN(ASSIGN, \"" << tok.text << "\")"; break;
  case Token::NUM: outs << "TOKEN(NUM, \"" << tok.text << "\")"; break;
  case Token::FLOATNUM: outs << "TOKEN(FLOATNUM, \"" << tok.text << "\")"; break;
  case Token::STRING: outs << "TOKEN(STRING, \"" << tok.text << "\")"; break;
  case Token::TRUE: outs << "TOKEN(TRUE, \"" << tok.text << "\")"; break;
  case Token::FALSE: outs << "TOKEN(FALSE, \"" << tok.text << "\")"; break;
  case Token::ID: outs << "TOKEN(ID, \"" << tok.text << "\")"; break;
  case Token::SQRT: outs << "TOKEN(SQRT, \"" << tok.text << "\")"; break;
  case Token::PRINT: outs << "TOKEN(PRINT, \"" << tok.text << "\")"; break;
  case Token::IF: outs << "TOKEN(IF, \"" << tok.text << "\")"; break;
  case Token::THEN: outs << "TOKEN(THEN, \"" << tok.text << "\")"; break;
  case Token::ELSE: outs << "TOKEN(ELSE, \"" << tok.text << "\")"; break;
  case Token::ENDIF: outs << "TOKEN(ENDIF, \"" << tok.text << "\")"; break;
  case Token::DOWHILE: outs << "TOKEN(DOWHILE, \"" << tok.text << "\")"; break;
  case Token::WHILE: outs << "TOKEN(WHILE, \"" << tok.text << "\")"; break;
  case Token::DO: outs << "TOKEN(DO, \"" << tok.text << "\")"; break;
  case Token::ENDDO: outs << "TOKEN(ENDDO, \"" << tok.text << "\")"; break;
  case Token::ENDWHILE: outs << "TOKEN(ENDWHILE, \"" << tok.text << "\")"; break;
  case Token::BREAK: outs << "TOKEN(BREAK, \"" << tok.text << "\")"; break;
  case Token::SWITCH: outs << "TOKEN(SWITCH, \"" << tok.text << "\")"; break;
  case Token::CASE: outs << "TOKEN(CASE, \"" << tok.text << "\")"; break;
  case Token::DEFAULT: outs << "TOKEN(DEFAULT, \"" << tok.text << "\")"; break;
  case Token::ENDSWITCH: outs << "TOKEN(ENDSWITCH, \"" << tok.text << "\")"; break;
  case Token::VAR: outs << "TOKEN(VAR, \"" << tok.text << "\")"; break;
  case Token::STRUCT: outs << "TOKEN(STRUCT, \"" << tok.text << "\")"; break;
  case Token::FUN: outs << "TOKEN(FUN, \"" << tok.text << "\")"; break;
  case Token::ENDFUN: outs << "TOKEN(ENDFUN, \"" << tok.text << "\")"; break;
  case Token::RETURN: outs << "TOKEN(RETURN, \"" << tok.text << "\")"; break;
  case Token::LAMBDA: outs << "TOKEN(LAMBDA, \"" << tok.text << "\")"; break;
  case Token::ENDLAMBDA: outs << "TOKEN(ENDLAMBDA, \"" << tok.text << "\")"; break;
  case Token::NEW: outs << "TOKEN(NEW, \"" << tok.text << "\")"; break;
  case Token::ERR: outs << "TOKEN(ERR, \"" << tok.text << "\")"; break;
  case Token::END: outs << "TOKEN(END)"; break;
  }
  return outs;
}

std::ostream &operator<<(std::ostream &outs, const Token *tok) {
  if (!tok)
    return outs << "TOKEN(NULL)";
  return outs << *tok;
}
