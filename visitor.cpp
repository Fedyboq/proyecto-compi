#include "visitor.h"
#include "ast.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

int BinaryExp::accept(Visitor *v)    { return v->visit(this); }
int NumberExp::accept(Visitor *v)    { return v->visit(this); }
int FloatExp::accept(Visitor *v)     { return v->visit(this); }
int IdExp::accept(Visitor *v)        { return v->visit(this); }
int Program::accept(Visitor *v)      { return v->visit(this); }
int PrintStm::accept(Visitor *v)     { return v->visit(this); }
int AssignStm::accept(Visitor *v)    { return v->visit(this); }
int ExpListSize::accept(Visitor *v)  { return v->visit(this); }
int ExpListVals::accept(Visitor *v)  { return v->visit(this); }
int ExpMatrixSize::accept(Visitor *v){ return v->visit(this); }
int ExpMatrixVals::accept(Visitor *v){ return v->visit(this); }
int IfStm::accept(Visitor *v)        { return v->visit(this); }
int WhileStm::accept(Visitor *v)     { return v->visit(this); }
int Body::accept(Visitor *v)         { return v->visit(this); }
int VarDec::accept(Visitor *v)       { return v->visit(this); }
int FcallExp::accept(Visitor *v)     { return v->visit(this); }
int FunDec::accept(Visitor *v)       { return v->visit(this); }
int ReturnStm::accept(Visitor *v)    { return v->visit(this); }
int DoWhileStm::accept(Visitor *v)   { return v->visit(this); }
int BreakStm::accept(Visitor *v)     { return v->visit(this); }
int SwitchStm::accept(Visitor *v)    { return v->visit(this); }
int UnaryExp::accept(Visitor *v)     { return v->visit(this); }
int StringExp::accept(Visitor *v)    { return v->visit(this); }
int AddrExp::accept(Visitor *v)      { return v->visit(this); }
int DerefExp::accept(Visitor *v)     { return v->visit(this); }
int LambdaExp::accept(Visitor *v)    { return v->visit(this); }
int IndexExp::accept(Visitor *v)     { return v->visit(this); }
int MatrixExp::accept(Visitor *v)    { return v->visit(this); }
int FieldExp::accept(Visitor *v)     { return v->visit(this); }
int StructDec::accept(Visitor *v)    { return v->visit(this); }

int TypeCheckerVisitor::TypeChecker(Program *program) {
  for (auto sd : program->sdlist)
    sd->accept(this);

  for (auto fd : program->fdlist)
    funAridad[fd->nombre] = int(fd->Pnombres.size());

  for (auto fd : program->fdlist)
    fd->accept(this);

  return 0;
}

int TypeCheckerVisitor::visit(FunDec *fd) {
  funcionActual = fd->nombre;
  locales = 0;
  int parametros = int(fd->Pnombres.size());

  entorno.add_level();
  tiposVar.add_level();

  for (size_t i = 0; i < fd->Pnombres.size(); ++i) {
    entorno.add_var(fd->Pnombres[i], 0);
    tiposVar.add_var(fd->Pnombres[i], fd->Ptipos[i]);
  }

  fd->cuerpo->accept(this);

  tiposVar.remove_level();
  entorno.remove_level();

  funcontador[fd->nombre] = parametros + locales;
  return 0;
}

int TypeCheckerVisitor::visit(Body *body) {
  entorno.add_level();
  tiposVar.add_level();

  for (auto dec : body->declarations)
    dec->accept(this);
  for (auto stm : body->StmList)
    stm->accept(this);

  tiposVar.remove_level();
  entorno.remove_level();
  return 0;
}

std::string TypeCheckerVisitor::inferType(Exp *e) {
  if (dynamic_cast<FloatExp *>(e)) return "float";
  if (dynamic_cast<StringExp *>(e)) return "string";
  if (dynamic_cast<ExpListVals *>(e) || dynamic_cast<ExpListSize *>(e))
    return "list";
  if (dynamic_cast<ExpMatrixVals *>(e) || dynamic_cast<ExpMatrixSize *>(e))
    return "matrix";
  if (auto *id = dynamic_cast<IdExp *>(e)) {
    std::string t;
    if (tiposVar.lookup(id->value, t) && !t.empty()) return t;
  }
  return "int"; // por defecto: entero (aritmética, llamadas, etc.)
}

int TypeCheckerVisitor::visit(VarDec *vd) {
  if (vd->init) {
    vd->init->accept(this);
    if (vd->type.empty())
      vd->type = inferType(vd->init); // inferencia de tipos
  }
  for (auto &nombre : vd->vars) {
    if (entorno.check(nombre)) {
      std::cerr << "[TypeChecker] Advertencia: la variable '" << nombre
                << "' ya fue declarada en este scope"
                << " (en función '" << funcionActual << "').\n";
    }
    entorno.add_var(nombre, 0);
    tiposVar.add_var(nombre, vd->type);
    locales++;
  }
  return 0;
}

int TypeCheckerVisitor::visit(StructDec *sd) {
  if (structFields.count(sd->name))
    throw std::runtime_error("[TypeChecker] Estructura duplicada: '" + sd->name + "'");

  int totalFields = 0;
  std::unordered_map<std::string, bool> names;
  std::unordered_map<std::string, int>  offsets;
  for (auto field : sd->fields) {
    for (auto &name : field->vars) {
      if (names.count(name))
        throw std::runtime_error("[TypeChecker] Campo duplicado '" + name +
                                 "' en estructura '" + sd->name + "'");
      names[name]   = true;
      offsets[name] = totalFields * 8;
      totalFields++;
    }
  }

  structFields[sd->name]       = totalFields;
  structFieldOffsets[sd->name] = offsets;
  return 0;
}

int TypeCheckerVisitor::visit(IdExp *exp) {
  if (!entorno.check(exp->value))
    throw std::runtime_error("[TypeChecker] Variable no declarada: '" +
                             exp->value + "' usada en la función '" +
                             funcionActual + "'");
  return 0;
}

