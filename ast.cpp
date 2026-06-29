#include "ast.h"

Exp::~Exp() {}

BinaryExp::BinaryExp(Exp *l, Exp *r, BinaryOp o)
    : left(l), right(r), op(o) {}

BinaryExp::~BinaryExp() {
  delete left;
  delete right;
}

UnaryExp::UnaryExp(Exp *o) : operand(o) {}

UnaryExp::~UnaryExp() { delete operand; }

NumberExp::NumberExp(int v) : value(v) {}

NumberExp::~NumberExp() {}

IdExp::IdExp(const std::string &v) : value(v) {}

IdExp::~IdExp() {}

IndexExp::~IndexExp() { delete index; }

IndexExp::IndexExp(const std::string &name, Exp *index)
    : name(name), index(index) {}

FieldExp::~FieldExp() {}

FieldExp::FieldExp(const std::string &object, const std::string &field)
    : object(object), field(field) {}

Stm::~Stm() {}

AssignStm::AssignStm(Exp *target, Exp *expr) : target(target), e(expr) {}

AssignStm::~AssignStm() {}

ExpListSize::ExpListSize(std::string t, Exp *s)
    : size(s), type(t) {}

ExpListSize::~ExpListSize() {}

ExpListVals::ExpListVals(std::string t) : type(t) {}

ExpListVals::~ExpListVals() {}

ExpMatrixSize::ExpMatrixSize(std::string t, Exp *r, Exp *c)
    : type(t), rows(r), cols(c) {}

ExpMatrixSize::~ExpMatrixSize() {
  delete rows;
  delete cols;
}

ExpMatrixVals::ExpMatrixVals(std::string t, Exp *r, Exp *c)
    : type(t), rows(r), cols(c) {}

ExpMatrixVals::~ExpMatrixVals() {
  delete rows;
  delete cols;
}

MatrixExp::~MatrixExp() {
  delete row;
  delete col;
}

MatrixExp::MatrixExp(const std::string &name, Exp *row, Exp *col)
    : name(name), row(row), col(col) {}

FcallExp::FcallExp() {}

PrintStm::PrintStm(Exp *expresion) : e(expresion) {}

PrintStm::~PrintStm() {}

IfStm::IfStm(Exp *c, Body *t, Body *e) : condition(c), then(t), els(e) {}

WhileStm::WhileStm(Exp *c, Body *t) : condition(c), b(t) {}

DoWhileStm::DoWhileStm(Body *body, Exp *cond) : b(body), condition(cond) {}

SwitchStm::SwitchStm(Exp *expr) : e(expr) {}

VarDec::VarDec() {}

VarDec::~VarDec() {}

Body::Body()
    : declarations(std::list<VarDec *>()), StmList(std::list<Stm *>()) {}

Body::~Body() {}
