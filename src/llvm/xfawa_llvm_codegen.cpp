#include "xfawa_llvm_codegen.h"
#include <sstream>
#include <llvm/ADT/APInt.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Support/TargetSelect.h>

namespace xfawa {

static llvm::ConstantInt* createConstInt(llvm::LLVMContext& ctx, llvm::IntegerType* ty, int64_t value) {
    return static_cast<llvm::ConstantInt*>(llvm::ConstantInt::get(ty, llvm::APInt(ty->getBitWidth(), value, true)));
}

LLVMCodegen::LLVMCodegen(llvm::LLVMContext& ctx, llvm::Module* mod) 
    : context(ctx), module(mod), builder(ctx), hasMainFunction(false), loopEndBB(nullptr) {
    initBuiltins();
}

void LLVMCodegen::initBuiltins() {
    llvm::FunctionType* printfType = llvm::FunctionType::get(builder.getInt32Ty(), {builder.getInt8Ty()->getPointerTo()}, true);
    llvm::Function* printfFunc = llvm::Function::Create(printfType, llvm::Function::LinkageTypes::ExternalLinkage, 0, "printf", module.get());
    printfFunc->setCallingConv(llvm::CallingConv::C);
}

void LLVMCodegen::initializeTargets() {
    // Initialize X86 target
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmPrinter();
    LLVMInitializeX86AsmParser();
    
    // Initialize native target
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

llvm::Value* LLVMCodegen::codegen(NumberLiteral* expr) {
    return createConstInt(context, llvm::Type::getInt32Ty(context), expr->value);
}

llvm::Value* LLVMCodegen::codegen(BooleanLiteral* expr) {
    return createConstInt(context, llvm::Type::getInt1Ty(context), expr->value ? 1 : 0);
}

llvm::Value* LLVMCodegen::codegen(StringLiteral* expr) {
    return builder.CreateGlobalString(expr->value, "str");
}

llvm::Value* LLVMCodegen::codegen(VariableExpression* expr) {
    auto it = locals.find(expr->name);
    if (it != locals.end()) {
        llvm::AllocaInst* alloca = it->second;
        return builder.CreateLoad(alloca->getAllocatedType(), alloca, expr->name.c_str());
    }
    addError("Undefined variable: " + expr->name);
    return nullptr;
}

llvm::Value* LLVMCodegen::codegen(UnaryOp* expr) {
    llvm::Value* operandVal = codegen(expr->expr.get());
    if (!operandVal) return nullptr;
    
    switch (expr->op) {
        case UnaryOpType::NEGATE:
            return builder.CreateNeg(operandVal, "negtmp");
        case UnaryOpType::NOT:
            return builder.CreateNot(operandVal, "nottmp");
        default:
            return nullptr;
    }
}

llvm::Value* LLVMCodegen::codegen(BinaryOp* expr) {
    llvm::Value* leftVal = codegen(expr->left.get());
    if (!leftVal) return nullptr;
    
    llvm::Value* rightVal = codegen(expr->right.get());
    if (!rightVal) return nullptr;
    
    llvm::Type* leftType = leftVal->getType();
    llvm::Type* rightType = rightVal->getType();
    
    if (leftType->isIntegerTy(1)) {
        llvm::Type* i32 = llvm::Type::getInt32Ty(context);
        leftVal = builder.CreateZExtOrTrunc(leftVal, i32, "booltoi32");
    }
    if (rightType->isIntegerTy(1)) {
        llvm::Type* i32 = llvm::Type::getInt32Ty(context);
        rightVal = builder.CreateZExtOrTrunc(rightVal, i32, "booltoi32");
    }
    
    switch (expr->op) {
        case BinaryOpType::ADD:
            return builder.CreateAdd(leftVal, rightVal, "addtmp");
        case BinaryOpType::SUB:
            return builder.CreateSub(leftVal, rightVal, "subtmp");
        case BinaryOpType::MUL:
            return builder.CreateMul(leftVal, rightVal, "multmp");
        case BinaryOpType::DIV:
            return builder.CreateSDiv(leftVal, rightVal, "divtmp");
        case BinaryOpType::MOD:
            return builder.CreateSRem(leftVal, rightVal, "modtmp");
        case BinaryOpType::EQUAL:
            return builder.CreateICmpEQ(leftVal, rightVal, "eqtmp");
        case BinaryOpType::NOT_EQUAL:
            return builder.CreateICmpNE(leftVal, rightVal, "netmp");
        case BinaryOpType::LESS:
            return builder.CreateICmpSLT(leftVal, rightVal, "lttmp");
        case BinaryOpType::LESS_EQUAL:
            return builder.CreateICmpSLE(leftVal, rightVal, "letmp");
        case BinaryOpType::GREATER:
            return builder.CreateICmpSGT(leftVal, rightVal, "gttmp");
        case BinaryOpType::GREATER_EQUAL:
            return builder.CreateICmpSGE(leftVal, rightVal, "getmp");
        case BinaryOpType::AND:
            return builder.CreateAnd(leftVal, rightVal, "andtmp");
        case BinaryOpType::OR:
            return builder.CreateOr(leftVal, rightVal, "ortmp");
        default:
            return nullptr;
    }
}

llvm::Value* LLVMCodegen::codegen(CallExpression* expr) {
    std::string funcName = expr->ns.empty() ? expr->name : (expr->ns + ":" + expr->name);
    llvm::Function* callee = module->getFunction(funcName);
    if (!callee) {
        callee = module->getFunction(expr->name);
    }
    if (!callee) {
        addError("Unknown function: " + funcName);
        return nullptr;
    }
    
    std::vector<llvm::Value*> args;
    for (auto& arg : expr->args) {
        llvm::Value* argVal = codegen(arg.get());
        if (!argVal) return nullptr;
        
        if (argVal->getType()->isIntegerTy(1)) {
            argVal = builder.CreateZExtOrTrunc(argVal, llvm::Type::getInt32Ty(context), "argbool");
        }
        
        args.push_back(argVal);
    }
    
    llvm::CallInst* callInst = builder.CreateCall(callee->getFunctionType(), callee, args);
    
    return callInst;
}

llvm::Value* LLVMCodegen::codegen(ArrayRangeExpression* expr) {
    llvm::Value* startVal = codegen(expr->start.get());
    if (!startVal) return nullptr;
    
    llvm::Value* endVal = codegen(expr->end.get());
    if (!endVal) return nullptr;
    
    llvm::Function* func = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* entryBB = builder.GetInsertBlock();
    
    llvm::BasicBlock* randBB = llvm::BasicBlock::Create(context, "rand.block", func);
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(context, "rand.merge", func);
    
    llvm::Value* isRandom = builder.CreateICmpEQ(
        builder.CreateGlobalString(expr->accessType, "access.type"),
        builder.CreateGlobalString("rnd", "rnd.str")
    );
    
    builder.CreateCondBr(isRandom, randBB, mergeBB);
    
    builder.SetInsertPoint(randBB);
    llvm::Function* randFunc = module->getFunction("rand");
    if (!randFunc) {
        llvm::FunctionType* randType = llvm::FunctionType::get(builder.getInt32Ty(), false);
        randFunc = llvm::Function::Create(randType, llvm::Function::ExternalLinkage, 0, "rand", module.get());
    }
    
    llvm::Value* randVal = builder.CreateCall(randFunc);
    llvm::Value* range = builder.CreateSub(endVal, startVal);
    range = builder.CreateAdd(range, createConstInt(context, builder.getInt32Ty(), 1));
    llvm::Value* modVal = builder.CreateSRem(randVal, range);
    llvm::Value* result = builder.CreateAdd(modVal, startVal);
    builder.CreateBr(mergeBB);
    
    builder.SetInsertPoint(mergeBB);
    llvm::PHINode* phi = builder.CreatePHI(builder.getInt32Ty(), 2, "range.result");
    phi->addIncoming(result, randBB);
    phi->addIncoming(startVal, entryBB);
    
    return phi;
}

llvm::Value* LLVMCodegen::codegen(ExpressionStatement* stmt) {
    return codegen(stmt->expr.get());
}

llvm::Value* LLVMCodegen::codegen(AssignmentStatement* stmt) {
    llvm::Value* value = codegen(stmt->value.get());
    if (!value) return nullptr;
    
    llvm::AllocaInst* alloca = nullptr;
    auto it = locals.find(stmt->name);
    if (it == locals.end()) {
        llvm::Function* func = builder.GetInsertBlock()->getParent();
        llvm::IRBuilderBase::InsertPoint ip = builder.saveIP();
        
        llvm::BasicBlock* entryBB = &func->getEntryBlock();
        if (entryBB->empty()) {
            builder.SetInsertPoint(entryBB);
        } else {
            builder.SetInsertPoint(entryBB, entryBB->getFirstInsertionPt());
        }
        
        llvm::Type* valueType = value->getType();
        if (valueType->isIntegerTy(1)) {
            valueType = llvm::Type::getInt32Ty(context);
        }
        
        alloca = builder.CreateAlloca(valueType, nullptr, stmt->name.c_str());
        builder.restoreIP(ip);
        
        locals[stmt->name] = alloca;
    } else {
        alloca = it->second;
    }
    
    llvm::Value* storeVal = value;
    if (storeVal->getType()->isIntegerTy(1)) {
        storeVal = builder.CreateZExtOrTrunc(storeVal, llvm::Type::getInt32Ty(context), "boolstore");
    }
    
    builder.CreateStore(storeVal, alloca);
    return value;
}

llvm::Value* LLVMCodegen::codegen(PrintStatement* stmt) {
    llvm::Value* arg = codegen(stmt->expr.get());
    if (!arg) return nullptr;
    
    llvm::Function* printfFunc = module->getFunction("printf");
    if (printfFunc) {
        llvm::Value* formatStr;
        if (arg->getType()->isIntegerTy()) {
            formatStr = builder.CreateGlobalString("%d\n", "format");
        } else if (arg->getType()->isPointerTy()) {
            formatStr = builder.CreateGlobalString("%s\n", "format");
        } else {
            formatStr = builder.CreateGlobalString("%d\n", "format");
        }
        llvm::Value* formatPtr = builder.CreateBitCast(formatStr, builder.getInt8Ty()->getPointerTo(), "fmtcast");
        return builder.CreateCall(printfFunc->getFunctionType(), printfFunc, {formatPtr, arg}, "printfcall");
    }
    
    return arg;
}

llvm::Value* LLVMCodegen::codegen(ReturnStatement* stmt) {
    llvm::Value* value = codegen(stmt->value.get());
    if (!value) return nullptr;
    
    if (value->getType()->isIntegerTy(1)) {
        value = builder.CreateZExtOrTrunc(value, llvm::Type::getInt32Ty(context), "retbool");
    }
    
    builder.CreateRet(value);
    return value;
}

llvm::Value* LLVMCodegen::codegen(BreakStatement* stmt) {
    if (loopEndBB) {
        builder.CreateBr(loopEndBB);
    }
    return nullptr;
}

llvm::Value* LLVMCodegen::codegen(BlockStatement* stmt) {
    for (auto& s : stmt->statements) {
        codegen(s.get());
    }
    return nullptr;
}

llvm::Value* LLVMCodegen::codegen(IfStatement* stmt) {
    llvm::Function* func = builder.GetInsertBlock()->getParent();
    
    llvm::Value* CondVal = codegen(stmt->condition.get());
    if (!CondVal) return nullptr;
    
    if (!CondVal->getType()->isIntegerTy(1)) {
        llvm::IntegerType* intType = llvm::cast<llvm::IntegerType>(CondVal->getType());
        llvm::Value* zero = createConstInt(context, intType, 0);
        CondVal = builder.CreateICmpNE(CondVal, zero, "condcmp");
    }
    
    llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(context, "then", func);
    llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(context, "ifcont", func);
    
    llvm::BasicBlock* NextBB = nullptr;
    if (stmt->elseIfBranches.empty() && !stmt->elseBranch) {
        NextBB = MergeBB;
    } else if (!stmt->elseIfBranches.empty()) {
        NextBB = llvm::BasicBlock::Create(context, "elseif.0", func);
    } else {
        NextBB = llvm::BasicBlock::Create(context, "else", func);
    }
    
    builder.CreateCondBr(CondVal, ThenBB, NextBB);
    
    builder.SetInsertPoint(ThenBB);
    codegen(stmt->thenBranch.get());
    
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(MergeBB);
    }
    
    builder.SetInsertPoint(NextBB);
    
    for (size_t i = 0; i < stmt->elseIfBranches.size(); i++) {
        auto& elseIfBranch = stmt->elseIfBranches[i];
        
        llvm::Value* elseIfCondVal = codegen(elseIfBranch.first.get());
        if (!elseIfCondVal) return nullptr;
        
        if (!elseIfCondVal->getType()->isIntegerTy(1)) {
            llvm::IntegerType* intType = llvm::cast<llvm::IntegerType>(elseIfCondVal->getType());
            llvm::Value* zero = createConstInt(context, intType, 0);
            elseIfCondVal = builder.CreateICmpNE(elseIfCondVal, zero, "condcmp");
        }
        
        llvm::BasicBlock* ElseIfThenBB = llvm::BasicBlock::Create(context, ("elseif.then." + std::to_string(i)).c_str(), func);
        
        if (i == stmt->elseIfBranches.size() - 1) {
            if (stmt->elseBranch) {
                NextBB = llvm::BasicBlock::Create(context, "else", func);
            } else {
                NextBB = MergeBB;
            }
        } else {
            NextBB = llvm::BasicBlock::Create(context, ("elseif." + std::to_string(i + 1)).c_str(), func);
        }
        
        builder.CreateCondBr(elseIfCondVal, ElseIfThenBB, NextBB);
        
        builder.SetInsertPoint(ElseIfThenBB);
        codegen(elseIfBranch.second.get());
        
        if (!builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(MergeBB);
        }
        
        builder.SetInsertPoint(NextBB);
    }
    
    if (stmt->elseBranch) {
        codegen(stmt->elseBranch.get());
    }
    
    if (!builder.GetInsertBlock()->getTerminator()) {
        if (builder.GetInsertBlock() != MergeBB) {
            builder.CreateBr(MergeBB);
        }
    }
    
    if (MergeBB->empty() && !MergeBB->getTerminator()) {
        builder.SetInsertPoint(MergeBB);
    } else if (!MergeBB->getTerminator()) {
        builder.SetInsertPoint(MergeBB);
    }
    
    return nullptr;
}

llvm::Value* LLVMCodegen::codegen(WhileStatement* stmt) {
    llvm::Function* func = builder.GetInsertBlock()->getParent();
    
    llvm::BasicBlock* HeaderBB = llvm::BasicBlock::Create(context, "whileheader", func);
    llvm::BasicBlock* BodyBB = llvm::BasicBlock::Create(context, "whilebody");
    llvm::BasicBlock* ExitBB = llvm::BasicBlock::Create(context, "whileexit", func);
    
    builder.CreateBr(HeaderBB);
    builder.SetInsertPoint(HeaderBB);
    
    llvm::Value* CondVal = codegen(stmt->condition.get());
    if (!CondVal) return nullptr;
    
    if (CondVal->getType()->isIntegerTy(1)) {
        llvm::Value* i32Cond = builder.CreateZExtOrTrunc(CondVal, llvm::Type::getInt32Ty(context), "condtoi32");
        llvm::Value* zero = createConstInt(context, llvm::Type::getInt32Ty(context), 0);
        CondVal = builder.CreateICmpNE(i32Cond, zero, "condcmp");
    }
    
    llvm::BasicBlock* PrevLoopEndBB = loopEndBB;
    loopEndBB = ExitBB;
    
    builder.CreateCondBr(CondVal, BodyBB, ExitBB);
    
    func->insert(func->end(), BodyBB);
    builder.SetInsertPoint(BodyBB);
    
    codegen(stmt->body.get());
    
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(HeaderBB);
    }
    