int TypeCheckerVisitor::visit(AssignStm *stm) {
  stm->target->accept(this);
  stm->e->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(ExpListSize *stm) {
  stm->size->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(ExpListVals *stm) {
  if (structFields.count(stm->type)) {
    int esperados = structFields[stm->type];
    int recibidos = int(stm->values.size());
    if (recibidos != 0 && recibidos != esperados)
      throw std::runtime_error("[TypeChecker] La estructura '" + stm->type +
                               "' espera " + std::to_string(esperados) +
                               " valor(es), pero se pasaron " +
                               std::to_string(recibidos));
  }
  for (auto value : stm->values)
    value->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(ExpMatrixSize *stm) {
  stm->rows->accept(this);
  stm->cols->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(ExpMatrixVals *stm) {
  stm->rows->accept(this);
  stm->cols->accept(this);
  for (auto value : stm->values)
    value->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(FcallExp *fcall) {
  if (funAridad.find(fcall->nombre) == funAridad.end()) {
    // Si el nombre es una variable, es una llamada a un puntero a función
    // (lambda almacenada): no se valida la aridad estáticamente.
    if (entorno.check(fcall->nombre)) {
      for (auto arg : fcall->argumentos)
        arg->accept(this);
      return 0;
    }
    throw std::runtime_error("[TypeChecker] Función no definida: '" +
                             fcall->nombre + "' llamada en '" + funcionActual + "'");
  }

  int esperados = funAridad[fcall->nombre];
  int recibidos = int(fcall->argumentos.size());
  if (recibidos != esperados)
    throw std::runtime_error("[TypeChecker] La función '" + fcall->nombre +
                             "' espera " + std::to_string(esperados) +
                             " argumento(s), pero se pasaron " +
                             std::to_string(recibidos));

  for (auto arg : fcall->argumentos)
    arg->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(IfStm *stm) {
  stm->condition->accept(this);

  int base = locales;

  locales = 0;
  stm->then->accept(this);
  int maxLocales = locales;

  if (stm->els) {
    locales = 0;
    stm->els->accept(this);
    maxLocales = std::max(maxLocales, locales);
  }

  locales = base + maxLocales;
  return 0;
}

int TypeCheckerVisitor::visit(WhileStm *stm) {
  stm->condition->accept(this);
  stm->b->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(PrintStm *stm) {
  stm->e->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(ReturnStm *r) {
  r->e->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(BinaryExp *exp) {
  exp->left->accept(this);
  exp->right->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(UnaryExp *exp) {
  exp->operand->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(IndexExp *exp) {
  if (!entorno.check(exp->name))
    throw std::runtime_error("[TypeChecker] Variable no declarada: '" +
                             exp->name + "' usada en la función '" +
                             funcionActual + "'");
  exp->index->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(MatrixExp *exp) {
  if (!entorno.check(exp->name))
    throw std::runtime_error("[TypeChecker] Variable no declarada: '" +
                             exp->name + "' usada en la función '" +
                             funcionActual + "'");
  exp->row->accept(this);
  exp->col->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(FieldExp *exp) {
  std::string type;
  if (!tiposVar.lookup(exp->object, type))
    throw std::runtime_error("[TypeChecker] Variable no declarada: '" +
                             exp->object + "' usada en la función '" +
                             funcionActual + "'");
  if (!structFieldOffsets.count(type))
    throw std::runtime_error("[TypeChecker] La variable '" + exp->object +
                             "' no es una estructura");
  if (!structFieldOffsets[type].count(exp->field))
    throw std::runtime_error("[TypeChecker] La estructura '" + type +
                             "' no tiene campo '" + exp->field + "'");
  return 0;
}

int TypeCheckerVisitor::visit(NumberExp *) { return 0; }
int TypeCheckerVisitor::visit(FloatExp *f) {
  floatLiterals.push_back(f); // recolectar para emitir en .data
  return 0;
}
int TypeCheckerVisitor::visit(StringExp *s) {
  stringLiterals.push_back(s); // recolectar para emitir en .data
  return 0;
}
int TypeCheckerVisitor::visit(AddrExp *exp) {
  if (!entorno.check(exp->name))
    throw std::runtime_error("[TypeChecker] Variable no declarada: '" +
                             exp->name + "' usada con '&' en la función '" +
                             funcionActual + "'");
  return 0;
}
int TypeCheckerVisitor::visit(DerefExp *exp) {
  exp->ptr->accept(this);
  return 0;
}
int TypeCheckerVisitor::visit(LambdaExp *) {
  return 0; // el cuerpo se verifica como función global izada
}
int TypeCheckerVisitor::visit(Program *)     { return 0; }

int TypeCheckerVisitor::visit(DoWhileStm *stm) {
  stm->condition->accept(this);
  stm->b->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(BreakStm *) { return 0; }

int TypeCheckerVisitor::visit(SwitchStm *stm) {
  stm->e->accept(this);
  for (auto c : stm->cases)
    for (auto s : c->body)
      s->accept(this);
  for (auto s : stm->default_body)
    s->accept(this);
  return 0;
}

LVal GenCodeVisitor::captureLVal(Exp *exp) {
  LVal lv;
  lvalTarget = &lv;
  exp->accept(this);
  lvalTarget = nullptr;
  return lv;
}

int GenCodeVisitor::storeTarget(const LVal &lv) {
  switch (lv.kind) {
  case LValKind::Id:     return storeId(lv);
  case LValKind::Index:  return storeIndex(lv);
  case LValKind::Matrix: return storeMatrix(lv);
  case LValKind::Field:  return storeField(lv);
  case LValKind::Deref:  return storeDeref(lv);
  default:
    throw std::runtime_error("Asignacion a una expresion que no es lvalue");
  }
}

int GenCodeVisitor::generar(Program *program) {
  tipos.TypeChecker(program);
  funcontador       = tipos.funcontador;
  structFields      = tipos.structFields;
  structFieldOffsets= tipos.structFieldOffsets;
  for (auto fd : program->fdlist) {
    funParamNames[fd->nombre] = fd->Pnombres;
    funParamTypes[fd->nombre] = fd->Ptipos;
    funReturnType[fd->nombre] = fd->tipo;
  }
  // Asignar una etiqueta .data a cada literal de cadena y float recolectado.
  for (size_t i = 0; i < tipos.stringLiterals.size(); ++i)
    tipos.stringLiterals[i]->strLabel = "str_" + std::to_string(i);
  for (size_t i = 0; i < tipos.floatLiterals.size(); ++i)
    tipos.floatLiterals[i]->fltLabel = "flt_" + std::to_string(i);
  program->accept(this);
  return 0;
}

int GenCodeVisitor::visit(Program *program) {
  out << ".data\n";
  out << "print_fmt: .string \"%ld \\n\"\n";
  out << "print_str_fmt: .string \"%s\\n\"\n";
  out << "print_flt_fmt: .string \"%g\\n\"\n";

  // Literales de cadena recolectados por el TypeChecker.
  for (auto s : tipos.stringLiterals)
    out << s->strLabel << ": .string \"" << s->value << "\"\n";

  // Literales en punto flotante (constantes .double).
  for (auto f : tipos.floatLiterals) {
    std::ostringstream oss;
    oss.precision(17);
    oss << f->value;
    out << f->fltLabel << ": .double " << oss.str() << "\n";
  }

  for (auto dec : program->vdlist)
    dec->accept(this);

  for (std::unordered_map<std::string, bool>::const_iterator it =
           memoriaGlobal.begin();
       it != memoriaGlobal.end(); ++it)
    out << it->first << ": .quad 0\n";
  for (auto fd : program->fdlist) {
    for (size_t i = 0; i < fd->Ptipos.size(); ++i) {
      if (fd->Ptipos[i] == "matrix")
        out << "__cols_" << fd->nombre << "_" << fd->Pnombres[i]
            << ": .quad 0\n";
    }
  }

  out << "\n.text\n";
  for (auto fd : program->fdlist)
    fd->accept(this);

  out << "\n.section .note.GNU-stack,\"\",@progbits\n";
  return 0;
}

int GenCodeVisitor::visit(StructDec *) { return 0; }

int GenCodeVisitor::visit(VarDec *stm) {
  for (auto &var : stm->vars) {
    if (!entornoFuncion) {
      memoriaGlobal[var]  = true;
      variableTypes[var]  = stm->type;
      if (structFields.count(stm->type))
        structAllocated[var] = false;
    } else {
      memoria[var]        = offset;
      variableTypes[var]  = stm->type;
      if (structFields.count(stm->type))
        structAllocated[var] = false;
      offset -= 8;
    }
  }
  // Inicializador con inferencia de tipos ('var x = expr'): se emite como una
  // asignación normal una vez reservado el espacio (solo variables locales).
  if (stm->init && entornoFuncion && !stm->vars.empty()) {
    IdExp target(stm->vars.front());
    AssignStm assign(&target, stm->init);
    assign.accept(this);
  }
  return 0;
}

int GenCodeVisitor::visit(NumberExp *exp) {
  if (lvalTarget) {
    lvalTarget->kind       = LValKind::Number;
    lvalTarget->numIsConst = true;
    lvalTarget->numVal     = exp->value;
    return 0;
  }
  out << "  movq $" << exp->value << ", %rax\n";
  return 0;
}

int GenCodeVisitor::visit(StringExp *exp) {
  // Carga la dirección del literal de cadena (RIP-relative).
  out << "  leaq " << exp->strLabel << "(%rip), %rax\n";
  return 0;
}

// Tipo estático de una expresión a efectos de generación de código: "float" o
// "int". Sirve para decidir si se usan registros SSE (xmm) o enteros (%rax) y
// para aplicar la promoción automática en operaciones mixtas.
std::string GenCodeVisitor::exprType(Exp *e) {
  if (dynamic_cast<FloatExp *>(e)) return "float";
  if (auto *id = dynamic_cast<IdExp *>(e)) {
    auto it = variableTypes.find(id->value);
    if (it != variableTypes.end() && it->second == "float") return "float";
    return "int";
  }
  if (auto *u = dynamic_cast<UnaryExp *>(e)) {
    if (u->op == NEG_OP) return exprType(u->operand);
    return "int";
  }
  if (auto *b = dynamic_cast<BinaryExp *>(e)) {
    bool arith = (b->op == PLUS_OP || b->op == MINUS_OP ||
                  b->op == MUL_OP || b->op == DIV_OP);
    if (arith && (exprType(b->left) == "float" || exprType(b->right) == "float"))
      return "float";
    return "int"; // comparaciones y lógicos producen un entero (bool)
  }
  if (auto *fc = dynamic_cast<FcallExp *>(e)) {
    auto it = funReturnType.find(fc->nombre);
    if (it != funReturnType.end() && it->second == "float") return "float";
  }
  return "int";
}

// Evalúa e dejando un double en %xmm0. Si e es entero, lo promueve (cvtsi2sd).
void GenCodeVisitor::genFloat(Exp *e) {
  if (exprType(e) == "float") {
    e->accept(this); // deja el double en %xmm0
  } else {
    e->accept(this); // entero en %rax
    out << "  cvtsi2sd %rax, %xmm0\n"; // promoción int -> float
  }
}

int GenCodeVisitor::visit(FloatExp *exp) {
  out << "  movsd " << exp->fltLabel << "(%rip), %xmm0\n";
  return 0;
}

int GenCodeVisitor::visit(AddrExp *exp) {
  // Dirección de una variable (leaq). No es lvalue.
  if (memoriaGlobal.count(exp->name))
    out << "  leaq " << exp->name << "(%rip), %rax\n";
  else
    out << "  leaq " << memoria[exp->name] << "(%rbp), %rax\n";
  return 0;
}

int GenCodeVisitor::visit(DerefExp *exp) {
  if (lvalTarget) {
    // *p = ... : el destino es la dirección apuntada por p.
    lvalTarget->kind  = LValKind::Deref;
    lvalTarget->index = exp->ptr;
    return 0;
  }
  exp->ptr->accept(this);          // dirección -> %rax
  out << "  movq (%rax), %rax\n";  // valor apuntado
  return 0;
}

int GenCodeVisitor::visit(LambdaExp *exp) {
  // Valor de la lambda = dirección de la función izada (puntero a función).
  out << "  leaq " << exp->name << "(%rip), %rax\n";
  return 0;
}

int GenCodeVisitor::storeDeref(const LVal &lv) {
  // Al entrar, el valor a almacenar está en %rax.
  out << "  pushq %rax\n";
  lv.index->accept(this);     // puntero -> %rax (dirección destino)
  out << "  movq %rax, %rdi\n";
  out << "  popq %rax\n";     // valor
  out << "  movq %rax, (%rdi)\n";
  return 0;
}

bool GenCodeVisitor::exprIsString(Exp *e) {
  if (dynamic_cast<StringExp *>(e))
    return true;
  if (auto *id = dynamic_cast<IdExp *>(e)) {
    auto it = variableTypes.find(id->value);
    return it != variableTypes.end() && it->second == "string";
  }
  return false;
}

int GenCodeVisitor::visit(IdExp *exp) {
  if (lvalTarget) {
    lvalTarget->kind = LValKind::Id;
    lvalTarget->name = exp->value;
    return 0;
  }
  bool isFloat = variableTypes.count(exp->value) &&
                 variableTypes[exp->value] == "float";
  if (memoriaGlobal.count(exp->value)) {
    if (isFloat)
      out << "  movsd " << exp->value << "(%rip), %xmm0\n";
    else
      out << "  movq " << exp->value << "(%rip), %rax\n";
  } else {
    if (isFloat)
      out << "  movsd " << memoria[exp->value] << "(%rbp), %xmm0\n";
    else
      out << "  movq " << memoria[exp->value] << "(%rbp), %rax\n";
  }
  return 0;
}

int GenCodeVisitor::visit(IndexExp *exp) {
  if (lvalTarget) {
    lvalTarget->kind  = LValKind::Index;
    lvalTarget->name  = exp->name;
    lvalTarget->index = exp->index;
    return 0;
  }
  exp->index->accept(this);
  out << "  movq %rax, %rdi\n";
  if (memoriaGlobal.count(exp->name))
    out << "  movq " << exp->name << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[exp->name] << "(%rbp), %rax\n";
  out << "  movq (%rax, %rdi, 8), %rax\n";
  return 0;
}

int GenCodeVisitor::visit(MatrixExp *exp) {
  if (lvalTarget) {
    lvalTarget->kind = LValKind::Matrix;
    lvalTarget->name = exp->name;
    lvalTarget->row  = exp->row;
    lvalTarget->col  = exp->col;
    return 0;
  }
  exp->row->accept(this);
  out << "  pushq %rax\n";
  exp->col->accept(this);
  out << "  movq %rax, %rdi\n";
  out << "  popq %rax\n";

  if (matrixColumns.count(exp->name))
    out << "  movq $" << matrixColumns[exp->name] << ", %rcx\n";
  else if (currentMatrixParamLabels.count(exp->name))
    out << "  movq " << currentMatrixParamLabels[exp->name] << "(%rip), %rcx\n";
  else
    out << "  movq $0, %rcx\n";

  out << "  imulq %rcx, %rax\n";
  out << "  addq %rdi, %rax\n";
  out << "  movq %rax, %rdi\n";

  if (memoriaGlobal.count(exp->name))
    out << "  movq " << exp->name << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[exp->name] << "(%rbp), %rax\n";

  out << "  movq (%rax, %rdi, 8), %rax\n";
  return 0;
}

int GenCodeVisitor::visit(FieldExp *exp) {
  if (lvalTarget) {
    lvalTarget->kind   = LValKind::Field;
    lvalTarget->object = exp->object;
    lvalTarget->field  = exp->field;
    return 0;
  }
  std::string type     = variableTypes[exp->object];
  int fieldIndex       = structFieldOffsets[type][exp->field] / 8;

  out << "  movq $" << fieldIndex << ", %rdi\n";
  if (memoriaGlobal.count(exp->object))
    out << "  movq " << exp->object << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[exp->object] << "(%rbp), %rax\n";
  out << "  movq (%rax, %rdi, 8), %rax\n";
  return 0;
}

int GenCodeVisitor::visit(ExpListSize *stm) {
  if (lvalTarget) {
    lvalTarget->kind  = LValKind::ListSize;
    lvalTarget->type  = stm->type;
    lvalTarget->index = stm->size;
    return 0;
  }
  stm->size->accept(this);
  out << "  movq $8, %rcx\n";
  out << "  imulq %rcx, %rax\n";
  out << "  movq %rax, %rdi\n"
      << "  call malloc@PLT\n";
  return 0;
}

int GenCodeVisitor::visit(ExpListVals *stm) {
  if (lvalTarget) {
    lvalTarget->kind   = LValKind::ListVals;
    lvalTarget->type   = stm->type;
    lvalTarget->values = stm->values;
    return 0;
  }
  int n = stm->values.size();
  out << "  movq $" << n << ", %rax\n";
  out << "  movq $8, %rcx\n";
  out << "  imulq %rcx, %rax\n";
  out << "  movq %rax, %rdi\n";
  out << "  call malloc@PLT\n";
  out << "  pushq %rax\n";
  for (size_t i = 0; i < (size_t)n; ++i) {
    stm->values[i]->accept(this);
    out << "  movq %rax, %rcx\n";
    out << "  movq $" << i << ", %rdi\n";
    out << "  popq %rax\n";
    out << "  movq %rcx, (%rax, %rdi, 8)\n";
    if (i + 1 < (size_t)n)
      out << "  pushq %rax\n";
  }
  return 0;
}

int GenCodeVisitor::visit(ExpMatrixSize *stm) {
  if (lvalTarget) {
    lvalTarget->kind = LValKind::MatrixSize;
    lvalTarget->type = stm->type;
    lvalTarget->row  = stm->rows;
    lvalTarget->col  = stm->cols;
    return 0;
  }
  stm->cols->accept(this);
  out << "  pushq %rax\n";
  stm->rows->accept(this);
  out << "  popq %rcx\n";
  out << "  imulq %rcx, %rax\n";
  out << "  salq $3, %rax\n";
  out << "  movq %rax, %rdi\n";
  out << "  call malloc@PLT\n";
  return 0;
}

int GenCodeVisitor::visit(ExpMatrixVals *stm) {
  if (lvalTarget) {
    lvalTarget->kind   = LValKind::MatrixVals;
    lvalTarget->type   = stm->type;
    lvalTarget->row    = stm->rows;
    lvalTarget->col    = stm->cols;
    lvalTarget->values = stm->values;
    return 0;
  }
  stm->cols->accept(this);
  out << "  pushq %rax\n";
  stm->rows->accept(this);
  out << "  popq %rcx\n";
  out << "  imulq %rcx, %rax\n";
  out << "  salq $3, %rax\n";
  out << "  movq %rax, %rdi\n";
  out << "  call malloc@PLT\n";
  out << "  pushq %rax\n";
  for (size_t i = 0; i < stm->values.size(); ++i) {
    out << "  pushq %rax\n";
    stm->values[i]->accept(this);
    out << "  movq %rax, %rcx\n";
    out << "  popq %rax\n";
    out << "  movq %rcx, " << (i * 8) << "(%rax)\n";
  }
  out << "  popq %rax\n";
  return 0;
}

int GenCodeVisitor::visit(AssignStm *stm) {
  LVal target = captureLVal(stm->target);

  // Asignación con tipos float: conversión y promoción automáticas.
  if (target.kind == LValKind::Id) {
    std::string targetType =
        variableTypes.count(target.name) ? variableTypes[target.name] : "int";
    bool rhsFloat = (exprType(stm->e) == "float");
    if (targetType == "float") {
      genFloat(stm->e); // promueve int -> float si hace falta
      if (memoriaGlobal.count(target.name))
        out << "  movsd %xmm0, " << target.name << "(%rip)\n";
      else
        out << "  movsd %xmm0, " << memoria[target.name] << "(%rbp)\n";
      return 0;
    }
    if (rhsFloat) { // destino entero, valor float -> truncar
      genFloat(stm->e);
      out << "  cvttsd2si %xmm0, %rax\n";
      if (memoriaGlobal.count(target.name))
        out << "  movq %rax, " << target.name << "(%rip)\n";
      else
        out << "  movq %rax, " << memoria[target.name] << "(%rbp)\n";
      return 0;
    }
  }

  bool specialRhs = stm->e->isSpecialRhs();

  if (target.kind == LValKind::Id && specialRhs) {
    const std::string &targetName = target.name;
    LVal rhs = captureLVal(stm->e);

    if (rhs.kind == LValKind::ListVals) {
      if (structFields.count(rhs.type)) {
        int fields = structFields[rhs.type];
        out << "  movq $" << (fields * 8) << ", %rdi\n";
        out << "  call malloc@PLT\n";
        if (memoriaGlobal.count(targetName))
          out << "  movq %rax, " << targetName << "(%rip)\n";
        else
          out << "  movq %rax, " << memoria[targetName] << "(%rbp)\n";
        structAllocated[targetName] = true;

        for (size_t i = 0; i < rhs.values.size(); ++i) {
          rhs.values[i]->accept(this);
          out << "  pushq %rax\n";
          out << "  movq $" << i << ", %rax\n";
          out << "  movq %rax, %rdi\n";
          out << "  popq %rax\n";
          out << "  movq %rax, %rcx\n";
          if (memoriaGlobal.count(targetName))
            out << "  movq " << targetName << "(%rip), %rax\n";
          else
            out << "  movq " << memoria[targetName] << "(%rbp), %rax\n";
          out << "  movq %rcx, (%rax, %rdi, 8)\n";
        }
        return 0;
      }

      int n = rhs.values.size();
      out << "  movq $" << n << ", %rax\n";
      out << "  movq $8, %rcx\n";
      out << "  imulq %rcx, %rax\n";
      out << "  movq %rax, %rdi\n";
      out << "  call malloc@PLT\n";
      if (memoriaGlobal.count(targetName))
        out << "  movq %rax, " << targetName << "(%rip)\n";
      else
        out << "  movq %rax, " << memoria[targetName] << "(%rbp)\n";

      for (size_t i = 0; i < rhs.values.size(); ++i) {
        rhs.values[i]->accept(this);
        out << "  pushq %rax\n";
        out << "  movq $" << i << ", %rax\n";
        out << "  movq %rax, %rdi\n";
        out << "  popq %rax\n";
        out << "  movq %rax, %rcx\n";
        if (memoriaGlobal.count(targetName))
          out << "  movq " << targetName << "(%rip), %rax\n";
        else
          out << "  movq " << memoria[targetName] << "(%rbp), %rax\n";
        out << "  movq %rcx, (%rax, %rdi, 8)\n";
      }
      return 0;
    }

    if (rhs.kind == LValKind::MatrixSize) {
      LVal colLV = captureLVal(rhs.col);
      if (colLV.numIsConst)
        matrixColumns[targetName] = colLV.numVal;

      rhs.col->accept(this);
      out << "  pushq %rax\n";
      rhs.row->accept(this);
      out << "  popq %rcx\n";
      out << "  imulq %rcx, %rax\n";
      out << "  salq $3, %rax\n";
      out << "  movq %rax, %rdi\n";
      out << "  call malloc@PLT\n";
      if (memoriaGlobal.count(targetName))
        out << "  movq %rax, " << targetName << "(%rip)\n";
      else
        out << "  movq %rax, " << memoria[targetName] << "(%rbp)\n";
      return 0;
    }

    if (rhs.kind == LValKind::MatrixVals) {
      LVal colLV = captureLVal(rhs.col);
      int cols = colLV.numIsConst ? colLV.numVal : 0;
      if (cols > 0)
        matrixColumns[targetName] = cols;

      rhs.col->accept(this);
      out << "  pushq %rax\n";
      rhs.row->accept(this);
      out << "  popq %rcx\n";
      out << "  imulq %rcx, %rax\n";
      out << "  salq $3, %rax\n";
      out << "  movq %rax, %rdi\n";
      out << "  call malloc@PLT\n";
      if (memoriaGlobal.count(targetName))
        out << "  movq %rax, " << targetName << "(%rip)\n";
      else
        out << "  movq %rax, " << memoria[targetName] << "(%rbp)\n";

      for (size_t i = 0; i < rhs.values.size(); ++i) {
        rhs.values[i]->accept(this);
        out << "  pushq %rax\n";
        if (cols > 0) {
          out << "  movq $" << (i / cols) << ", %rax\n";
          out << "  pushq %rax\n";
          out << "  movq $" << (i % cols) << ", %rax\n";
        } else {
          out << "  movq $0, %rax\n";
          out << "  pushq %rax\n";
          out << "  movq $" << i << ", %rax\n";
        }
        out << "  movq %rax, %rdi\n";
        out << "  popq %rax\n";
        if (matrixColumns.count(targetName))
          out << "  movq $" << matrixColumns[targetName] << ", %rcx\n";
        else if (currentMatrixParamLabels.count(targetName))
          out << "  movq " << currentMatrixParamLabels[targetName] << "(%rip), %rcx\n";
        else
          out << "  movq $0, %rcx\n";
        out << "  imulq %rcx, %rax\n";
        out << "  addq %rdi, %rax\n";
        out << "  movq %rax, %rdi\n";
        out << "  popq %rcx\n";
        if (memoriaGlobal.count(targetName))
          out << "  movq " << targetName << "(%rip), %rax\n";
        else
          out << "  movq " << memoria[targetName] << "(%rbp), %rax\n";
        out << "  movq %rcx, (%rax, %rdi, 8)\n";
      }
      return 0;
    }
  }

  stm->e->accept(this);
  storeTarget(target);
  return 0;
}

int GenCodeVisitor::storeId(const LVal &lv) {
  if (memoriaGlobal.count(lv.name))
    out << "  movq %rax, " << lv.name << "(%rip)\n";
  else
    out << "  movq %rax, " << memoria[lv.name] << "(%rbp)\n";
  return 0;
}

int GenCodeVisitor::storeIndex(const LVal &lv) {
  out << "  pushq %rax\n";
  lv.index->accept(this);
  out << "  movq %rax, %rdi\n";
  out << "  popq %rax\n";
  out << "  movq %rax, %rcx\n";
  if (memoriaGlobal.count(lv.name))
    out << "  movq " << lv.name << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[lv.name] << "(%rbp), %rax\n";
  out << "  movq %rcx, (%rax, %rdi, 8)\n";
  return 0;
}

int GenCodeVisitor::storeMatrix(const LVal &lv) {
  out << "  pushq %rax\n";
  lv.row->accept(this);
  out << "  pushq %rax\n";
  lv.col->accept(this);
  out << "  movq %rax, %rdi\n";
  out << "  popq %rax\n";

  if (matrixColumns.count(lv.name))
    out << "  movq $" << matrixColumns[lv.name] << ", %rcx\n";
  else if (currentMatrixParamLabels.count(lv.name))
    out << "  movq " << currentMatrixParamLabels[lv.name] << "(%rip), %rcx\n";
  else
    out << "  movq $0, %rcx\n";

  out << "  imulq %rcx, %rax\n";
  out << "  addq %rdi, %rax\n";
  out << "  movq %rax, %rdi\n";
  out << "  popq %rcx\n";

  if (memoriaGlobal.count(lv.name))
    out << "  movq " << lv.name << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[lv.name] << "(%rbp), %rax\n";
  out << "  movq %rcx, (%rax, %rdi, 8)\n";
  return 0;
}

int GenCodeVisitor::storeField(const LVal &lv) {
  std::string type = variableTypes[lv.object];
  int fieldIndex   = structFieldOffsets[type][lv.field] / 8;

  out << "  pushq %rax\n";
  out << "  movq $" << fieldIndex << ", %rax\n";
  out << "  movq %rax, %rdi\n";
  out << "  popq %rax\n";
  out << "  movq %rax, %rcx\n";

  if (structAllocated.count(lv.object) && !structAllocated[lv.object]) {
    int fields = structFields[type];
    out << "  pushq %rcx\n";
    out << "  pushq %rdi\n";
    out << "  movq $" << (fields * 8) << ", %rdi\n";
    out << "  call malloc@PLT\n";
    if (memoriaGlobal.count(lv.object))
      out << "  movq %rax, " << lv.object << "(%rip)\n";
    else
      out << "  movq %rax, " << memoria[lv.object] << "(%rbp)\n";
    out << "  popq %rdi\n";
    out << "  popq %rcx\n";
    structAllocated[lv.object] = true;
  }

  if (memoriaGlobal.count(lv.object))
    out << "  movq " << lv.object << "(%rip), %rax\n";
  else
    out << "  movq " << memoria[lv.object] << "(%rbp), %rax\n";
  out << "  movq %rcx, (%rax, %rdi, 8)\n";
  return 0;
}

// Un operando "hoja" puede usarse directamente como inmediato (número) o como
// operando de memoria (variable) dentro de la instrucción aritmética, sin
// gastar un registro temporal ni un push/pop. Devuelve true y rellena
// `operand` con la sintaxis AT&T correspondiente.
bool GenCodeVisitor::leafOperand(Exp *e, std::string &operand) {
  if (auto *n = dynamic_cast<NumberExp *>(e)) {
    operand = "$" + std::to_string(n->value);
    return true;
  }
  if (auto *id = dynamic_cast<IdExp *>(e)) {
    if (memoriaGlobal.count(id->value))
      operand = id->value + "(%rip)";
    else
      operand = std::to_string(memoria[id->value]) + "(%rbp)";
    return true;
  }
  return false;
}

// Emite  %rax = %rax  <op>  src , donde src es un registro (%rcx), un
// inmediato ($n) o un operando de memoria. Para DIV y POW el divisor/exponente
// siempre llega ya materializado en %rcx (ver visit(BinaryExp)).
void GenCodeVisitor::emitBinOp(BinaryOp op, const std::string &src) {
  switch (op) {
  case PLUS_OP:  out << "  addq "  << src << ", %rax\n"; break;
  case MINUS_OP: out << "  subq "  << src << ", %rax\n"; break;
  case MUL_OP:   out << "  imulq " << src << ", %rax\n"; break;
  case DIV_OP:
    out << "  cqto\n";
    out << "  idivq " << src << "\n";
    break;
  case POW_OP: {
    int lbl = labelcont++;
    out << "  movq $1, %rdx\n";
    out << "pow_loop_" << lbl << ":\n";
    out << "  cmpq $0, %rcx\n";
    out << "  jle pow_end_" << lbl << "\n";
    out << "  imulq %rax, %rdx\n";
    out << "  subq $1, %rcx\n";
    out << "  jmp pow_loop_" << lbl << "\n";
    out << "pow_end_" << lbl << ":\n";
    out << "  movq %rdx, %rax\n";
    break;
  }
  case LE_OP:
    out << "  cmpq " << src << ", %rax\n  movq $0, %rax\n  setl %al\n  movzbq %al, %rax\n";
    break;
  case GT_OP:
    out << "  cmpq " << src << ", %rax\n  movq $0, %rax\n  setg %al\n  movzbq %al, %rax\n";
    break;
  case LEQ_OP:
    out << "  cmpq " << src << ", %rax\n  movq $0, %rax\n  setle %al\n  movzbq %al, %rax\n";
    break;
  case GEQ_OP:
    out << "  cmpq " << src << ", %rax\n  movq $0, %rax\n  setge %al\n  movzbq %al, %rax\n";
    break;
  case EQ_OP:
    out << "  cmpq " << src << ", %rax\n  movq $0, %rax\n  sete %al\n  movzbq %al, %rax\n";
    break;
  case NE_OP:
    out << "  cmpq " << src << ", %rax\n  movq $0, %rax\n  setne %al\n  movzbq %al, %rax\n";
    break;
  case AND_OP: out << "  andq " << src << ", %rax\n"; break;
  case OR_OP:  out << "  orq "  << src << ", %rax\n"; break;
  }
}

int GenCodeVisitor::visit(BinaryExp *exp) {
  // 1) Plegado de constantes: la subexpresión ya fue evaluada en Opt1.
  if (exp->isConstant) {
    out << "  movq $" << exp->constantValue << ", %rax\n";
    return 0;
  }

  // 1b) Aritmética / comparación en punto flotante (SSE). Si algún operando es
  //     float, ambos se cargan en %xmm0/%xmm1 (promoviendo enteros) y se opera
  //     con instrucciones SSE. Resultado: aritmética -> %xmm0; comparación -> %rax.
  bool anyFloat = (exprType(exp->left) == "float" || exprType(exp->right) == "float");
  bool arith = (exp->op == PLUS_OP || exp->op == MINUS_OP ||
                exp->op == MUL_OP || exp->op == DIV_OP);
  bool comp = (exp->op == LE_OP || exp->op == GT_OP || exp->op == LEQ_OP ||
               exp->op == GEQ_OP || exp->op == EQ_OP || exp->op == NE_OP);
  if (anyFloat && (arith || comp)) {
    genFloat(exp->left);
    out << "  subq $8, %rsp\n  movsd %xmm0, (%rsp)\n"; // guardar izquierda
    genFloat(exp->right);
    out << "  movsd %xmm0, %xmm1\n";                   // derecha -> %xmm1
    out << "  movsd (%rsp), %xmm0\n  addq $8, %rsp\n"; // izquierda -> %xmm0
    if (arith) {
      switch (exp->op) {
      case PLUS_OP:  out << "  addsd %xmm1, %xmm0\n"; break;
      case MINUS_OP: out << "  subsd %xmm1, %xmm0\n"; break;
      case MUL_OP:   out << "  mulsd %xmm1, %xmm0\n"; break;
      case DIV_OP:   out << "  divsd %xmm1, %xmm0\n"; break;
      default: break;
      }
    } else {
      out << "  ucomisd %xmm1, %xmm0\n";
      out << "  movq $0, %rax\n";
      const char *set = "sete";
      switch (exp->op) {
      case LE_OP:  set = "setb";  break;
      case GT_OP:  set = "seta";  break;
      case LEQ_OP: set = "setbe"; break;
      case GEQ_OP: set = "setae"; break;
      case EQ_OP:  set = "sete";  break;
      case NE_OP:  set = "setne"; break;
      default: break;
      }
      out << "  " << set << " %al\n  movzbq %al, %rax\n";
    }
    return 0;
  }

  // 2) Peephole (caso r=0 de Sethi-Ullman): si el operando derecho no necesita
  //    registro (es un número o una variable) lo plegamos como operando
  //    inmediato/memoria de la propia instrucción y evitamos el push/pop.
  //    Se excluyen DIV y POW, cuyo divisor/exponente debe estar en %rcx.
  std::string rop;
  if (exp->op != DIV_OP && exp->op != POW_OP && leafOperand(exp->right, rop)) {
    exp->left->accept(this);   // izquierda -> %rax
    emitBinOp(exp->op, rop);   // %rax = izquierda <op> derecha
    return 0;
  }

  // 3) Caso general (Sethi-Ullman): se evalúa primero el subárbol que requiere
  //    más registros para minimizar el uso de la pila. En ambas ramas el
  //    resultado queda con la izquierda en %rax y la derecha en %rcx, de modo
  //    que `op %rcx, %rax` calcula siempre  izquierda <op> derecha.
  if (exp->left->label >= exp->right->label) {
    exp->left->accept(this);          // izquierda -> %rax
    out << "  pushq %rax\n";
    exp->right->accept(this);         // derecha -> %rax
    out << "  movq %rax, %rcx\n";     // derecha -> %rcx
    out << "  popq %rax\n";           // izquierda -> %rax
  } else {
    exp->right->accept(this);         // derecha -> %rax
    out << "  pushq %rax\n";
    exp->left->accept(this);          // izquierda -> %rax
    out << "  popq %rcx\n";           // derecha -> %rcx
  }
  emitBinOp(exp->op, "%rcx");
  return 0;
}

int GenCodeVisitor::visit(UnaryExp *exp) {
  exp->operand->accept(this);
  if (exp->op == NEG_OP) {
    out << "  negq %rax\n"; // negación aritmética
    return 0;
  }
  int lbl = labelcont++;
  out << "  cmpq $0, %rax\n";
  out << "  je not_true_" << lbl << "\n";
  out << "  movq $0, %rax\n";
  out << "  jmp not_end_" << lbl << "\n";
  out << "not_true_" << lbl << ":\n";
  out << "  movq $1, %rax\n";
  out << "not_end_" << lbl << ":\n";
  return 0;
}

int GenCodeVisitor::visit(PrintStm *stm) {
  if (exprType(stm->e) == "float") {
    genFloat(stm->e); // double -> %xmm0
    out << "  leaq print_flt_fmt(%rip), %rdi\n";
    out << "  movb $1, %al\n"; // 1 registro vectorial usado (printf variádica)
    out << "  call printf@PLT\n";
    return 0;
  }
  bool isStr = exprIsString(stm->e);
  stm->e->accept(this);
  out << "  movq %rax, %rsi\n";
  if (isStr)
    out << "  leaq print_str_fmt(%rip), %rdi\n";
  else
    out << "  leaq print_fmt(%rip), %rdi\n";
  out << "  movq $0, %rax\n";
  out << "  call printf@PLT\n";
  return 0;
}

int GenCodeVisitor::visit(Body *b) {
  for (auto dec : b->declarations)
    dec->accept(this);
  for (auto stm : b->StmList)
    stm->accept(this);
  return 0;
}

int GenCodeVisitor::visit(IfStm *stm) {
  int lbl = labelcont++;
  stm->condition->accept(this);
  out << "  cmpq $0, %rax\n";
  out << "  je else_" << lbl << "\n";
  stm->then->accept(this);
  out << "  jmp endif_" << lbl << "\n";
  out << "else_" << lbl << ":\n";
  if (stm->els)
    stm->els->accept(this);
  out << "endif_" << lbl << ":\n";
  return 0;
}

int GenCodeVisitor::visit(WhileStm *stm) {
  int lbl = labelcont++;
  std::string oldBreak = currentBreakLabel;
  currentBreakLabel = "endwhile_" + std::to_string(lbl);

  out << "while_" << lbl << ":\n";
  stm->condition->accept(this);
  out << "  cmpq $0, %rax\n";
  out << "  je endwhile_" << lbl << "\n";
  stm->b->accept(this);
  out << "  jmp while_" << lbl << "\n";
  out << "endwhile_" << lbl << ":\n";

  currentBreakLabel = oldBreak;
  return 0;
}

int GenCodeVisitor::visit(DoWhileStm *stm) {
  int lbl = labelcont++;
  std::string oldBreak = currentBreakLabel;
  currentBreakLabel = "endwhile_" + std::to_string(lbl);

  out << "dowhile_" << lbl << ":\n";
  stm->b->accept(this);
  stm->condition->accept(this);
  out << "  cmpq $0, %rax\n";
  out << "  jne dowhile_" << lbl << "\n";
  out << "endwhile_" << lbl << ":\n";

  currentBreakLabel = oldBreak;
  return 0;
}

int GenCodeVisitor::visit(ReturnStm *stm) {
  if (funReturnType.count(nombreFuncion) && funReturnType[nombreFuncion] == "float")
    genFloat(stm->e); // valor de retorno en %xmm0
  else
    stm->e->accept(this); // valor de retorno en %rax
  out << "  jmp .end_" << nombreFuncion << "\n";
  return 0;
}

int GenCodeVisitor::visit(BreakStm *) {
  if (currentBreakLabel.empty()) {
    std::cerr << "Error: break fuera de while, do-while o switch\n";
    exit(1);
  }
  out << "  jmp " << currentBreakLabel << "\n";
  return 0;
}

int GenCodeVisitor::visit(SwitchStm *stm) {
  int lbl = labelcont++;
  stm->e->accept(this);
  out << "  movq %rax, %r10\n";

  for (auto c : stm->cases) {
    out << "  movq $" << c->value << ", %rax\n";
    out << "  cmpq %rax, %r10\n";
    out << "  je case_" << lbl << "_" << c->value << "\n";
  }

  if (!stm->default_body.empty())
    out << "  jmp default_" << lbl << "\n";
  else
    out << "  jmp endswitch_" << lbl << "\n";

  std::string oldBreak = currentBreakLabel;
  currentBreakLabel = "endswitch_" + std::to_string(lbl);

  for (auto c : stm->cases) {
    out << "case_" << lbl << "_" << c->value << ":\n";
    for (auto s : c->body)
      s->accept(this);
    out << "  jmp endswitch_" << lbl << "\n";
  }

  if (!stm->default_body.empty()) {
    out << "default_" << lbl << ":\n";
    for (auto s : stm->default_body)
      s->accept(this);
  }

  currentBreakLabel = oldBreak;
  out << "endswitch_" << lbl << ":\n";
  return 0;
}

int GenCodeVisitor::visit(FunDec *f) {
  entornoFuncion = true;
  memoria.clear();
  variableTypes.clear();
  structAllocated.clear();
  matrixColumns.clear();
  currentMatrixParamLabels.clear();
  offset = -8;
  nombreFuncion = f->nombre;

  const std::vector<std::string> argRegs = {"%rdi", "%rsi", "%rdx",
                                             "%rcx", "%r8",  "%r9"};

  out << "\n.globl " << f->nombre << "\n";
  out << f->nombre << ":\n";
  out << "  pushq %rbp\n";
  out << "  movq %rsp, %rbp\n";
  // El marco se alinea a 16 bytes: la ABI x86-64 exige %rsp alineado a 16
  // antes de cada 'call' (printf/malloc usan SSE alineado internamente).
  int frame = funcontador[f->nombre] * 8;
  frame = (frame + 15) & ~15;
  out << "  subq $" << frame << ", %rsp\n";

  int nParams = int(f->Pnombres.size());
  for (int i = 0; i < nParams; i++) {
    memoria[f->Pnombres[i]]       = offset;
    variableTypes[f->Pnombres[i]] = f->Ptipos[i];
    if (f->Ptipos[i] == "matrix")
      currentMatrixParamLabels[f->Pnombres[i]] =
          "__cols_" + f->nombre + "_" + f->Pnombres[i];
    out << "  movq " << argRegs[i] << ", " << offset << "(%rbp)\n";
    offset -= 8;
  }

  for (auto dec : f->cuerpo->declarations)
    dec->accept(this);

  for (auto stm : f->cuerpo->StmList)
    stm->accept(this);

  out << ".end_" << f->nombre << ":\n";
  out << "  leave\n";
  out << "  ret\n";

  entornoFuncion = false;
  return 0;
}

int GenCodeVisitor::visit(FcallExp *exp) {
  const std::vector<std::string> argRegs = {"%rdi", "%rsi", "%rdx",
                                             "%rcx", "%r8",  "%r9"};
  int nArgs = int(exp->argumentos.size());
  for (int i = 0; i < nArgs; i++) {
    exp->argumentos[i]->accept(this);
    // Convención uniforme: todo valor de 64 bits se pasa por registro entero.
    // Un float se mueve de %xmm0 al GPR conservando su patrón de bits.
    if (exprType(exp->argumentos[i]) == "float")
      out << "  movq %xmm0, " << argRegs[i] << "\n";
    else
      out << "  movq %rax, " << argRegs[i] << "\n";

    if (funParamTypes.count(exp->nombre) &&
        i < int(funParamTypes[exp->nombre].size()) &&
        funParamTypes[exp->nombre][i] == "matrix") {
      std::string targetLabel =
          "__cols_" + exp->nombre + "_" + funParamNames[exp->nombre][i];
      LVal argLV = captureLVal(exp->argumentos[i]);
      if (argLV.kind == LValKind::Id) {
        const std::string &argName = argLV.name;
        if (matrixColumns.count(argName)) {
          out << "  movq $" << matrixColumns[argName] << ", %r10\n";
          out << "  movq %r10, " << targetLabel << "(%rip)\n";
        } else if (currentMatrixParamLabels.count(argName)) {
          out << "  movq " << currentMatrixParamLabels[argName]
              << "(%rip), %r10\n";
          out << "  movq %r10, " << targetLabel << "(%rip)\n";
        }
      }
    }
  }
  // Llamada directa si es una función declarada; indirecta (call *reg) si el
  // nombre es una variable que contiene un puntero a función (lambda).
  if (funReturnType.count(exp->nombre)) {
    out << "  call " << exp->nombre << "\n";
  } else {
    if (memoriaGlobal.count(exp->nombre))
      out << "  movq " << exp->nombre << "(%rip), %r11\n";
    else
      out << "  movq " << memoria[exp->nombre] << "(%rbp), %r11\n";
    out << "  call *%r11\n";
  }
  return 0;
}

int Opt1Visitor::Opt1(Program *program) {
  program->accept(this);
  return 0;
}

int Opt1Visitor::visit(BinaryExp *exp) {
  exp->left->accept(this);
  exp->right->accept(this);

  // Solo se pliega si ambos operandos son constantes.
  if (!(exp->left->isConstant && exp->right->isConstant)) {
    exp->isConstant = false;
    return 0;
  }

  long l = exp->left->constantValue;
  long r = exp->right->constantValue;
  long v = 0;
  bool foldable = true;

  switch (exp->op) {
  case PLUS_OP:  v = l + r; break;
  case MINUS_OP: v = l - r; break;
  case MUL_OP:   v = l * r; break;
  case DIV_OP:
    if (r == 0) foldable = false; // división por cero: se deja para runtime
    else v = l / r;
    break;
  case POW_OP: {
    v = 1;
    for (long i = 0; i < r; ++i) v *= l;
    break;
  }
  case LE_OP:  v = (l <  r); break;
  case GT_OP:  v = (l >  r); break;
  case LEQ_OP: v = (l <= r); break;
  case GEQ_OP: v = (l >= r); break;
  case EQ_OP:  v = (l == r); break;
  case NE_OP:  v = (l != r); break;
  case AND_OP: v = (l && r); break;
  case OR_OP:  v = (l || r); break;
  default:     foldable = false; break;
  }

  exp->isConstant = foldable;
  if (foldable)
    exp->constantValue = static_cast<int>(v);
  return 0;
}

int Opt1Visitor::visit(NumberExp *exp) {
  exp->isConstant = true;
  exp->constantValue = exp->value;
  return 0;
}

int Opt1Visitor::visit(StringExp *) { return 0; }
int Opt1Visitor::visit(FloatExp *) { return 0; }
int Opt1Visitor::visit(AddrExp *) { return 0; }
int Opt1Visitor::visit(DerefExp *exp) { exp->ptr->accept(this); return 0; }
int Opt1Visitor::visit(LambdaExp *) { return 0; }
int Opt1Visitor::visit(IdExp *) { return 0; }

int Opt1Visitor::visit(UnaryExp *exp) {
  exp->operand->accept(this);
  if (exp->op == NEG_OP && exp->operand->isConstant) {
    exp->isConstant = true;
    exp->constantValue = -exp->operand->constantValue;
  }
  return 0;
}
int Opt1Visitor::visit(IndexExp *) { return 0; }
int Opt1Visitor::visit(MatrixExp *) { return 0; }
int Opt1Visitor::visit(FieldExp *) { return 0; }

int Opt1Visitor::visit(Program *p) {
  for (auto i : p->sdlist) i->accept(this);
  for (auto i : p->vdlist) i->accept(this);
  for (auto i : p->fdlist) i->accept(this);
  return 0;
}

int Opt1Visitor::visit(StructDec *sd) {
  for (auto i : sd->fields) i->accept(this);
  return 0;
}

int Opt1Visitor::visit(PrintStm *stm) {
  stm->e->accept(this);
  return 0;
}

int Opt1Visitor::visit(AssignStm *stm) {
  stm->e->accept(this);
  return 0;
}

int Opt1Visitor::visit(ExpListSize *) { return 0; }
int Opt1Visitor::visit(ExpListVals *) { return 0; }
int Opt1Visitor::visit(ExpMatrixSize *) { return 0; }
int Opt1Visitor::visit(ExpMatrixVals *) { return 0; }
int Opt1Visitor::visit(WhileStm *) { return 0; }
int Opt1Visitor::visit(DoWhileStm *) { return 0; }
int Opt1Visitor::visit(IfStm *) { return 0; }
int Opt1Visitor::visit(BreakStm *) { return 0; }

int Opt1Visitor::visit(SwitchStm *stm) {
  stm->e->accept(this);
  for (auto j : stm->default_body) j->accept(this);
  for (auto i : stm->cases) {
    for (auto j : i->body) {
      j->accept(this);
    }
  }
  return 0;
}

int Opt1Visitor::visit(Body *body) {
  for (auto i : body->declarations) i->accept(this);
  for (auto i : body->StmList) i->accept(this);
  return 0;
}

int Opt1Visitor::visit(VarDec *vd) {
  if (vd->init) vd->init->accept(this);
  return 0;
}
int Opt1Visitor::visit(FcallExp *) { return 0; }
int Opt1Visitor::visit(ReturnStm *) { return 0; }

int Opt1Visitor::visit(FunDec *fd) {
  fd->cuerpo->accept(this);
  return 0;
}

// ---------------------------------------------------------------------------
// Opt2: etiquetado de Sethi-Ullman (Sem12).
// Hoja -> label 1 (necesita un registro). Nodo binario -> max(l,r) si las
// etiquetas difieren, o l+1 si son iguales (al empatar hace falta un registro
// extra para no perder un resultado intermedio). El resto de nodos solo se
// recorren para alcanzar las expresiones anidadas.
// ---------------------------------------------------------------------------
int Opt2Visitor::Opt2(Program *program) {
  program->accept(this);
  return 0;
}

int Opt2Visitor::visit(BinaryExp *exp) {
  exp->left->accept(this);
  exp->right->accept(this);
  int l = exp->left->label;
  int r = exp->right->label;
  exp->label = (l == r) ? l + 1 : std::max(l, r);
  return 0;
}

int Opt2Visitor::visit(NumberExp *exp) { exp->label = 1; return 0; }
int Opt2Visitor::visit(StringExp *exp) { exp->label = 1; return 0; }
int Opt2Visitor::visit(FloatExp *exp)  { exp->label = 1; return 0; }
int Opt2Visitor::visit(AddrExp *exp)   { exp->label = 1; return 0; }
int Opt2Visitor::visit(DerefExp *exp)  { exp->ptr->accept(this); exp->label = 1; return 0; }
int Opt2Visitor::visit(LambdaExp *exp) { exp->label = 1; return 0; }
int Opt2Visitor::visit(IdExp *exp)     { exp->label = 1; return 0; }
int Opt2Visitor::visit(IndexExp *exp)  { exp->label = 1; return 0; }
int Opt2Visitor::visit(MatrixExp *exp) { exp->label = 1; return 0; }
int Opt2Visitor::visit(FieldExp *exp)  { exp->label = 1; return 0; }
int Opt2Visitor::visit(FcallExp *exp)  { exp->label = 1; return 0; }

int Opt2Visitor::visit(UnaryExp *exp) {
  exp->operand->accept(this);
  exp->label = exp->operand->label;
  return 0;
}

int Opt2Visitor::visit(Program *p) {
  for (auto i : p->sdlist) i->accept(this);
  for (auto i : p->vdlist) i->accept(this);
  for (auto i : p->fdlist) i->accept(this);
  return 0;
}

int Opt2Visitor::visit(StructDec *sd) {
  for (auto i : sd->fields) i->accept(this);
  return 0;
}

int Opt2Visitor::visit(PrintStm *stm) { stm->e->accept(this); return 0; }
int Opt2Visitor::visit(AssignStm *stm) { stm->e->accept(this); return 0; }

int Opt2Visitor::visit(ExpListSize *) { return 0; }
int Opt2Visitor::visit(ExpListVals *) { return 0; }
int Opt2Visitor::visit(ExpMatrixSize *) { return 0; }
int Opt2Visitor::visit(ExpMatrixVals *) { return 0; }
int Opt2Visitor::visit(WhileStm *stm) { stm->condition->accept(this); stm->b->accept(this); return 0; }
int Opt2Visitor::visit(DoWhileStm *stm) { stm->condition->accept(this); stm->b->accept(this); return 0; }

int Opt2Visitor::visit(IfStm *stm) {
  stm->condition->accept(this);
  if (stm->then) stm->then->accept(this);
  if (stm->els) stm->els->accept(this);
  return 0;
}

int Opt2Visitor::visit(BreakStm *) { return 0; }

int Opt2Visitor::visit(SwitchStm *stm) {
  stm->e->accept(this);
  for (auto j : stm->default_body) j->accept(this);
  for (auto i : stm->cases)
    for (auto j : i->body) j->accept(this);
  return 0;
}

int Opt2Visitor::visit(Body *body) {
  for (auto i : body->declarations) i->accept(this);
  for (auto i : body->StmList) i->accept(this);
  return 0;
}

int Opt2Visitor::visit(VarDec *vd) {
  if (vd->init) vd->init->accept(this);
  return 0;
}
int Opt2Visitor::visit(ReturnStm *r) { r->e->accept(this); return 0; }

int Opt2Visitor::visit(FunDec *fd) {
  fd->cuerpo->accept(this);
  return 0;
}


// ---------------------------------------------------------------------------
// Impresor del AST (visualización del AST como árbol indentado, Sem4).
// Recorre el AST con dynamic_cast y vuelca una representación legible. Se usa
// desde la aplicación (modo --ast) para mostrar la estructura del programa.
// ---------------------------------------------------------------------------
static const char *binName(BinaryOp op) {
  switch (op) {
  case PLUS_OP: return "+";   case MINUS_OP: return "-";
  case MUL_OP: return "*";    case DIV_OP: return "/";
  case POW_OP: return "**";   case LE_OP: return "<";
  case GT_OP: return ">";     case LEQ_OP: return "<=";
  case GEQ_OP: return ">=";   case EQ_OP: return "==";
  case NE_OP: return "!=";    case AND_OP: return "&&";
  case OR_OP: return "||";    default: return "?";
  }
}

static void ind(std::ostream &o, int d) {
  for (int i = 0; i < d; i++) o << "  ";
}

static void pExp(Exp *e, std::ostream &o, int d);

static void pExp(Exp *e, std::ostream &o, int d) {
  ind(o, d);
  if (!e) { o << "- (null)\n"; return; }
  if (auto *n = dynamic_cast<NumberExp *>(e)) {
    o << "- NumberExp(" << n->value << ")\n";
  } else if (auto *fl = dynamic_cast<FloatExp *>(e)) {
    o << "- FloatExp(" << fl->value << ")\n";
  } else if (auto *s = dynamic_cast<StringExp *>(e)) {
    o << "- StringExp(\"" << s->value << "\")\n";
  } else if (auto *id = dynamic_cast<IdExp *>(e)) {
    o << "- IdExp(" << id->value << ")\n";
  } else if (auto *b = dynamic_cast<BinaryExp *>(e)) {
    o << "- BinaryExp(" << binName(b->op) << ")\n";
    pExp(b->left, o, d + 1);
    pExp(b->right, o, d + 1);
  } else if (auto *u = dynamic_cast<UnaryExp *>(e)) {
    o << "- UnaryExp(" << (u->op == NEG_OP ? "-" : "!") << ")\n";
    pExp(u->operand, o, d + 1);
  } else if (auto *ix = dynamic_cast<IndexExp *>(e)) {
    o << "- IndexExp(" << ix->name << ")\n";
    pExp(ix->index, o, d + 1);
  } else if (auto *m = dynamic_cast<MatrixExp *>(e)) {
    o << "- MatrixExp(" << m->name << ")\n";
    pExp(m->row, o, d + 1);
    pExp(m->col, o, d + 1);
  } else if (auto *ad = dynamic_cast<AddrExp *>(e)) {
    o << "- AddrExp(&" << ad->name << ")\n";
  } else if (auto *dr = dynamic_cast<DerefExp *>(e)) {
    o << "- DerefExp(*)\n";
    pExp(dr->ptr, o, d + 1);
  } else if (auto *lm = dynamic_cast<LambdaExp *>(e)) {
    o << "- LambdaExp(" << lm->name << ")\n";
  } else if (auto *f = dynamic_cast<FieldExp *>(e)) {
    o << "- FieldExp(" << f->object << "." << f->field << ")\n";
  } else if (auto *fc = dynamic_cast<FcallExp *>(e)) {
    o << "- FcallExp(" << fc->nombre << ")\n";
    for (auto *a : fc->argumentos) pExp(a, o, d + 1);
  } else if (auto *ls = dynamic_cast<ExpListSize *>(e)) {
    o << "- new " << ls->type << "[] (tamaño)\n";
    pExp(ls->size, o, d + 1);
  } else if (auto *lv = dynamic_cast<ExpListVals *>(e)) {
    o << "- new " << lv->type << " {valores}\n";
    for (auto *v : lv->values) pExp(v, o, d + 1);
  } else if (auto *ms = dynamic_cast<ExpMatrixSize *>(e)) {
    o << "- new " << ms->type << "[][] (tamaño)\n";
    pExp(ms->rows, o, d + 1);
    pExp(ms->cols, o, d + 1);
  } else if (auto *mv = dynamic_cast<ExpMatrixVals *>(e)) {
    o << "- new " << mv->type << "[][] {valores}\n";
    pExp(mv->rows, o, d + 1);
    pExp(mv->cols, o, d + 1);
    for (auto *v : mv->values) pExp(v, o, d + 1);
  } else {
    o << "- Exp\n";
  }
}

static void pBody(Body *b, std::ostream &o, int d);

static void pStm(Stm *s, std::ostream &o, int d) {
  ind(o, d);
  if (auto *a = dynamic_cast<AssignStm *>(s)) {
    o << "- AssignStm\n";
    ind(o, d + 1); o << "target:\n"; pExp(a->target, o, d + 2);
    ind(o, d + 1); o << "value:\n";  pExp(a->e, o, d + 2);
  } else if (auto *p = dynamic_cast<PrintStm *>(s)) {
    o << "- PrintStm\n"; pExp(p->e, o, d + 1);
  } else if (auto *r = dynamic_cast<ReturnStm *>(s)) {
    o << "- ReturnStm\n"; pExp(r->e, o, d + 1);
  } else if (auto *i = dynamic_cast<IfStm *>(s)) {
    o << "- IfStm\n";
    ind(o, d + 1); o << "cond:\n"; pExp(i->condition, o, d + 2);
    ind(o, d + 1); o << "then:\n"; pBody(i->then, o, d + 2);
    if (i->els) { ind(o, d + 1); o << "else:\n"; pBody(i->els, o, d + 2); }
  } else if (auto *w = dynamic_cast<WhileStm *>(s)) {
    o << "- WhileStm\n";
    ind(o, d + 1); o << "cond:\n"; pExp(w->condition, o, d + 2);
    ind(o, d + 1); o << "body:\n"; pBody(w->b, o, d + 2);
  } else if (auto *dw = dynamic_cast<DoWhileStm *>(s)) {
    o << "- DoWhileStm\n";
    ind(o, d + 1); o << "body:\n"; pBody(dw->b, o, d + 2);
    ind(o, d + 1); o << "cond:\n"; pExp(dw->condition, o, d + 2);
  } else if (dynamic_cast<BreakStm *>(s)) {
    o << "- BreakStm\n";
  } else if (auto *sw = dynamic_cast<SwitchStm *>(s)) {
    o << "- SwitchStm\n";
    pExp(sw->e, o, d + 1);
    for (auto *c : sw->cases) {
      ind(o, d + 1); o << "case " << c->value << ":\n";
      for (auto *cs : c->body) pStm(cs, o, d + 2);
    }
    if (!sw->default_body.empty()) {
      ind(o, d + 1); o << "default:\n";
      for (auto *cs : sw->default_body) pStm(cs, o, d + 2);
    }
  } else {
    o << "- Stm\n";
  }
}

static void pBody(Body *b, std::ostream &o, int d) {
  if (!b) { ind(o, d); o << "- (vacío)\n"; return; }
  for (auto *vd : b->declarations) {
    ind(o, d); o << "- VarDec " << (vd->type.empty() ? "(inferido)" : vd->type) << ":";
    for (auto &v : vd->vars) o << " " << v;
    o << "\n";
    if (vd->init) pExp(vd->init, o, d + 1);
  }
  for (auto *s : b->StmList) pStm(s, o, d);
}

void printAst(Program *p, std::ostream &o) {
  o << "Program\n";
  for (auto *sd : p->sdlist) {
    o << "  - StructDec " << sd->name << "\n";
    for (auto *fld : sd->fields) {
      o << "      - campo " << fld->type << ":";
      for (auto &v : fld->vars) o << " " << v;
      o << "\n";
    }
  }
  for (auto *vd : p->vdlist) {
    o << "  - VarDec global " << (vd->type.empty() ? "(inferido)" : vd->type) << ":";
    for (auto &v : vd->vars) o << " " << v;
    o << "\n";
  }
  for (auto *fd : p->fdlist) {
    o << "  - FunDec " << fd->tipo << " " << fd->nombre << "(";
    for (size_t i = 0; i < fd->Pnombres.size(); ++i) {
      if (i) o << ", ";
      o << fd->Ptipos[i] << " " << fd->Pnombres[i];
    }
    o << ")\n";
    pBody(fd->cuerpo, o, 2);
  }
}
