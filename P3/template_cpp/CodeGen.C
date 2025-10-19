#include "CodeGen.H"

void CodeGen::visitProgram(Program *t) {} //abstract class
void CodeGen::visitDef(Def *t) {} //abstract class
void CodeGen::visitField(Field *t) {} //abstract class
void CodeGen::visitStm(Stm *t) {} //abstract class
void CodeGen::visitExp(Exp *t) {} //abstract class
void CodeGen::visitType(Type *t) {} //abstract class
void CodeGen::visitChar(Char x) {}
void CodeGen::visitDouble(Double x) {}
void CodeGen::visitString(String x) {}


void CodeGen::generate(Program* prog)
{
    prog->accept(this);
    module->print(llvm::outs(), nullptr);
    //module->print(llvm::errs(), nullptr);
}

bool CodeGen::isStructLike(llvm::Type *ty) {
    return ty->isStructTy() ||
           (ty->isPointerTy() && ty->getPointerElementType()->isStructTy());
}

llvm::Value* CodeGen::cmpStruct(llvm::Value *L, llvm::Value *R, bool wantEq) {
    llvm::Value *left  = L;
    llvm::Value *right = R;
    if (left->getType()->isPointerTy() &&
        left->getType()->getPointerElementType()->isStructTy()) {
        auto *eltTy = left->getType()->getPointerElementType();
        left  = builder.CreateLoad(eltTy, left,  "load_l");
        right = builder.CreateLoad(eltTy, right, "load_r");
    }

    auto *stTy = llvm::cast<llvm::StructType>(left->getType());
    unsigned numFields = stTy->getNumElements();

    llvm::Value *acc = wantEq
        ? llvm::ConstantInt::get(context, llvm::APInt(1, 1))
        : llvm::ConstantInt::get(context, llvm::APInt(1, 0));

    for (unsigned i = 0; i < numFields; ++i) {
        llvm::Value *fL = builder.CreateExtractValue(left,  {i}, "fld_l");
        llvm::Value *fR = builder.CreateExtractValue(right, {i}, "fld_r");

        llvm::Value *cmp;
        if (isStructLike(fL->getType())) {
            cmp = cmpStruct(fL, fR, wantEq);
        } else {
            cmp = wantEq
                ? builder.CreateICmpEQ(fL, fR, "eq_field")
                : builder.CreateICmpNE(fL, fR, "ne_field");
        }

        acc = wantEq
            ? builder.CreateAnd(acc, cmp, "and_eq")
            : builder.CreateOr (acc, cmp, "or_ne");
    }

    return acc;
}

llvm::Value *CodeGen::getPtrToField(Exp *baseExp, const std::string &field)
{
    llvm::Value *basePtr = nullptr;

    if (auto *id  = dynamic_cast<EIdent *>(baseExp)) {
        basePtr = lookupVariable(id->ident_);
    }
    else if (auto *prj = dynamic_cast<EProj *>(baseExp)) {
        basePtr = getPtrToField(prj->exp_, prj->ident_);
    }
    else {
        baseExp->accept(this);
        llvm::Value *val = lastValue;

        if (val->getType()->isPointerTy())
            basePtr = val;
        else if (val->getType()->isStructTy())
        {
            basePtr = builder.CreateAlloca(val->getType(), nullptr, "tmp.str");
            builder.CreateStore(val, basePtr);
        }
        else {
            std::cerr << "Projection on non-struct value\n";
            exit(1);
        }
    }

    auto *stTy  = llvm::cast<llvm::StructType>(
                    basePtr->getType()->getPointerElementType());
    unsigned idx = getFieldIndex(stTy, field);

    return builder.CreateStructGEP(stTy, basePtr, idx, field + ".ptr");
}

void CodeGen::visitPDefs(PDefs *p_defs)
{
    if (p_defs->listdef_)
    {
        for (auto& def : *p_defs->listdef_) {
            if(dynamic_cast<DVar*>(def)) {

                def->accept(this);
            } else if (dynamic_cast<DFun*>(def)) {
                auto fun = dynamic_cast<DFun*>(def);
                addFunction(fun->ident_, nullptr);
                def->accept(this);
            } else if (dynamic_cast<DStruct*>(def)) {
                def->accept(this);
                auto d_struct = dynamic_cast<DStruct*>(def);
            } else {
                std::cerr << "Error: Unknown definition type." << std::endl;
                exit(1);
            }
        }
    }
}

