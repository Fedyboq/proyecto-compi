#ifndef VISITOR_H
#define VISITOR_H

#include "ast.h"
#include "environment.h"
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

class BinaryExp;
class Body;
class BreakStm;
class DoWhileStm;
class FunDec;
class FcallExp;
class IdExp;
class IndexExp;
class IfStm;
class NumberExp;
class PrintStm;
class Program;
class ReturnStm;
class Stm;
class StructDec;
class SwitchStm;
class UnaryExp;
class VarDec;
class WhileStm;
class ExpListSize;
class ExpListVals;
class ExpMatrixSize;
class ExpMatrixVals;
class FieldExp;
class MatrixExp;

class Visitor {
public:
  virtual int visit(BinaryExp *exp) = 0;
  virtual int visit(NumberExp *exp) = 0;
  virtual int visit(IdExp *exp) = 0;
  virtual int visit(UnaryExp *exp) = 0;
  virtual int visit(IndexExp *exp) = 0;
  virtual int visit(MatrixExp *exp) = 0;
  virtual int visit(FieldExp *exp) = 0;
  virtual int visit(Program *p) = 0;
  virtual int visit(StructDec *sd) = 0;
  virtual int visit(PrintStm *stm) = 0;
  virtual int visit(WhileStm *stm) = 0;
  virtual int visit(DoWhileStm *stm) = 0;
  virtual int visit(IfStm *stm) = 0;
  virtual int visit(AssignStm *stm) = 0;
  virtual int visit(ExpListSize *stm) = 0;
  virtual int visit(ExpListVals *stm) = 0;
  virtual int visit(ExpMatrixSize *stm) = 0;
  virtual int visit(ExpMatrixVals *stm) = 0;
  virtual int visit(BreakStm *stm) = 0;
  virtual int visit(SwitchStm *stm) = 0;
  virtual int visit(Body *body) = 0;
  virtual int visit(VarDec *vd) = 0;
  virtual int visit(FcallExp *fc) = 0;
  virtual int visit(ReturnStm *r) = 0;
  virtual int visit(FunDec *fd) = 0;
};

class Opt1Visitor : public Visitor {
public:
  int Opt1(Program *program);
  int visit(BinaryExp *exp) override;
  int visit(NumberExp *exp) override;
  int visit(IdExp *exp) override;
  int visit(UnaryExp *exp) override;
  int visit(IndexExp *exp) override;
  int visit(MatrixExp *exp) override;
  int visit(FieldExp *exp) override;
  int visit(Program *p) override;
  int visit(StructDec *sd) override;
  int visit(PrintStm *stm) override;
  int visit(AssignStm *stm) override;
  int visit(ExpListSize *stm) override;
  int visit(ExpListVals *stm) override;
  int visit(ExpMatrixSize *stm) override;
  int visit(ExpMatrixVals *stm) override;
  int visit(WhileStm *stm) override;
  int visit(DoWhileStm *stm) override;
  int visit(IfStm *stm) override;
  int visit(BreakStm *stm) override;
  int visit(SwitchStm *stm) override;
  int visit(Body *body) override;
  int visit(VarDec *vd) override;
  int visit(FcallExp *fc) override;
  int visit(ReturnStm *r) override;
  int visit(FunDec *fd) override;
};

// Pase de etiquetado de Sethi-Ullman (Sem12): asigna a cada nodo del arbol de
// expresion el numero minimo de registros necesarios para evaluarlo. El
// generador de codigo usa estas etiquetas para decidir el orden de evaluacion
// (evaluar primero el subarbol mas costoso) y minimizar el uso de la pila.
class Opt2Visitor : public Visitor {
public:
  int Opt2(Program *program);
  int visit(BinaryExp *exp) override;
  int visit(NumberExp *exp) override;
  int visit(IdExp *exp) override;
  int visit(UnaryExp *exp) override;
  int visit(IndexExp *exp) override;
  int visit(MatrixExp *exp) override;
  int visit(FieldExp *exp) override;
  int visit(Program *p) override;
  int visit(StructDec *sd) override;
  int visit(PrintStm *stm) override;
  int visit(AssignStm *stm) override;
  int visit(ExpListSize *stm) override;
  int visit(ExpListVals *stm) override;
  int visit(ExpMatrixSize *stm) override;
  int visit(ExpMatrixVals *stm) override;
  int visit(WhileStm *stm) override;
  int visit(DoWhileStm *stm) override;
  int visit(IfStm *stm) override;
  int visit(BreakStm *stm) override;
  int visit(SwitchStm *stm) override;
  int visit(Body *body) override;
  int visit(VarDec *vd) override;
  int visit(FcallExp *fc) override;
  int visit(ReturnStm *r) override;
  int visit(FunDec *fd) override;
};

