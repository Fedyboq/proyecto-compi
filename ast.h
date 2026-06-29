#ifndef AST_H
#define AST_H

#include <list>
#include <ostream>
#include <string>
#include <vector>

class Visitor;
class VarDec;
class StructDec;
class Body;

enum BinaryOp {
  PLUS_OP,
  MINUS_OP,
  MUL_OP,
  DIV_OP,
  POW_OP,
  LE_OP,
  GT_OP,
  LEQ_OP,
  GEQ_OP,
  EQ_OP,
  NE_OP,
  AND_OP,
  OR_OP
};

class Exp {
public:
  bool isConstant = false;
  int constantValue = 0;
  int label = 0;
  bool ishoja = false;
  virtual bool isSpecialRhs() const { return false; }
  virtual int accept(Visitor *visitor) = 0;
  virtual ~Exp() = 0;
};

class BinaryExp : public Exp {
public:
  Exp *left;
  Exp *right;
  BinaryOp op;
  BinaryExp(Exp *l, Exp *r, BinaryOp op);
  int accept(Visitor *visitor) override;
  ~BinaryExp();
};

class UnaryExp : public Exp {
public:
  Exp *operand;
  UnaryExp(Exp *operand);
  int accept(Visitor *visitor) override;
  ~UnaryExp();
};

class NumberExp : public Exp {
public:
  int value;
  NumberExp(int v);
  int accept(Visitor *visitor) override;
  ~NumberExp();
};

class IdExp : public Exp {
public:
  std::string value;
  IdExp(const std::string &v);
  int accept(Visitor *visitor) override;
  ~IdExp();
};

class ExpListSize : public Exp {
public:
  Exp *size;
  std::string type;
  ExpListSize(std::string t, Exp *s);
  int accept(Visitor *visitor) override;
  ~ExpListSize();
};

class ExpListVals : public Exp {
public:
  std::string type;
  std::vector<Exp *> values;
  ExpListVals(std::string t);
  bool isSpecialRhs() const override { return true; }
  int accept(Visitor *visitor) override;
  ~ExpListVals();
};

class ExpMatrixSize : public Exp {
public:
  std::string type;
  Exp *rows;
  Exp *cols;
  ExpMatrixSize(std::string t, Exp *rows, Exp *cols);
  bool isSpecialRhs() const override { return true; }
  int accept(Visitor *visitor) override;
  ~ExpMatrixSize();
};

class ExpMatrixVals : public Exp {
public:
  std::string type;
  Exp *rows;
  Exp *cols;
  std::vector<Exp *> values;
  ExpMatrixVals(std::string t, Exp *rows, Exp *cols);
  bool isSpecialRhs() const override { return true; }
  int accept(Visitor *visitor) override;
  ~ExpMatrixVals();
};

class FcallExp : public Exp {
public:
  std::string nombre;
  std::vector<Exp *> argumentos;
  FcallExp();
  int accept(Visitor *visitor) override;
  ~FcallExp() = default;
};

class IndexExp : public Exp {
public:
  std::string name;
  Exp *index;
  IndexExp(const std::string &name, Exp *index);
  int accept(Visitor *visitor) override;
  ~IndexExp();
};

class MatrixExp : public Exp {
public:
  std::string name;
  Exp *row;
  Exp *col;
  MatrixExp(const std::string &name, Exp *row, Exp *col);
  int accept(Visitor *visitor) override;
  ~MatrixExp();
};

class FieldExp : public Exp {
public:
  std::string object;
  std::string field;
  FieldExp(const std::string &object, const std::string &field);
  int accept(Visitor *visitor) override;
  ~FieldExp();
};

class Stm {
public:
  virtual int accept(Visitor *visitor) = 0;
  virtual ~Stm() = 0;
};

class AssignStm : public Stm {
public:
  Exp *target;
  Exp *e;
  AssignStm(Exp *target, Exp *expr);
  int accept(Visitor *visitor) override;
  ~AssignStm();
};

class PrintStm : public Stm {
public:
  Exp *e;
  PrintStm(Exp *e);
  int accept(Visitor *visitor) override;
  ~PrintStm();
};

class ReturnStm : public Stm {
public:
  Exp *e;
  ReturnStm() {}
  int accept(Visitor *visitor) override;
  ~ReturnStm() {}
};

class IfStm : public Stm {
public:
  Exp *condition;
  Body *then;
  Body *els;
  IfStm(Exp *condition, Body *then, Body *els);
  int accept(Visitor *visitor) override;
  ~IfStm() {}
};

class WhileStm : public Stm {
public:
  Exp *condition;
  Body *b;
  WhileStm(Exp *condition, Body *b);
  int accept(Visitor *visitor) override;
  ~WhileStm() {}
};

class DoWhileStm : public Stm {
public:
  Body *b;
  Exp *condition;
  DoWhileStm(Body *b, Exp *condition);
  int accept(Visitor *visitor) override;
  ~DoWhileStm() {}
};

class BreakStm : public Stm {
public:
  BreakStm() {}
  int accept(Visitor *visitor) override;
  ~BreakStm() {}
};

class CaseStm {
public:
  int value;
  std::list<Stm *> body;
  CaseStm(int value) : value(value) {}
  ~CaseStm() {}
};

class SwitchStm : public Stm {
public:
  Exp *e;
  std::list<CaseStm *> cases;
  std::list<Stm *> default_body;
  SwitchStm(Exp *e);
  int accept(Visitor *visitor) override;
  ~SwitchStm() {}
};

class VarDec {
public:
  std::string type;
  std::list<std::string> vars;
  VarDec();
  int accept(Visitor *visitor);
  ~VarDec();
};

class StructDec {
public:
  std::string name;
  std::list<VarDec *> fields;
  StructDec() = default;
  int accept(Visitor *visitor);
  ~StructDec() = default;
};

class Body {
public:
  std::list<VarDec *> declarations;
  std::list<Stm *> StmList;
  Body();
  int accept(Visitor *visitor);
  ~Body();
};

class FunDec {
public:
  std::string nombre;
  std::string tipo;
  Body *cuerpo;
  std::vector<std::string> Ptipos;
  std::vector<std::string> Pnombres;
  FunDec() = default;
  int accept(Visitor *visitor);
  ~FunDec() = default;
};

class Program {
public:
  std::list<StructDec *> sdlist;
  std::list<VarDec *> vdlist;
  std::list<FunDec *> fdlist;
  Program() = default;
  int accept(Visitor *visitor);
  ~Program() = default;
};

#endif