void CodeGen::visitDVar(DVar *d_var)
{
    if (d_var->type_) d_var->type_->accept(this);

    if (!lastType) {
        std::cerr << "Error: Null type encountered in DVar for " << d_var->ident_ << "\n";
        exit(1);
    }

    llvm::GlobalVariable* globalVar = new llvm::GlobalVariable(
        *module,
        lastType,
        false,
        llvm::GlobalValue::ExternalLinkage,
        llvm::Constant::getNullValue(lastType),
        d_var->ident_
    );
    addGlobalVar(d_var->ident_, globalVar);
}

void CodeGen::visitDFun(DFun *d_fun) {
    llvm::Type *retType = getLLVMType(d_fun->type_);
    if (!retType) {
        std::cerr << "Error: Unsupported return type in function "
                  << d_fun->ident_ << "\n";
        exit(1);
    }

    llvm::FunctionType *funcType =
        llvm::FunctionType::get(retType, false);

    llvm::Function *func = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        d_fun->ident_,
        module
    );
    addFunction(d_fun->ident_, func);
    currentFunction = func;

    llvm::BasicBlock *entryBB =
    llvm::BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(entryBB);

    if (d_fun->liststm_) {
        d_fun->liststm_->accept(this);
    }

    if (retType->isVoidTy())
        builder.CreateRetVoid();
    else
        builder.CreateRet(llvm::Constant::getNullValue(retType));
}

void CodeGen::visitDStruct(DStruct *d_struct)
{
    std::string structName = d_struct->ident_;

    std::vector<llvm::Type*> fieldTypes;
    std::vector<std::string> fieldNames;

    if (d_struct->listfield_) {
        for (Field* field : *d_struct->listfield_) {
            FDecl* fdecl = dynamic_cast<FDecl*>(field);
            if (!fdecl) {
                std::cerr << "Error: Unsupported field type in struct " << structName << "\n";
                exit(1);
            }

            llvm::Type* llvmFieldType = getLLVMType(fdecl->type_);
            if (!llvmFieldType) {
                std::cerr << "Error: Unsupported LLVM type in struct field\n";
                exit(1);
            }

            fieldTypes.push_back(llvmFieldType);
            fieldNames.push_back(fdecl->ident_);
        }
    }

    llvm::StructType* structType = llvm::StructType::create(context, fieldTypes, structName);
    addStruct(structName, structType);
    structFieldNames[structName] = fieldNames;
}

void CodeGen::visitFDecl(FDecl *f_decl) {
    if (f_decl->type_) {
        f_decl->type_->accept(this);
    }
    if (!lastType) {
        std::cerr << "Error: Unsupported field type in struct\n";
        exit(1);
    }

    currentFieldTypes.push_back(lastType);

}

void CodeGen::visitSExp(SExp *s_exp)
{
    if (s_exp->exp_) s_exp->exp_->accept(this);
}

void CodeGen::visitSReturn(SReturn *s_return)
{
    if (s_return->exp_) {
        s_return->exp_->accept(this);
        builder.CreateRet(lastValue);
    } else {
        exit(1);
    }
}

void CodeGen::visitSReturnV(SReturnV *s_return_v)
{
    builder.CreateRetVoid();
}

void CodeGen::visitSWhile(SWhile *s_while)
{
    llvm::Function *func = currentFunction;

    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "while.cond", func);
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "while.body");
    llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "while.after");

    // Jump to the condition block
    builder.CreateBr(condBB);

    // Condition block
    builder.SetInsertPoint(condBB);
    if (s_while->exp_) s_while->exp_->accept(this);
    llvm::Value *condVal = lastValue;
    if (!condVal)
    {
        std::cerr << "Error: invalid while loop condition.\n";
        exit(1);
    }
    builder.CreateCondBr(condVal, bodyBB, afterBB);

    // Body block
    func->getBasicBlockList().push_back(bodyBB);
    builder.SetInsertPoint(bodyBB);
    if (s_while->stm_) s_while->stm_->accept(this);
    builder.CreateBr(condBB); // back to condition

    // After bloc
    func->getBasicBlockList().push_back(afterBB);
    builder.SetInsertPoint(afterBB);
}