class TypeCheckerVisitor : public Visitor {
public:
  std::unordered_map<std::string, int> funcontador;
  std::unordered_map<std::string, int> funAridad;
  std::unordered_map<std::string, int> structFields;
  std::unordered_map<std::string, std::unordered_map<std::string, int>> structFieldOffsets;

  int locales;
  Environment<int> entorno;
  Environment<std::string> tiposVar;
  std::string funcionActual;

  int TypeChecker(Program *program);

  int visit(BinaryExp *exp) override;
  int visit(NumberExp *exp) override;
  int visit(IdExp *exp) override;
  int visit(UnaryExp *exp) override;
  int visit(IndexExp *exp) override;
  int visit(MatrixExp *exp) override;
  int visit(FieldExp *exp) override;
  int visit(Program *p) override;
  int visit(StructDec *sd) override;
  int visit(PrintStm *stm) override;
  int visit(AssignStm *stm) override;
  int visit(ExpListSize *stm) override;
  int visit(ExpListVals *stm) override;
  int visit(ExpMatrixSize *stm) override;
  int visit(ExpMatrixVals *stm) override;
  int visit(WhileStm *stm) override;
  int visit(DoWhileStm *stm) override;
  int visit(IfStm *stm) override;
  int visit(BreakStm *stm) override;
  int visit(SwitchStm *stm) override;
  int visit(Body *body) override;
  int visit(VarDec *vd) override;
  int visit(FcallExp *fc) override;
  int visit(ReturnStm *r) override;
  int visit(FunDec *fd) override;
};

enum class LValKind {
  Invalid,
  Number,
  Id,
  ListSize,
  ListVals,
  MatrixSize,
  MatrixVals,
  Index,
  Matrix,
  Field
};

struct LVal {
  LValKind kind = LValKind::Invalid;
  std::string name;
  Exp *index = nullptr;
  Exp *row = nullptr;
  Exp *col = nullptr;
  std::string object;
  std::string field;
  std::string type;
  std::vector<Exp*> values;
  bool numIsConst = false;
  int numVal = 0;
};

class GenCodeVisitor : public Visitor {
private:
  std::ostream &out;
  LVal *lvalTarget = nullptr;
  LVal captureLVal(Exp *exp);
  bool leafOperand(Exp *e, std::string &operand);
  void emitBinOp(BinaryOp op, const std::string &src);
  int storeTarget(const LVal &lv);
  int storeId    (const LVal &lv);
  int storeIndex (const LVal &lv);
  int storeMatrix(const LVal &lv);
  int storeField (const LVal &lv);

public:
  TypeCheckerVisitor tipos;
  std::unordered_map<std::string, int> funcontador;
  std::unordered_map<std::string, int> structFields;
  std::unordered_map<std::string, std::unordered_map<std::string, int>> structFieldOffsets;
  std::unordered_map<std::string, int> memoria;
  std::unordered_map<std::string, std::string> variableTypes;
  std::unordered_map<std::string, bool> structAllocated;
  std::unordered_map<std::string, int> matrixColumns;
  std::unordered_map<std::string, std::vector<std::string>> funParamNames;
  std::unordered_map<std::string, std::vector<std::string>> funParamTypes;
  std::unordered_map<std::string, std::string> currentMatrixParamLabels;
  std::unordered_map<std::string, bool> memoriaGlobal;
  int offset = -8;
  int labelcont = 0;
  bool entornoFuncion = false;
  std::string currentBreakLabel;
  std::string nombreFuncion;

  explicit GenCodeVisitor(std::ostream &out) : out(out) {}

  int generar(Program *program);

  int visit(BinaryExp *exp) override;
  int visit(NumberExp *exp) override;
  int visit(IdExp *exp) override;
  int visit(UnaryExp *exp) override;
  int visit(IndexExp *exp) override;
  int visit(MatrixExp *exp) override;
  int visit(FieldExp *exp) override;
  int visit(Program *p) override;
  int visit(StructDec *sd) override;
  int visit(PrintStm *stm) override;
  int visit(AssignStm *stm) override;
  int visit(ExpListSize *stm) override;
  int visit(ExpListVals *stm) override;
  int visit(ExpMatrixSize *stm) override;
  int visit(ExpMatrixVals *stm) override;
  int visit(WhileStm *stm) override;
  int visit(DoWhileStm *stm) override;
  int visit(IfStm *stm) override;
  int visit(BreakStm *stm) override;
  int visit(SwitchStm *stm) override;
  int visit(Body *body) override;
  int visit(VarDec *vd) override;
  int visit(FcallExp *fc) override;
  int visit(ReturnStm *r) override;
  int visit(FunDec *fd) override;
};

#endif