    loopEndBB = PrevLoopEndBB;
    
    builder.SetInsertPoint(ExitBB);
    
    return nullptr;
}

bool LLVMCodegen::codegenProgram(Program* program) {
    for (auto& imp : program->imports) {
        if (!codegen(imp.get())) {
            return false;
        }
    }
    
    for (auto& mod : program->modules) {
        for (auto& func : mod->functions) {
            std::vector<llvm::Type*> paramTypes;
            for (size_t i = 0; i < func->params.size(); i++) {
                paramTypes.push_back(llvm::Type::getInt32Ty(context));
            }
            
            bool isMain = (func->name == "main");
            llvm::FunctionType* funcType;
            if (isMain) {
                funcType = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), paramTypes, false);
            } else {
                funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), paramTypes, false);
            }
            
            llvm::Function* llvmFunc = llvm::Function::Create(funcType, llvm::Function::LinkageTypes::ExternalLinkage, 0, func->name, module.get());
            
            if (isMain) {
                hasMainFunction = true;
            }
        }
    }
    
    for (auto& mod : program->modules) {
        if (!codegen(mod.get())) {
            return false;
        }
    }
    
    return true;
}

bool LLVMCodegen::codegen(Module* mod) {
    for (auto& func : mod->functions) {
        if (!codegen(func.get())) {
            return false;
        }
    }
    return true;
}