void CodeGen::visitSDoWhile(SDoWhile *s_do_while)
{
    llvm::Function *func = currentFunction;

    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "do.body", func);
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "do.cond");
    llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "do.after");

    // Jump to body first (do-while executes body at least once)
    builder.CreateBr(bodyBB);

    // Body block
    builder.SetInsertPoint(bodyBB);
    if (s_do_while->stm_) s_do_while->stm_->accept(this);
    builder.CreateBr(condBB);

    // Condition block
    func->getBasicBlockList().push_back(condBB);
    builder.SetInsertPoint(condBB);
    if (s_do_while->exp_) s_do_while->exp_->accept(this);
    llvm::Value *condVal = lastValue;
    if (!condVal)
    {
        std::cerr << "Error: invalid do-while loop condition.\n";
        exit(1);
    }
    builder.CreateCondBr(condVal, bodyBB, afterBB);

    // After block
    func->getBasicBlockList().push_back(afterBB);
    builder.SetInsertPoint(afterBB);
}

void CodeGen::visitSFor(SFor *s_for)
{
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "for.cond", currentFunction);
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "for.body", currentFunction);
    llvm::BasicBlock *incBB = llvm::BasicBlock::Create(context, "for.inc", currentFunction);
    llvm::BasicBlock *endBB = llvm::BasicBlock::Create(context, "for.end", currentFunction);

    if (s_for->exp_1) s_for->exp_1->accept(this);
    builder.CreateBr(condBB);
    builder.SetInsertPoint(condBB);
    llvm::Value *condVal;
    if (s_for->exp_2){
    s_for->exp_2->accept(this);
    condVal = lastValue;
    } else{
        condVal = llvm::ConstantInt::getTrue(context);
    }
    builder.CreateCondBr(condVal, bodyBB, endBB);
    builder.SetInsertPoint(bodyBB);
    //currentFunction->getBasicBlockList().push_back(endBB);
    //currentFunction->getBasicBlockList().push_back(incBB);
    if (s_for->stm_) s_for->stm_->accept(this);
    builder.CreateBr(incBB);
    //currentFunction->getBasicBlockList().pop_back();
    //currentFunction->getBasicBlockList().pop_back();
    if (s_for->exp_3) s_for->exp_3->accept(this);
    builder.CreateBr(condBB);
    builder.SetInsertPoint(endBB);
    
}

void CodeGen::visitSBlock(SBlock *s_block)
{
    pushScope();

    if (s_block->liststm_) {
        s_block->liststm_->accept(this);
    }

    popScope();
}

void CodeGen::visitSIfElse(SIfElse *s_if_else)
{
    s_if_else->exp_->accept(this);
    llvm::Value* cond = lastValue;

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(context, "then", currentFunction);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(context, "else");
    llvm::BasicBlock* mergeBB= llvm::BasicBlock::Create(context, "ifcont");

    builder.CreateCondBr(cond, thenBB, elseBB);

    builder.SetInsertPoint(thenBB);
    s_if_else->stm_1->accept(this);
    builder.CreateBr(mergeBB);

    currentFunction->getBasicBlockList().push_back(elseBB);
    builder.SetInsertPoint(elseBB);
    if (s_if_else->stm_2) s_if_else->stm_2->accept(this);
    builder.CreateBr(mergeBB);

    currentFunction->getBasicBlockList().push_back(mergeBB);
    builder.SetInsertPoint(mergeBB);
}


void CodeGen::visitETrue(ETrue *) {
    lastValue = llvm::ConstantInt::get(
        context,
        llvm::APInt(1, 1, false)
    );
}

void CodeGen::visitEFalse(EFalse *) {
    lastValue = llvm::ConstantInt::get(
        context,
        llvm::APInt(1, 0, false)
    );
}

void CodeGen::visitEInt(EInt *e_int)
{
    lastValue = builder.getInt32(static_cast<uint32_t>(e_int->integer_));
}

void CodeGen::visitEIdent(EIdent *e_ident)
{
    visitIdent(e_ident->ident_);
}

void CodeGen::visitEApp(EApp *e_app) {
    auto fit = functionTable.find(e_app->ident_);
    if (fit == functionTable.end()) {
        std::cerr << "Error: Function '" << e_app->ident_
                  << "' not found.\n";
        exit(1);
    }
    llvm::Function *func = fit->second;

    llvm::ArrayRef<llvm::Value*> args{};
    if (func->getReturnType()->isVoidTy()) {
    builder.CreateCall(func, {});
    lastValue = nullptr; 
    } else {
        lastValue = builder.CreateCall(func, {}, e_app->ident_);
    }
}

void CodeGen::visitEProj(EProj *e_proj)
{
    llvm::Value *fieldPtr = getPtrToField(e_proj->exp_, e_proj->ident_);

    llvm::Type *eltTy = fieldPtr->getType()->getPointerElementType();
    lastValue         = builder.CreateLoad(eltTy, fieldPtr, e_proj->ident_);
}

