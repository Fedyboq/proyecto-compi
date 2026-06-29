#ifndef TOKEN_H
#define TOKEN_H

#include <ostream>
#include <string>

class Token {
public:
  enum Type {
    PLUS,
    MINUS,
    MUL,
    DIV,
    POW,

    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    LBRACE,
    RBRACE,
    SEMICOL,
    COMA,
    DOT,

    LE,
    GT,
    LEQ,
    GEQ,
    EQ,
    NE,

    AND,
    OR,
    NOT,

    ASSIGN,

    NUM,
    TRUE,
    FALSE,

    ID,
    SQRT,
    PRINT,

    IF,
    THEN,
    ELSE,
    ENDIF,
    WHILE,
    DOWHILE,
    DO,
    ENDWHILE,
    ENDDO,
    BREAK,
    SWITCH,
    CASE,
    DEFAULT,
    NEW,
    ENDSWITCH,

    VAR,
    STRUCT,

    FUN,
    ENDFUN,
    RETURN,

    ERR,
    END
  };

  Type type;
  std::string text;

  Token(Type type);
  Token(Type type, char c);
  Token(Type type, const std::string &source, int first, int last);

  static std::string typeName(Type t);

  friend std::ostream &operator<<(std::ostream &outs, const Token &tok);
  friend std::ostream &operator<<(std::ostream &outs, const Token *tok);
};

#endif
