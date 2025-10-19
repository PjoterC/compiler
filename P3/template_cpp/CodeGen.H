#ifndef CODEGEN_HEADER
#define CODEGEN_HEADER

#include "Absyn.H"
#include <ostream>
#include <memory>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <iostream>

using VarTable = std::unordered_map<std::string, llvm::Value*>;
using FnTable  = std::unordered_map<std::string, llvm::Function*>;
using StTable  = std::unordered_map<std::string, llvm::StructType*>;

class CodeGen : public Visitor
{
private:
    llvm::LLVMContext context;
    llvm::Module*      module;
    llvm::IRBuilder<>  builder;

    llvm::Function* currentFunction = nullptr;
    llvm::Value*    lastValue       = nullptr;
    llvm::Type*     lastType        = nullptr;

    llvm::StructType*             currentStruct    = nullptr;
    std::vector<llvm::Type*>      currentFieldTypes;
    std::unordered_map<std::string, std::vector<std::string>> structFieldNames;

    FnTable  functionTable;
    StTable  structTable;
    VarTable globalVarTable;
    using Scope = std::vector<VarTable>;
    Scope    scope;

    void pushScope() { scope.push_back({}); }
    void popScope()  { scope.pop_back(); }

    void addLocalVar(const std::string& name, llvm::AllocaInst* var) {
        if (scope.empty()) {
            std::cerr << "Error: No active scope to add local variable " << name << "\n";
            return;
        }
        scope.back()[name] = var;
    }

    void addGlobalVar(const std::string& name, llvm::GlobalVariable* var) {
        globalVarTable[name] = var;
    }
    void addFunction(const std::string& name, llvm::Function* func) {
        functionTable[name] = func;
    }
    void addStruct(const std::string& name, llvm::StructType* st) {
        structTable[name] = st;
    }

    llvm::Value* lookupVariable(const std::string& name) {
        for (auto it = scope.rbegin(); it != scope.rend(); ++it) {
            auto f = it->find(name);
            if (f != it->end()) return f->second;
        }
        auto g = globalVarTable.find(name);
        if (g != globalVarTable.end()) return g->second;
        return nullptr;
    }

    llvm::Value* getPtrToField(Exp *baseExp, const std::string &field);
    llvm::Value* cmpStruct(llvm::Value *L, llvm::Value *R, bool wantEq);
    static bool isStructLike(llvm::Type *ty);

    llvm::Type* getLLVMType(Type* type) {
        if (auto t = dynamic_cast<Type_int*>(type))  return builder.getInt32Ty();
        if (auto t = dynamic_cast<Type_bool*>(type)) return builder.getInt1Ty();
        if (auto t = dynamic_cast<Type_void*>(type)) return builder.getVoidTy();
        if (auto t = dynamic_cast<TypeIdent*>(type)) {
            auto it = structTable.find(t->ident_);
            if (it != structTable.end()) return it->second;
            std::cerr << "Error: Unknown struct type: " << t->ident_ << "\n";
            exit(1);
        }
        return nullptr;
    }

    unsigned getFieldIndex(llvm::StructType *st, const std::string &field) {
        auto it = structFieldNames.find(st->getName().str());
        if (it == structFieldNames.end()) {
            std::cerr << "Error: No field list for struct " << st->getName().str() << "\n";
            exit(1);
        }
        auto &names = it->second;
        auto pos = std::find(names.begin(), names.end(), field);
        if (pos == names.end()) {
            std::cerr << "Error: Field '" << field
                      << "' not found in struct " << st->getName().str() << "\n";
            exit(1);
        }
        return unsigned(std::distance(names.begin(), pos));
    }

public:
    CodeGen() : module(new llvm::Module("main", context)), builder(context) {}

    void visitProgram(Program *p);
    void visitDef(Def *p);
    void visitField(Field *p);
    void visitStm(Stm *p);
    void visitExp(Exp *p);
    void visitType(Type *p);
    void visitChar(Char x);
    void visitDouble(Double x);
    void visitString(String x);


    /* Piotr Ciupiński */
    void generate(Program* prog);
    void visitPDefs(PDefs *p);
    void visitDVar(DVar *p);
    void visitDStruct(DStruct *p);
    void visitSExp(SExp *p);
    void visitListDef(ListDef *p);
    void visitListField(ListField *p);
    void visitListStm(ListStm *p);

    // Task 2
    void visitSWhile(SWhile *p);
    void visitEMinus(EMinus *p);
    void visitEGt(EGt *p);

    /* Rafał Mironko */
    void visitFDecl(FDecl *p); //
    void visitDFun(DFun *p); //
    void visitEApp(EApp *p); //
    void visitEInt(EInt *p); //
    void visitSReturn(SReturn *p); //
    void visitSReturnV(SReturnV *p); //
    void visitSBlock(SBlock *p); //
    void visitType_bool(Type_bool *p); //
    void visitType_int(Type_int *p); //
    void visitType_void(Type_void *p); //
    void visitIdent(Ident x); //
    void visitEFalse(EFalse *p); //
    void visitETrue(ETrue *p); //
    void visitEProj(EProj *p); //
    void visitEAss(EAss *p); //

    // Task 2
    void visitSIfElse(SIfElse *p); //
    void visitEDiv(EDiv *p); //
    void visitEGtEq(EGtEq *p); //

    /* Arion Minina */
    void visitETwc(ETwc *p);
    void visitEEq(EEq *p);
    void visitENEq(ENEq *p);
    void visitEAnd(EAnd *p);
    void visitEOr(EOr *p);
    void visitECond(ECond *p);
    void visitTypeIdent(TypeIdent *p);
    void visitInteger(Integer x);

    // Task 2
    void visitSFor(SFor *p);
    void visitETimes(ETimes *p);
    void visitELtEq(ELtEq *p);

    /* Lars Bornträger */
    void visitEIdent(EIdent *p);
    void visitEPIncr(EPIncr *p);
    void visitEPDecr(EPDecr *p);
    void visitEIncr(EIncr *p);
    void visitEDecr(EDecr *p);
    void visitEUPlus(EUPlus *p);
    void visitEUMinus(EUMinus *p);

    // Task 2
    void visitSDoWhile(SDoWhile *p);
    void visitEPlus(EPlus *p);
    void visitELt(ELt *p);
};

#endif