void CodeGen::visitEPIncr(EPIncr *ep_incr)
{
    llvm::Value *addr = nullptr;

    if (auto id = dynamic_cast<EIdent*>(ep_incr->exp_))
        addr = lookupVariable(id->ident_);
    else if (auto proj = dynamic_cast<EProj*>(ep_incr->exp_))
        addr = getPtrToField(proj->exp_, proj->ident_);
    else {
        std::cerr << "Post-increment target not addressable\n";
        exit(1);
    }

    llvm::Value *oldVal = builder.CreateLoad(addr->getType()->getPointerElementType(), addr);
    llvm::Value *incVal = builder.CreateAdd(oldVal, llvm::ConstantInt::get(oldVal->getType(), 1));
    builder.CreateStore(incVal, addr);
    lastValue = oldVal;  // return original value (post-incr)
}

void CodeGen::visitEPDecr(EPDecr *ep_decr)
{
    llvm::Value *addr = nullptr;

    if (auto id = dynamic_cast<EIdent*>(ep_decr->exp_))
        addr = lookupVariable(id->ident_);
    else if (auto proj = dynamic_cast<EProj*>(ep_decr->exp_))
        addr = getPtrToField(proj->exp_, proj->ident_);
    else {
        std::cerr << "Post-decrement target not addressable\n";
        exit(1);
    }

    llvm::Value *oldVal = builder.CreateLoad(addr->getType()->getPointerElementType(), addr);
    llvm::Value *decVal = builder.CreateSub(oldVal, llvm::ConstantInt::get(oldVal->getType(), 1));
    builder.CreateStore(decVal, addr);
    lastValue = oldVal;  
}

void CodeGen::visitEIncr(EIncr *e_incr)
{
    llvm::Value *addr = nullptr;

    if (auto id = dynamic_cast<EIdent*>(e_incr->exp_))
        addr = lookupVariable(id->ident_);
    else if (auto proj = dynamic_cast<EProj*>(e_incr->exp_))
        addr = getPtrToField(proj->exp_, proj->ident_);
    else {
        std::cerr << "Pre-increment target not addressable\n";
        exit(1);
    }

    llvm::Value *val = builder.CreateLoad(addr->getType()->getPointerElementType(), addr);
    llvm::Value *incVal = builder.CreateAdd(val, llvm::ConstantInt::get(val->getType(), 1));
    builder.CreateStore(incVal, addr);
    lastValue = incVal;  
}

void CodeGen::visitEDecr(EDecr *e_decr)
{
    llvm::Value *addr = nullptr;

    if (auto id = dynamic_cast<EIdent*>(e_decr->exp_))
        addr = lookupVariable(id->ident_);
    else if (auto proj = dynamic_cast<EProj*>(e_decr->exp_))
        addr = getPtrToField(proj->exp_, proj->ident_);
    else {
        std::cerr << "Pre-decrement target not addressable\n";
        exit(1);
    }

    llvm::Value *val = builder.CreateLoad(addr->getType()->getPointerElementType(), addr);
    llvm::Value *decVal = builder.CreateSub(val, llvm::ConstantInt::get(val->getType(), 1));
    builder.CreateStore(decVal, addr);
    lastValue = decVal;  
}

void CodeGen::visitEUPlus(EUPlus *eu_plus)
{
    if (eu_plus->exp_) eu_plus->exp_->accept(this);
}

void CodeGen::visitEUMinus(EUMinus *eu_minus) {
    if (eu_minus->exp_) {
        eu_minus->exp_->accept(this);
        lastValue = builder.CreateNeg(lastValue, "neg_tmp");
    }
}

void CodeGen::visitETimes(ETimes *e_times)
{
    if (e_times->exp_1) e_times->exp_1->accept(this);
    llvm::Value *lhs = lastValue;
    if (e_times->exp_2) e_times->exp_2->accept(this);
    llvm::Value *rhs = lastValue;
    lastValue = builder.CreateMul(lhs, rhs, "multiply_tmp");
}

void CodeGen::visitEDiv(EDiv *e_div)
{
    e_div->exp_1->accept(this);
    llvm::Value *lhs = lastValue;
    e_div->exp_2->accept(this);
    llvm::Value *rhs = lastValue;
    lastValue = builder.CreateSDiv(lhs, rhs, "divtmp");
}