bool LLVMCodegen::codegen(Function* func) {
    locals.clear();
    
    std::string funcName = func->ns.empty() ? func->name : (func->ns + ":" + func->name);
    llvm::Function* llvmFunc = module->getFunction(funcName);
    if (!llvmFunc) {
        std::vector<llvm::Type*> paramTypes;
        for (size_t i = 0; i < func->params.size(); i++) {
            paramTypes.push_back(llvm::Type::getInt32Ty(context));
        }
        
        bool isMain = (func->name == "main" && func->ns.empty());
        llvm::FunctionType* funcType;
        if (isMain) {
            funcType = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), paramTypes, false);
        } else {
            funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), paramTypes, false);
        }
        
        llvmFunc = llvm::Function::Create(funcType, llvm::Function::LinkageTypes::ExternalLinkage, 0, funcName, module.get());
        
        if (isMain) {
            hasMainFunction = true;
        }
    }
    
    if (llvmFunc->empty()) {
        llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", llvmFunc);
        builder.SetInsertPoint(entryBB);
        
        size_t i = 0;
        for (auto& arg : llvmFunc->args()) {
            std::string paramName = func->params[i]->name;
            llvm::AllocaInst* alloca = builder.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr, paramName.c_str());
            builder.CreateStore(&arg, alloca);
            locals[paramName] = alloca;
            i++;
        }
        
        if (func->body) {
            codegen(func->body.get());
        }
        
        if (builder.GetInsertBlock() && !builder.GetInsertBlock()->getTerminator()) {
            if (func->name == "main" && func->ns.empty()) {
                builder.CreateRet(createConstInt(context, llvm::Type::getInt32Ty(context), 0));
            } else {
                builder.CreateRetVoid();
            }
        }
    }
    
    return true;
}