void CodeGen::visitEPlus(EPlus *e_plus)
{
    if (e_plus->exp_1) e_plus->exp_1->accept(this);
    if (e_plus->exp_2) e_plus->exp_2->accept(this);
}

void CodeGen::visitEMinus(EMinus *e_minus)
{
    e_minus->exp_1->accept(this);
    llvm::Value *lhs = lastValue;

    e_minus->exp_2->accept(this);
    llvm::Value *rhs = lastValue;

    lastValue = builder.CreateSub(lhs, rhs, "subtmp");
}

void CodeGen::visitETwc(ETwc *e_twc)
{
    if (e_twc->exp_1) e_twc->exp_1->accept(this);
    llvm::Value *lhs = lastValue;
    if (e_twc->exp_2) e_twc->exp_2->accept(this);
    llvm::Value *rhs = lastValue;
    llvm::Type *ty = lhs->getType();
    
    llvm::Type *i32Ty = llvm::Type::getInt32Ty(context);
    
    auto *negOne = llvm::ConstantInt::get(i32Ty, -1, /*isSigned=*/true);
    auto *zero   = llvm::ConstantInt::get(i32Ty,  0, /*isSigned=*/true);
    auto *one    = llvm::ConstantInt::get(i32Ty,  1, /*isSigned=*/true);

    llvm::Value *ltCmp, *gtCmp;
    if (ty->isIntegerTy()) {
        // signed integer compares
        ltCmp = builder.CreateICmpSLT(lhs, rhs, "iltcmp");
        gtCmp = builder.CreateICmpSGT(lhs, rhs, "igtcmp");
    }
    else {
        llvm::errs() << "Error: three‑way compare on unsupported type\n";
        exit(1);
    }

    // if L<R then -1, else if L>R then +1, else 0
    llvm::Value *ltVal = builder.CreateSelect(ltCmp, negOne, zero, "ltval");
    lastValue = builder.CreateSelect(gtCmp, one, ltVal, "cmp_three_way");
}


void CodeGen::visitELt(ELt *e_lt)
{
    if (e_lt->exp_1) e_lt->exp_1->accept(this);
    llvm::Value *lhs = lastValue;
    if (e_lt->exp_2) e_lt->exp_2->accept(this);
    llvm::Value *rhs = lastValue;

    llvm::Type *ty = lhs->getType();
    // structs have no natural ordering
    if (isStructLike(ty)) {
        std::cerr << "Error: cannot perform < on structs\n";
        exit(1);
    }
    // signed int or bool <
    else {
        lastValue = builder.CreateICmpSLT(lhs, rhs, "ilt");
    }
}

void CodeGen::visitEGt(EGt *e_gt)
{
    e_gt->exp_1->accept(this);
    llvm::Value *lhs = lastValue;

    e_gt->exp_2->accept(this);
    llvm::Value *rhs = lastValue;

    lastValue = builder.CreateICmpSGT(lhs, rhs, "gt_tmp");
}

void CodeGen::visitELtEq(ELtEq *e_lt_eq)
{
    if (e_lt_eq->exp_1) e_lt_eq->exp_1->accept(this);
    llvm::Value *lhs = lastValue;
    if (e_lt_eq->exp_2) e_lt_eq->exp_2->accept(this);
    llvm::Value *rhs = lastValue;
    lastValue = builder.CreateICmpSLE(lhs, rhs, "sle_tmp");
}

void CodeGen::visitEGtEq(EGtEq *e_gt_eq)
{
    if (e_gt_eq->exp_1) e_gt_eq->exp_1->accept(this);
    llvm::Value *lhs = lastValue;
    if (e_gt_eq->exp_2) e_gt_eq->exp_2->accept(this);
    llvm::Value *rhs = lastValue;
    lastValue = builder.CreateICmpSGE(lhs, rhs, "ge_tmp");
}

void CodeGen::visitEEq(EEq *e_eq) {
    e_eq->exp_1->accept(this);
    llvm::Value *L = lastValue;
    e_eq->exp_2->accept(this);
    llvm::Value *R = lastValue;

    if (isStructLike(L->getType())) {
        lastValue = cmpStruct(L, R, true);
    } else {
        lastValue = builder.CreateICmpEQ(L, R, "eq");
    }
}

void CodeGen::visitENEq(ENEq *e_ne) {
    e_ne->exp_1->accept(this);
    llvm::Value *L = lastValue;
    e_ne->exp_2->accept(this);
    llvm::Value *R = lastValue;

    if (isStructLike(L->getType())) {
        lastValue = cmpStruct(L, R, false);
    } else {
        lastValue = builder.CreateICmpNE(L, R, "ne");
    }
}