bool LLVMCodegen::codegen(Statement* stmt) {
    if (dynamic_cast<AssignmentStatement*>(stmt)) return codegen(dynamic_cast<AssignmentStatement*>(stmt)) != nullptr;
    if (dynamic_cast<ExpressionStatement*>(stmt)) return codegen(dynamic_cast<ExpressionStatement*>(stmt)) != nullptr;
    if (dynamic_cast<PrintStatement*>(stmt)) return codegen(dynamic_cast<PrintStatement*>(stmt)) != nullptr;
    if (dynamic_cast<ReturnStatement*>(stmt)) return codegen(dynamic_cast<ReturnStatement*>(stmt)) != nullptr;
    if (dynamic_cast<BreakStatement*>(stmt)) return codegen(dynamic_cast<BreakStatement*>(stmt)) != nullptr;
    if (dynamic_cast<BlockStatement*>(stmt)) return codegen(dynamic_cast<BlockStatement*>(stmt)) != nullptr;
    if (dynamic_cast<IfStatement*>(stmt)) return codegen(dynamic_cast<IfStatement*>(stmt)) != nullptr;
    if (dynamic_cast<WhileStatement*>(stmt)) return codegen(dynamic_cast<WhileStatement*>(stmt)) != nullptr;
    if (dynamic_cast<ImportStatement*>(stmt)) return codegen(dynamic_cast<ImportStatement*>(stmt));
    if (auto* funcDecl = dynamic_cast<FunctionDeclarationStatement*>(stmt)) {
        if (funcDecl->func) {
            return codegen(funcDecl->func.get());
        }
        return true;
    }
    return false;
}