void CodeGen::visitEAnd(EAnd *e_and)
{
    if (e_and->exp_1) e_and->exp_1->accept(this);
    llvm::Value *lhs = lastValue;
    if (e_and->exp_2) e_and->exp_2->accept(this);
    llvm::Value *rhs = lastValue;
    lastValue = builder.CreateAnd(lhs, rhs, "and_tmp");
}

void CodeGen::visitEOr(EOr *e_or)
{
    if (e_or->exp_1) e_or->exp_1->accept(this);
    llvm::Value *lhs = lastValue;
    if (e_or->exp_2) e_or->exp_2->accept(this);
    llvm::Value *rhs = lastValue;
    lastValue = builder.CreateOr(lhs, rhs, "or_tmp");
}

void CodeGen::visitEAss(EAss *e_ass)
{
    llvm::Value *addr = nullptr;

    if (auto *id = dynamic_cast<EIdent *>(e_ass->exp_1))
        addr = lookupVariable(id->ident_);
    else if (auto *pr = dynamic_cast<EProj *>(e_ass->exp_1))
        addr = getPtrToField(pr->exp_, pr->ident_);
    else {
        e_ass->exp_1->accept(this);
        addr = lastValue;
    }

    if (!addr || !addr->getType()->isPointerTy()) {
        std::cerr << "Assignment target is not addressable\n";
        exit(1);
    }

    e_ass->exp_2->accept(this);
    llvm::Value *rhs = lastValue;

    builder.CreateStore(rhs, addr);
    lastValue = rhs;
}

void CodeGen::visitECond(ECond *e_cond)
{
    if (e_cond->exp_1) e_cond->exp_1->accept(this);
    llvm::Value *condition = lastValue;
    if (e_cond->exp_2) e_cond->exp_2->accept(this);
    llvm::Value *trueValue = lastValue;
    if (e_cond->exp_3) e_cond->exp_3->accept(this);
    llvm::Value *falseValue = lastValue;
    lastValue = builder.CreateSelect(condition, trueValue, falseValue, "cond_tmp");
}

void CodeGen::visitType_bool(Type_bool *) {
    lastType = llvm::Type::getInt1Ty(context);
}

void CodeGen::visitType_int(Type_int *) {
    lastType = llvm::Type::getInt32Ty(context);
}

void CodeGen::visitType_void(Type_void *) {
    lastType = llvm::Type::getVoidTy(context);
}

void CodeGen::visitTypeIdent(TypeIdent *type_ident)
{
    auto sit = structTable.find(type_ident->ident_);
    if (sit != structTable.end()) {
        lastType = sit->second;
        return;
    }

    std::cerr << "Error: Struct type '" << type_ident->ident_ << "' not found.\n";
    exit(1);
}

void CodeGen::visitListDef(ListDef *list_def)
{
    for (ListDef::iterator i = list_def->begin() ; i != list_def->end() ; ++i)
    {
        (*i)->accept(this);
    }
}

void CodeGen::visitListField(ListField *list_field)
{
    for (ListField::iterator i = list_field->begin() ; i != list_field->end() ; ++i)
    {
        (*i)->accept(this);
    }
}

void CodeGen::visitListStm(ListStm *list_stm)
{
    for (ListStm::iterator i = list_stm->begin() ; i != list_stm->end() ; ++i)
    {
        (*i)->accept(this);
    }
}

void CodeGen::visitInteger(Integer x)
{
    lastValue = llvm::ConstantInt::get(context, llvm::APInt(32, x, true));
}

void CodeGen::visitIdent(Ident x) {
    if (auto *v = lookupVariable(x)) {
        if (auto *alloc = llvm::dyn_cast<llvm::AllocaInst>(v)) {
            lastValue = builder.CreateLoad(
                alloc->getAllocatedType(),
                alloc,
                x
            );
            return;
        }
        if (auto *gv = llvm::dyn_cast<llvm::GlobalVariable>(v)) {
            auto ptrTy = llvm::cast<llvm::PointerType>(gv->getType());
            llvm::Type *eltTy = ptrTy->getElementType();
            lastValue = builder.CreateLoad(
                eltTy,
                gv,
                x
            );
            return;
        }
    }

    auto fit = functionTable.find(x);
    if (fit != functionTable.end()) {
        lastValue = fit->second;
        return;
    }

    std::cerr << "Error: Identifier '" << x << "' not found.\n";
    exit(1);
}