bool LLVMCodegen::codegen(ImportStatement* stmt) {
    return true;
}

llvm::Value* LLVMCodegen::codegen(Expression* expr) {
    if (auto* e = dynamic_cast<NumberLiteral*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<BooleanLiteral*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<StringLiteral*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<VariableExpression*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<UnaryOp*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<BinaryOp*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<CallExpression*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<ArrayRangeExpression*>(expr)) return codegen(e);
    return nullptr;
}

bool LLVMCodegen::emitObjectFile(const std::string& filename) {
    return emitObjectFile(filename, false, false, "", "");
}

bool LLVMCodegen::emitObjectFile(const std::string& filename, bool keepLL, bool emitAsm, 
                                  const std::string& llOutputPath, const std::string& asmOutputPath) {
    llvm::Triple triple = module->getTargetTriple();
    if (triple.getTriple().empty()) {
        triple = llvm::Triple("x86_64-pc-windows-msvc");
    }
    module->setTargetTriple(triple);
    
    std::string baseFile = filename;
    if (baseFile.length() > 2 && baseFile.substr(baseFile.length() - 2) == ".o") {
        baseFile = baseFile.substr(0, baseFile.length() - 2);
    }
    
    std::string llFile = llOutputPath.empty() ? (baseFile + ".ll") : llOutputPath;
    std::string objFile = filename;
    std::string asmFile = asmOutputPath.empty() ? (baseFile + ".asm") : asmOutputPath;
    
    std::error_code EC;
    llvm::raw_fd_ostream llOut(llFile, EC);
    if (EC) {
        addError("Could not open file: " + EC.message());
        return false;
    }
    module->print(llOut, nullptr);
    llOut.close();
    
    std::string cmd;
    
    if (emitAsm) {
        cmd = "clang -target x86_64-pc-windows-msvc -S \"" + llFile + "\" -o \"" + asmFile + "\"";
        int result = system(cmd.c_str());
        if (result != 0) {
            cmd = "clang -S \"" + llFile + "\" -o \"" + asmFile + "\"";
            result = system(cmd.c_str());
        }
        
        if (result != 0) {
            addError("ASM file generation failed");
            return false;
        }
        
        cmd = "clang -target x86_64-pc-windows-msvc -c \"" + asmFile + "\" -o \"" + objFile + "\"";
        result = system(cmd.c_str());
        
        if (result != 0) {
            cmd = "clang -c \"" + asmFile + "\" -o \"" + objFile + "\"";
            result = system(cmd.c_str());
        }
        
        if (result != 0) {
            addError("Object file generation from ASM failed");
            return false;
        }
        
        if (!keepLL) {
            std::remove(llFile.c_str());
        }
        
        return true;
    }
    
    cmd = "clang -target x86_64-pc-windows-msvc -c \"" + llFile + "\" -o \"" + objFile + "\"";
    int result = system(cmd.c_str());
    
    if (result != 0) {
        cmd = "clang -c \"" + llFile + "\" -o \"" + objFile + "\"";
        result = system(cmd.c_str());
    }
    
    if (result != 0) {
        addError("Object file generation failed");
        return false;
    }
    
    if (!keepLL) {
        std::remove(llFile.c_str());
    }
    
    return true;
}

bool LLVMCodegen::linkExecutable(const std::string& objFile, const std::string& outFile) {
    std::string cmd = "lld-link /OUT:\"" + outFile + "\" /SUBSYSTEM:CONSOLE /STACK:1048576 /ENTRY:mainCRTStartup /NODEFAULTLIB:msvcrt /DEFAULTLIB:libcmt /DEFAULTLIB:oldnames \"" + objFile + "\"";
    
    int result = system(cmd.c_str());
    
    if (result != 0) {
        addError("Linking failed with exit code " + std::to_string(result));
        return false;
    }
    
    return true;
}

bool LLVMCodegen::verifyModule() {
    std::string out;
    llvm::raw_string_ostream oss(out);
    if (llvm::verifyModule(*module, &oss)) {
        addError(oss.str());
        return false;
    }
    return true;
}

}

