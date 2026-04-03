#include "xfawa_llvm_codegen.h"
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <sstream>
#include <llvm/ADT/APInt.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/ADCE.h>
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Scalar/EarlyCSE.h>
#include <llvm/Transforms/Scalar/SCCP.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/LoopSimplify.h>
#include <llvm/Passes/PassBuilder.h>
#include <lld/Common/Driver.h>

LLD_HAS_DRIVER(coff)

namespace xfawa {

static llvm::ConstantInt* createConstInt(llvm::LLVMContext& ctx, llvm::IntegerType* ty, int64_t value) {
    return static_cast<llvm::ConstantInt*>(llvm::ConstantInt::get(ty, llvm::APInt(ty->getBitWidth(), value, true)));
}

namespace {

llvm::CodeGenOptLevel toCodeGenOptLevel(xfawa::OptimizationLevel level) {
    switch (level) {
        case xfawa::OptimizationLevel::O0:
            return llvm::CodeGenOptLevel::None;
        case xfawa::OptimizationLevel::O1:
            return llvm::CodeGenOptLevel::Less;
        case xfawa::OptimizationLevel::O2:
            return llvm::CodeGenOptLevel::Default;
        case xfawa::OptimizationLevel::O3:
            return llvm::CodeGenOptLevel::Aggressive;
    }

    return llvm::CodeGenOptLevel::Default;
}

std::unique_ptr<llvm::TargetMachine> createTargetMachine(llvm::Module& module, xfawa::OptimizationLevel optLevel, std::vector<std::string>& errors) {
    llvm::Triple triple = module.getTargetTriple();
    if (triple.str().empty()) {
        triple = llvm::Triple(llvm::sys::getDefaultTargetTriple());
        module.setTargetTriple(triple);
    }

    std::string err;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, err);
    if (!target) {
        errors.push_back("Unable to find target for triple '" + triple.str() + "': " + err);
        return nullptr;
    }

    llvm::TargetOptions options;
    std::optional<llvm::Reloc::Model> relocModel = llvm::Reloc::PIC_;
    auto targetMachine = std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(triple, "generic", "", options, relocModel, std::nullopt, toCodeGenOptLevel(optLevel)));

    if (!targetMachine) {
        errors.push_back("Unable to create LLVM target machine for triple '" + triple.str() + "'");
        return nullptr;
    }

    module.setDataLayout(targetMachine->createDataLayout());
    return targetMachine;
}

bool emitMachineCode(llvm::Module& module, llvm::TargetMachine& targetMachine,
                     const std::string& path, llvm::CodeGenFileType fileType,
                     std::vector<std::string>& errors) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(path, ec, llvm::sys::fs::OF_None);
    if (ec) {
        errors.push_back("Unable to open output file '" + path + "': " + ec.message());
        return false;
    }

    llvm::legacy::PassManager passManager;
    if (targetMachine.addPassesToEmitFile(passManager, dest, nullptr, fileType)) {
        errors.push_back("LLVM target cannot emit requested file type for '" + path + "'");
        return false;
    }

    passManager.run(module);
    dest.flush();
    return true;
}

std::optional<std::filesystem::path> findLatestVersionDir(const std::filesystem::path& root) {
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
        return std::nullopt;
    }

    std::optional<std::filesystem::path> best;
    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_directory()) {
            continue;
        }

        if (!best || entry.path().filename().string() > best->filename().string()) {
            best = entry.path();
        }
    }

    return best;
}

std::optional<std::filesystem::path> getEnvPath(const char* name) {
    const char* value = std::getenv(name);
    if (!value || *value == '\0') {
        return std::nullopt;
    }

    return std::filesystem::path(value);
}

bool appendLibPath(std::vector<std::string>& args, const std::filesystem::path& path) {
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        return false;
    }

    args.push_back("/libpath:" + path.string());
    return true;
}

} // namespace

LLVMCodegen::LLVMCodegen(llvm::LLVMContext& ctx, llvm::Module* mod) 
    : context(ctx), module(mod), builder(ctx), hasMainFunction(false), hasWindowStatements(false), loopEndBB(nullptr), optLevel(OptimizationLevel::O2), generatedWindowCount(0) {
    initBuiltins();
}

LLVMCodegen::LLVMCodegen(llvm::LLVMContext& ctx, llvm::Module* mod, OptimizationLevel opt)
    : context(ctx), module(mod), builder(ctx), hasMainFunction(false), hasWindowStatements(false), loopEndBB(nullptr), optLevel(opt), generatedWindowCount(0) {
    initBuiltins();
}

void LLVMCodegen::initBuiltins() {
    auto declareFunction = [this](const std::string& name, llvm::FunctionType* type) -> llvm::Function* {
        if (llvm::Function* existing = module->getFunction(name)) {
            return existing;
        }
        return llvm::Function::Create(type, llvm::Function::LinkageTypes::ExternalLinkage, 0, name, module);
    };

    llvm::Type* ptrTy = builder.getInt8Ty()->getPointerTo();

    llvm::FunctionType* printfType = llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy}, true);
    llvm::Function* printfFunc = declareFunction("printf", printfType);
    printfFunc->setCallingConv(llvm::CallingConv::C);

    llvm::FunctionType* mallocType = llvm::FunctionType::get(ptrTy, {builder.getInt64Ty()}, false);
    declareFunction("malloc", mallocType);

    llvm::FunctionType* randType = llvm::FunctionType::get(builder.getInt32Ty(), false);
    declareFunction("rand", randType);

    llvm::FunctionType* srandType = llvm::FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty()}, false);
    declareFunction("srand", srandType);

    llvm::FunctionType* timeType = llvm::FunctionType::get(builder.getInt64Ty(), {builder.getInt64Ty()->getPointerTo()}, false);
    declareFunction("time", timeType);

    llvm::FunctionType* clockType = llvm::FunctionType::get(builder.getInt64Ty(), false);
    declareFunction("clock", clockType);

    declareFunction("GetModuleHandleA", llvm::FunctionType::get(ptrTy, {ptrTy}, false));
    declareFunction("LoadCursorA", llvm::FunctionType::get(ptrTy, {ptrTy, ptrTy}, false));
    declareFunction("RegisterClassA", llvm::FunctionType::get(builder.getInt16Ty(), {ptrTy}, false));
    declareFunction("CreateWindowExA", llvm::FunctionType::get(
        ptrTy,
        {builder.getInt32Ty(), ptrTy, ptrTy, builder.getInt32Ty(), builder.getInt32Ty(),
         builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), ptrTy, ptrTy, ptrTy, ptrTy},
        false));
    declareFunction("ShowWindow", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy, builder.getInt32Ty()}, false));
    declareFunction("UpdateWindow", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy}, false));
    declareFunction("GetMessageA", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy, ptrTy, builder.getInt32Ty(), builder.getInt32Ty()}, false));
    declareFunction("TranslateMessage", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy}, false));
    declareFunction("DispatchMessageA", llvm::FunctionType::get(builder.getInt64Ty(), {ptrTy}, false));
    declareFunction("DefWindowProcA", llvm::FunctionType::get(builder.getInt64Ty(), {ptrTy, builder.getInt32Ty(), builder.getInt64Ty(), builder.getInt64Ty()}, false));
    declareFunction("PostQuitMessage", llvm::FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty()}, false));
    declareFunction("BeginPaint", llvm::FunctionType::get(ptrTy, {ptrTy, ptrTy}, false));
    declareFunction("EndPaint", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy, ptrTy}, false));
    declareFunction("CreateSolidBrush", llvm::FunctionType::get(ptrTy, {builder.getInt32Ty()}, false));
    declareFunction("FillRect", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy, ptrTy, ptrTy}, false));
    declareFunction("DeleteObject", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy}, false));
}

llvm::Type* LLVMCodegen::getLLVMType(VarType type) {
    switch (type) {
        case VarType::INT:
            return builder.getInt32Ty();
        case VarType::FLOAT:
            return builder.getFloatTy();
        case VarType::BOOL:
            return builder.getInt1Ty();
        case VarType::ARRAY_INT:
            return llvm::PointerType::get(builder.getInt32Ty(), 0);
        case VarType::ARRAY_FLOAT:
            return llvm::PointerType::get(builder.getFloatTy(), 0);
        case VarType::ARRAY_BOOL:
            return llvm::PointerType::get(builder.getInt1Ty(), 0);
        default:
            return builder.getInt32Ty();
    }
}

llvm::Type* LLVMCodegen::getArrayElementType(VarType type) {
    switch (type) {
        case VarType::ARRAY_INT:
            return builder.getInt32Ty();
        case VarType::ARRAY_FLOAT:
            return builder.getFloatTy();
        case VarType::ARRAY_BOOL:
            return builder.getInt1Ty();
        default:
            return builder.getInt32Ty();
    }
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

llvm::Value* LLVMCodegen::codegen(FloatLiteral* expr) {
    llvm::APFloat apFloat((float)expr->value);
    return llvm::ConstantFP::get(builder.getFloatTy(), apFloat);
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
    if (expr->name == "rnd") {
        if (expr->args.size() == 1) {
            llvm::Value* arrVal = codegen(expr->args[0].get());
            if (!arrVal) return nullptr;
            
            int64_t knownArrayLen = 0;
            if (auto* varExpr = dynamic_cast<VariableExpression*>(expr->args[0].get())) {
                auto lenIt = arrayLengths.find(varExpr->name);
                if (lenIt != arrayLengths.end()) {
                    knownArrayLen = lenIt->second;
                }
            }
            
            llvm::Function* randFunc = module->getFunction("rand");
            llvm::Value* randVal = builder.CreateCall(randFunc, {}, "rand.val");
            
            llvm::Value* arrLen;
            if (knownArrayLen > 0) {
                arrLen = createConstInt(context, builder.getInt32Ty(), knownArrayLen);
            } else {
                addError("rnd(array): array length must be known at compile time");
                return nullptr;
            }
            
            llvm::Value* modVal = builder.CreateSRem(randVal, arrLen, "rand.idx");
            llvm::Value* idxExt = builder.CreateSExt(modVal, builder.getInt64Ty(), "idx.ext");
            llvm::Value* elemPtr = builder.CreateGEP(builder.getInt32Ty(), arrVal, idxExt, "rnd.elem.ptr");
            return builder.CreateLoad(builder.getInt32Ty(), elemPtr, "rnd.elem");
        }
        else if (expr->args.size() == 2) {
            llvm::Value* minVal = codegen(expr->args[0].get());
            if (!minVal) return nullptr;
            
            llvm::Value* maxVal = codegen(expr->args[1].get());
            if (!maxVal) return nullptr;
            
            llvm::Function* randFunc = module->getFunction("rand");
            llvm::Value* randVal = builder.CreateCall(randFunc, {}, "rand.val");
            
            llvm::Value* range = builder.CreateSub(maxVal, minVal, "range.sub");
            range = builder.CreateAdd(range, createConstInt(context, builder.getInt32Ty(), 1), "range.size");
            llvm::Value* modVal = builder.CreateSRem(randVal, range, "rand.mod");
            return builder.CreateAdd(modVal, minVal, "rnd.result");
        }
        else {
            addError("rnd() requires 1 (array) or 2 (min, max) arguments");
            return nullptr;
        }
    }
    
    std::string funcName = expr->ns.empty() ? expr->name : (expr->ns + ":" + expr->name);
    llvm::Function* callee = module->getFunction(funcName);
    if (!callee) {
        callee = module->getFunction(expr->name);
    }
    if (!callee && expr->ns.empty()) {
        for (auto it = module->begin(); it != module->end(); ++it) {
            std::string fname = it->getName().str();
            size_t colonPos = fname.find(':');
            if (colonPos != std::string::npos) {
                std::string pureName = fname.substr(colonPos + 1);
                if (pureName == expr->name) {
                    callee = &(*it);
                    break;
                }
            }
        }
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
        randFunc = llvm::Function::Create(randType, llvm::Function::ExternalLinkage, 0, "rand", module);
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

llvm::Value* LLVMCodegen::codegen(ArrayLiteral* expr) {
    if (expr->isRange) {
        llvm::Value* startVal = codegen(expr->rangeStart.get());
        if (!startVal) return nullptr;
        
        llvm::Value* endVal = codegen(expr->rangeEnd.get());
        if (!endVal) return nullptr;
        
        if (auto* startInt = llvm::dyn_cast<llvm::ConstantInt>(startVal)) {
            if (auto* endInt = llvm::dyn_cast<llvm::ConstantInt>(endVal)) {
                int64_t start = startInt->getSExtValue();
                int64_t end = endInt->getSExtValue();
                int64_t size = end - start + 1;
                
                if (size > 0 && size < 10000) {
                    llvm::Function* mallocFunc = module->getFunction("malloc");
                    llvm::Value* allocSize = createConstInt(context, builder.getInt64Ty(), size * 4);
                    llvm::Value* rawPtr = builder.CreateCall(mallocFunc, {allocSize}, "arr.alloc");
                    llvm::Value* arrPtr = builder.CreateBitCast(rawPtr, llvm::PointerType::get(builder.getInt32Ty(), 0), "arr.ptr");
                    
                    for (int64_t i = 0; i < size; i++) {
                        llvm::Value* idx = createConstInt(context, builder.getInt64Ty(), i);
                        llvm::Value* elemPtr = builder.CreateGEP(builder.getInt32Ty(), arrPtr, idx, "elem.ptr");
                        llvm::Value* val = createConstInt(context, builder.getInt32Ty(), start + i);
                        builder.CreateStore(val, elemPtr);
                    }
                    
                    return arrPtr;
                }
            }
        }
        
        addError("Range array must have constant integer bounds");
        return nullptr;
    }
    
    size_t size = expr->elements.size();
    if (size == 0) {
        return llvm::ConstantPointerNull::get(llvm::PointerType::get(builder.getInt32Ty(), 0));
    }
    
    llvm::Function* mallocFunc = module->getFunction("malloc");
    llvm::Value* allocSize = createConstInt(context, builder.getInt64Ty(), size * 4);
    llvm::Value* rawPtr = builder.CreateCall(mallocFunc, {allocSize}, "arr.alloc");
    llvm::Value* arrPtr = builder.CreateBitCast(rawPtr, llvm::PointerType::get(builder.getInt32Ty(), 0), "arr.ptr");
    
    for (size_t i = 0; i < size; i++) {
        llvm::Value* elemVal = codegen(expr->elements[i].get());
        if (!elemVal) return nullptr;
        
        if (elemVal->getType()->isIntegerTy(1)) {
            elemVal = builder.CreateZExtOrTrunc(elemVal, builder.getInt32Ty(), "boolext");
        }
        
        llvm::Value* idx = createConstInt(context, builder.getInt64Ty(), i);
        llvm::Value* elemPtr = builder.CreateGEP(builder.getInt32Ty(), arrPtr, idx, "elem.ptr");
        builder.CreateStore(elemVal, elemPtr);
    }
    
    return arrPtr;
}

llvm::Value* LLVMCodegen::codegen(ArrayIndexExpression* expr) {
    llvm::Value* arrVal = codegen(expr->array.get());
    if (!arrVal) return nullptr;
    
    llvm::Value* idxVal = codegen(expr->index.get());
    if (!idxVal) return nullptr;
    
    int64_t knownArrayLen = 0;
    if (auto* varExpr = dynamic_cast<VariableExpression*>(expr->array.get())) {
        auto lenIt = arrayLengths.find(varExpr->name);
        if (lenIt != arrayLengths.end()) {
            knownArrayLen = lenIt->second;
        }
    }
    
    llvm::Value* finalIdx = idxVal;
    
    if (auto* constIdx = llvm::dyn_cast<llvm::ConstantInt>(idxVal)) {
        int64_t idx = constIdx->getSExtValue();
        if (idx < 0 && knownArrayLen > 0) {
            int64_t adjustedIdx = knownArrayLen + idx;
            finalIdx = createConstInt(context, builder.getInt64Ty(), adjustedIdx);
        } else if (idx < 0) {
            addWarning("Negative array index without known array length");
        }
    } else {
        if (knownArrayLen > 0) {
            llvm::Value* negCheck = builder.CreateICmpSLT(idxVal, createConstInt(context, builder.getInt32Ty(), 0), "neg.check");
            llvm::Value* adjustedIdx = builder.CreateAdd(
                createConstInt(context, builder.getInt32Ty(), knownArrayLen),
                idxVal,
                "adj.idx"
            );
            finalIdx = builder.CreateSelect(negCheck, adjustedIdx, idxVal, "final.idx");
        }
    }
    
    llvm::Value* idxExt = finalIdx;
    if (finalIdx->getType()->isIntegerTy(32)) {
        idxExt = builder.CreateSExt(finalIdx, builder.getInt64Ty(), "idx.ext");
    }
    
    llvm::Value* elemPtr = builder.CreateGEP(builder.getInt32Ty(), arrVal, idxExt, "arr.elem.ptr");
    return builder.CreateLoad(builder.getInt32Ty(), elemPtr, "arr.elem");
}

llvm::Value* LLVMCodegen::codegen(ExpressionStatement* stmt) {
    return codegen(stmt->expr.get());
}

llvm::Value* LLVMCodegen::codegen(AssignmentStatement* stmt) {
    int64_t arrayLen = 0;
    if (auto* arrLit = dynamic_cast<ArrayLiteral*>(stmt->value.get())) {
        if (arrLit->isRange) {
            if (auto* startInt = dynamic_cast<NumberLiteral*>(arrLit->rangeStart.get())) {
                if (auto* endInt = dynamic_cast<NumberLiteral*>(arrLit->rangeEnd.get())) {
                    arrayLen = endInt->value - startInt->value + 1;
                }
            }
        } else {
            arrayLen = arrLit->elements.size();
        }
    }
    
    llvm::Value* value = codegen(stmt->value.get());
    if (!value) return nullptr;
    
    llvm::AllocaInst* alloca = nullptr;
    auto it = locals.find(stmt->name);
    
    if (stmt->isReassignment) {
        if (it == locals.end()) {
            addError("Variable '" + stmt->name + "' used before declaration");
            return nullptr;
        }
        alloca = it->second;
    } else {
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
            VarType actualType = stmt->declaredType;
            
            if (stmt->hasExplicitType) {
                if (valueType->isPointerTy()) {
                    if (stmt->declaredType == VarType::INT) {
                        actualType = VarType::ARRAY_INT;
                    } else if (stmt->declaredType == VarType::FLOAT) {
                        actualType = VarType::ARRAY_FLOAT;
                    } else if (stmt->declaredType == VarType::BOOL) {
                        actualType = VarType::ARRAY_BOOL;
                    }
                }
                valueType = getLLVMType(actualType);
                localTypes[stmt->name] = actualType;
            } else {
                if (valueType->isIntegerTy(1)) {
                    valueType = llvm::Type::getInt32Ty(context);
                    localTypes[stmt->name] = VarType::INT;
                } else if (valueType->isFloatTy()) {
                    localTypes[stmt->name] = VarType::FLOAT;
                } else if (valueType->isPointerTy()) {
                    localTypes[stmt->name] = VarType::ARRAY_INT;
                } else {
                    localTypes[stmt->name] = VarType::INT;
                }
            }
            
            alloca = builder.CreateAlloca(valueType, nullptr, stmt->name.c_str());
            builder.restoreIP(ip);
            
            locals[stmt->name] = alloca;
        } else {
            alloca = it->second;
        }
    }
    
    if (arrayLen > 0) {
        arrayLengths[stmt->name] = arrayLen;
    }
    
    llvm::Value* storeVal = value;
    llvm::Type* allocaType = alloca->getAllocatedType();
    
    if (storeVal->getType()->isIntegerTy(1) && allocaType->isIntegerTy(32)) {
        storeVal = builder.CreateZExtOrTrunc(storeVal, llvm::Type::getInt32Ty(context), "boolstore");
    } else if (storeVal->getType()->isIntegerTy(32) && allocaType->isIntegerTy(1)) {
        storeVal = builder.CreateICmpNE(storeVal, createConstInt(context, builder.getInt32Ty(), 0), "inttobool");
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
        llvm::Value* printArg = arg;
        
        if (arg->getType()->isFloatTy()) {
            formatStr = builder.CreateGlobalString("%f\n", "format");
            printArg = builder.CreateFPExt(arg, builder.getDoubleTy(), "float.ext");
        } else if (arg->getType()->isIntegerTy(1)) {
            formatStr = builder.CreateGlobalString("%d\n", "format");
            printArg = builder.CreateZExtOrTrunc(arg, builder.getInt32Ty(), "bool.ext");
        } else if (arg->getType()->isIntegerTy()) {
            formatStr = builder.CreateGlobalString("%d\n", "format");
        } else if (arg->getType()->isPointerTy()) {
            formatStr = builder.CreateGlobalString("%s\n", "format");
        } else {
            formatStr = builder.CreateGlobalString("%d\n", "format");
        }
        llvm::Value* formatPtr = builder.CreateBitCast(formatStr, builder.getInt8Ty()->getPointerTo(), "fmtcast");
        return builder.CreateCall(printfFunc->getFunctionType(), printfFunc, {formatPtr, printArg}, "printfcall");
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
    
    if (auto* constBool = llvm::dyn_cast<llvm::ConstantInt>(CondVal)) {
        if (constBool->isOne()) {
            builder.CreateBr(BodyBB);
            func->insert(func->end(), BodyBB);
            builder.SetInsertPoint(BodyBB);
            
            llvm::BasicBlock* PrevLoopEndBB = loopEndBB;
            loopEndBB = ExitBB;
            
            codegen(stmt->body.get());
            
            if (!builder.GetInsertBlock()->getTerminator()) {
                builder.CreateBr(BodyBB);
            }
            
            loopEndBB = PrevLoopEndBB;
            builder.SetInsertPoint(ExitBB);
            return nullptr;
        } else if (constBool->isZero()) {
            builder.CreateBr(ExitBB);
            builder.SetInsertPoint(ExitBB);
            return nullptr;
        }
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

llvm::Value* LLVMCodegen::codegen(ForInStatement* stmt) {
    llvm::Function* func = builder.GetInsertBlock()->getParent();
    
    int64_t arrayLen = 0;
    bool isRange = false;
    int64_t rangeStart = 0;
    
    if (auto* arrLit = dynamic_cast<ArrayLiteral*>(stmt->iterable.get())) {
        if (arrLit->isRange) {
            isRange = true;
            if (auto* startInt = dynamic_cast<NumberLiteral*>(arrLit->rangeStart.get())) {
                rangeStart = startInt->value;
                if (auto* endInt = dynamic_cast<NumberLiteral*>(arrLit->rangeEnd.get())) {
                    arrayLen = endInt->value - startInt->value + 1;
                }
            }
        } else {
            arrayLen = arrLit->elements.size();
        }
    } else if (auto* varExpr = dynamic_cast<VariableExpression*>(stmt->iterable.get())) {
        auto lenIt = arrayLengths.find(varExpr->name);
        if (lenIt != arrayLengths.end()) {
            arrayLen = lenIt->second;
        }
    }
    
    llvm::Value* iterable = codegen(stmt->iterable.get());
    if (!iterable) return nullptr;
    
    llvm::AllocaInst* idxAlloca = nullptr;
    llvm::AllocaInst* varAlloca = nullptr;
    llvm::AllocaInst* lenAlloca = nullptr;
    
    {
        llvm::IRBuilderBase::InsertPoint ip = builder.saveIP();
        llvm::BasicBlock* entryBB = &func->getEntryBlock();
        if (entryBB->empty()) {
            builder.SetInsertPoint(entryBB);
        } else {
            builder.SetInsertPoint(entryBB, entryBB->getFirstInsertionPt());
        }
        
        idxAlloca = builder.CreateAlloca(builder.getInt64Ty(), nullptr, "for.idx");
        varAlloca = builder.CreateAlloca(builder.getInt32Ty(), nullptr, stmt->varName.c_str());
        lenAlloca = builder.CreateAlloca(builder.getInt64Ty(), nullptr, "for.len");
        
        builder.restoreIP(ip);
    }
    
    builder.CreateStore(createConstInt(context, builder.getInt64Ty(), 0), idxAlloca);
    builder.CreateStore(createConstInt(context, builder.getInt64Ty(), arrayLen), lenAlloca);
    
    llvm::BasicBlock* CondBB = llvm::BasicBlock::Create(context, "for.cond", func);
    llvm::BasicBlock* BodyBB = llvm::BasicBlock::Create(context, "for.body", func);
    llvm::BasicBlock* ExitBB = llvm::BasicBlock::Create(context, "for.exit", func);
    
    builder.CreateBr(CondBB);
    
    builder.SetInsertPoint(CondBB);
    llvm::Value* curIdx = builder.CreateLoad(builder.getInt64Ty(), idxAlloca, "cur.idx");
    llvm::Value* arrLen = builder.CreateLoad(builder.getInt64Ty(), lenAlloca, "arr.len");
    llvm::Value* cond = builder.CreateICmpSLT(curIdx, arrLen, "for.cmp");
    
    llvm::BasicBlock* PrevLoopEndBB = loopEndBB;
    loopEndBB = ExitBB;
    
    builder.CreateCondBr(cond, BodyBB, ExitBB);
    
    builder.SetInsertPoint(BodyBB);
    
    llvm::Value* curIdxInBody = builder.CreateLoad(builder.getInt64Ty(), idxAlloca, "cur.idx.body");
    llvm::Value* elemPtr = builder.CreateGEP(builder.getInt32Ty(), iterable, curIdxInBody, "elem.ptr");
    llvm::Value* elemVal = builder.CreateLoad(builder.getInt32Ty(), elemPtr, "elem.val");
    builder.CreateStore(elemVal, varAlloca);
    
    locals[stmt->varName] = varAlloca;
    localTypes[stmt->varName] = VarType::INT;
    
    codegen(stmt->body.get());
    
    llvm::Value* curIdxForInc = builder.CreateLoad(builder.getInt64Ty(), idxAlloca, "cur.idx.inc");
    llvm::Value* nextIdx = builder.CreateAdd(curIdxForInc, createConstInt(context, builder.getInt64Ty(), 1), "next.idx");
    builder.CreateStore(nextIdx, idxAlloca);
    
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(CondBB);
    }
    
    loopEndBB = PrevLoopEndBB;
    
    builder.SetInsertPoint(ExitBB);
    
    locals.erase(stmt->varName);
    localTypes.erase(stmt->varName);
    
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
            
            llvm::Function* llvmFunc = llvm::Function::Create(funcType, llvm::Function::LinkageTypes::ExternalLinkage, 0, func->name, module);
            
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

    if (!hasMainFunction) {
        addError("No entry point found. Define main().");
        return false;
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
    localTypes.clear();
    
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
        
        llvmFunc = llvm::Function::Create(funcType, llvm::Function::LinkageTypes::ExternalLinkage, 0, funcName, module);
        
        if (isMain) {
            hasMainFunction = true;
        }
    }
    
    if (llvmFunc->empty()) {
        llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", llvmFunc);
        builder.SetInsertPoint(entryBB);
        
        bool isMain = (func->name == "main" && func->ns.empty());
        if (isMain) {
            llvm::Function* timeFunc = module->getFunction("time");
            llvm::Value* nullPtr = llvm::ConstantPointerNull::get(builder.getInt64Ty()->getPointerTo());
            llvm::Value* timeVal = builder.CreateCall(timeFunc, {nullPtr}, "time.val");
            llvm::Value* timeCast = builder.CreateTrunc(timeVal, builder.getInt32Ty(), "time.cast");
            
            llvm::Function* clockFunc = module->getFunction("clock");
            llvm::Value* clockVal = builder.CreateCall(clockFunc, {}, "clock.val");
            llvm::Value* clockCast = builder.CreateTrunc(clockVal, builder.getInt32Ty(), "clock.cast");
            
            llvm::Value* seed = builder.CreateXor(timeCast, clockCast, "seed");
            
            llvm::Function* srandFunc = module->getFunction("srand");
            builder.CreateCall(srandFunc, {seed});
        }
        
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

        if (isMain && hasWindowStatements && builder.GetInsertBlock() && !builder.GetInsertBlock()->getTerminator()) {
            llvm::Type* ptrTy = builder.getInt8Ty()->getPointerTo();
            llvm::Function* getMessageFunc = module->getFunction("GetMessageA");
            llvm::Function* translateMessageFunc = module->getFunction("TranslateMessage");
            llvm::Function* dispatchMessageFunc = module->getFunction("DispatchMessageA");

            llvm::BasicBlock* msgLoopCondBB = llvm::BasicBlock::Create(context, "msg_loop_cond", llvmFunc);
            llvm::BasicBlock* msgLoopBodyBB = llvm::BasicBlock::Create(context, "msg_loop_body", llvmFunc);
            llvm::BasicBlock* msgLoopExitBB = llvm::BasicBlock::Create(context, "msg_loop_exit", llvmFunc);

            llvm::AllocaInst* msgBuffer = builder.CreateAlloca(llvm::ArrayType::get(builder.getInt8Ty(), 48), nullptr, "msg");
            llvm::Value* nullPtr = llvm::Constant::getNullValue(ptrTy);
            builder.CreateBr(msgLoopCondBB);

            builder.SetInsertPoint(msgLoopCondBB);
            llvm::Value* getMessageResult = builder.CreateCall(getMessageFunc, {msgBuffer, nullPtr, builder.getInt32(0), builder.getInt32(0)}, "msg_result");
            llvm::Value* keepRunning = builder.CreateICmpSGT(getMessageResult, builder.getInt32(0), "keep_running");
            builder.CreateCondBr(keepRunning, msgLoopBodyBB, msgLoopExitBB);

            builder.SetInsertPoint(msgLoopBodyBB);
            builder.CreateCall(translateMessageFunc, {msgBuffer});
            builder.CreateCall(dispatchMessageFunc, {msgBuffer});
            builder.CreateBr(msgLoopCondBB);

            builder.SetInsertPoint(msgLoopExitBB);
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
    if (dynamic_cast<ForInStatement*>(stmt)) return codegen(dynamic_cast<ForInStatement*>(stmt)) != nullptr;
    if (dynamic_cast<WindowStatement*>(stmt)) return codegen(dynamic_cast<WindowStatement*>(stmt)) != nullptr;
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

uint32_t LLVMCodegen::resolveWindowColor(const std::string& colorName) const {
    std::string normalized;
    normalized.reserve(colorName.size());
    for (char ch : colorName) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    if (normalized == "red") return 0x000000FF;
    if (normalized == "green") return 0x0000FF00;
    if (normalized == "blue") return 0x00FF0000;
    if (normalized == "black") return 0x00000000;
    if (normalized == "white") return 0x00FFFFFF;
    if (normalized == "yellow") return 0x0000FFFF;
    if (normalized == "cyan") return 0x00FFFF00;
    if (normalized == "magenta") return 0x00FF00FF;
    if (normalized == "gray" || normalized == "grey") return 0x00808080;
    return 0x00FFFFFF;
}

llvm::GlobalVariable* LLVMCodegen::getWindowCountGlobal() {
    if (llvm::GlobalVariable* existing = module->getNamedGlobal("__xfawa_active_window_count")) {
        return existing;
    }

    return new llvm::GlobalVariable(
        *module,
        builder.getInt32Ty(),
        false,
        llvm::GlobalValue::InternalLinkage,
        builder.getInt32(0),
        "__xfawa_active_window_count");
}

llvm::Function* LLVMCodegen::createWindowProc(WindowStatement* windowDecl, int windowId) {
    llvm::Type* ptrTy = builder.getInt8Ty()->getPointerTo();
    llvm::Type* i32Ty = builder.getInt32Ty();
    llvm::Type* i64Ty = builder.getInt64Ty();

    llvm::FunctionType* wndProcTy = llvm::FunctionType::get(i64Ty, {ptrTy, i32Ty, i64Ty, i64Ty}, false);
    llvm::Function* wndProc = llvm::Function::Create(
        wndProcTy,
        llvm::Function::LinkageTypes::InternalLinkage,
        "__xfawa_window_proc_" + std::to_string(windowId),
        module);

    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", wndProc);
    llvm::BasicBlock* paintBB = llvm::BasicBlock::Create(context, "paint", wndProc);
    llvm::BasicBlock* destroyBB = llvm::BasicBlock::Create(context, "destroy", wndProc);
    llvm::BasicBlock* defaultBB = llvm::BasicBlock::Create(context, "default", wndProc);

    builder.SetInsertPoint(entryBB);

    auto argIt = wndProc->arg_begin();
    llvm::Value* hwnd = argIt++;
    hwnd->setName("hwnd");
    llvm::Value* msg = argIt++;
    msg->setName("msg");
    llvm::Value* wparam = argIt++;
    wparam->setName("wparam");
    llvm::Value* lparam = argIt++;
    lparam->setName("lparam");

    llvm::Value* isPaint = builder.CreateICmpEQ(msg, builder.getInt32(15), "is_paint");
    llvm::Value* isDestroy = builder.CreateICmpEQ(msg, builder.getInt32(2), "is_destroy");
    llvm::Value* routePaintOrOther = builder.CreateSelect(isPaint, builder.getInt1(true), isDestroy, "route_window_msg");
    builder.CreateCondBr(routePaintOrOther, paintBB, defaultBB);

    builder.SetInsertPoint(paintBB);
    llvm::BasicBlock* paintContinueBB = llvm::BasicBlock::Create(context, "paint_body", wndProc);
    builder.CreateCondBr(isDestroy, destroyBB, paintContinueBB);

    builder.SetInsertPoint(paintContinueBB);
    llvm::StructType* paintStructTy = llvm::StructType::create(
        context,
        {ptrTy, i32Ty, llvm::ArrayType::get(i32Ty, 4), i32Ty, i32Ty, llvm::ArrayType::get(builder.getInt8Ty(), 32)},
        "xfawa.paintstruct");
    llvm::AllocaInst* paintStruct = builder.CreateAlloca(paintStructTy, nullptr, "paintstruct");
    builder.CreateStore(llvm::Constant::getNullValue(paintStructTy), paintStruct);

    llvm::Function* beginPaintFunc = module->getFunction("BeginPaint");
    llvm::Function* endPaintFunc = module->getFunction("EndPaint");
    llvm::Function* createSolidBrushFunc = module->getFunction("CreateSolidBrush");
    llvm::Function* fillRectFunc = module->getFunction("FillRect");
    llvm::Function* deleteObjectFunc = module->getFunction("DeleteObject");

    llvm::Value* hdc = builder.CreateCall(beginPaintFunc, {hwnd, paintStruct}, "hdc");
    llvm::Value* rectPtr = builder.CreateStructGEP(paintStructTy, paintStruct, 2, "rectptr");
    llvm::Value* brush = builder.CreateCall(createSolidBrushFunc, {builder.getInt32(resolveWindowColor(windowDecl->color))}, "brush");
    builder.CreateCall(fillRectFunc, {hdc, rectPtr, brush});
    builder.CreateCall(deleteObjectFunc, {brush});
    builder.CreateCall(endPaintFunc, {hwnd, paintStruct});
    builder.CreateRet(builder.getInt64(0));

    builder.SetInsertPoint(destroyBB);
    llvm::GlobalVariable* windowCount = getWindowCountGlobal();
    llvm::Value* currentCount = builder.CreateLoad(builder.getInt32Ty(), windowCount, "window_count");
    llvm::Value* nextCount = builder.CreateSub(currentCount, builder.getInt32(1), "window_count_next");
    builder.CreateStore(nextCount, windowCount);

    llvm::BasicBlock* quitBB = llvm::BasicBlock::Create(context, "quit", wndProc);
    llvm::BasicBlock* noQuitBB = llvm::BasicBlock::Create(context, "no_quit", wndProc);
    llvm::Value* shouldQuit = builder.CreateICmpSLE(nextCount, builder.getInt32(0), "should_quit");
    builder.CreateCondBr(shouldQuit, quitBB, noQuitBB);

    builder.SetInsertPoint(quitBB);
    llvm::Function* postQuitMessageFunc = module->getFunction("PostQuitMessage");
    builder.CreateCall(postQuitMessageFunc, {builder.getInt32(0)});
    builder.CreateRet(builder.getInt64(0));

    builder.SetInsertPoint(noQuitBB);
    builder.CreateRet(builder.getInt64(0));

    builder.SetInsertPoint(defaultBB);
    llvm::Function* defWindowProcFunc = module->getFunction("DefWindowProcA");
    builder.CreateRet(builder.CreateCall(defWindowProcFunc, {hwnd, msg, wparam, lparam}, "defproc"));

    return wndProc;
}

llvm::Function* LLVMCodegen::createWindowRuntime(WindowStatement* windowDecl, int windowId, llvm::Function* wndProc) {
    llvm::Type* ptrTy = builder.getInt8Ty()->getPointerTo();
    llvm::Type* i32Ty = builder.getInt32Ty();

    llvm::FunctionType* runtimeTy = llvm::FunctionType::get(builder.getVoidTy(), false);
    llvm::Function* mainFunc = llvm::Function::Create(
        runtimeTy,
        llvm::Function::LinkageTypes::InternalLinkage,
        "__xfawa_window_runtime_" + std::to_string(windowId),
        module);

    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", mainFunc);

    builder.SetInsertPoint(entryBB);

    llvm::StructType* wndClassTy = llvm::StructType::create(
        context,
        {i32Ty, ptrTy, i32Ty, i32Ty, ptrTy, ptrTy, ptrTy, ptrTy, ptrTy, ptrTy},
        "xfawa.wndclassa");

    llvm::AllocaInst* wndClass = builder.CreateAlloca(wndClassTy, nullptr, "wndclass");
    builder.CreateStore(llvm::Constant::getNullValue(wndClassTy), wndClass);

    llvm::Value* className = builder.CreateGlobalStringPtr("XfawaWindowClass" + std::to_string(windowId), "window_class_name_" + std::to_string(windowId));
    llvm::Value* title = builder.CreateGlobalStringPtr(windowDecl->title, "window_title_" + std::to_string(windowId));

    llvm::Function* getModuleHandleFunc = module->getFunction("GetModuleHandleA");
    llvm::Function* loadCursorFunc = module->getFunction("LoadCursorA");
    llvm::Function* registerClassFunc = module->getFunction("RegisterClassA");
    llvm::Function* createWindowFunc = module->getFunction("CreateWindowExA");
    llvm::Function* showWindowFunc = module->getFunction("ShowWindow");
    llvm::Function* updateWindowFunc = module->getFunction("UpdateWindow");
    llvm::Value* nullPtr = llvm::Constant::getNullValue(ptrTy);
    llvm::Value* hInstance = builder.CreateCall(getModuleHandleFunc, {nullPtr}, "hinstance");
    llvm::Value* arrowCursorId = llvm::ConstantExpr::getIntToPtr(builder.getInt64(32512), llvm::cast<llvm::PointerType>(ptrTy));
    llvm::Value* cursor = builder.CreateCall(loadCursorFunc, {nullPtr, arrowCursorId}, "cursor");

    builder.CreateStore(builder.getInt32(3), builder.CreateStructGEP(wndClassTy, wndClass, 0));
    builder.CreateStore(builder.CreateBitCast(wndProc, ptrTy), builder.CreateStructGEP(wndClassTy, wndClass, 1));
    builder.CreateStore(builder.getInt32(0), builder.CreateStructGEP(wndClassTy, wndClass, 2));
    builder.CreateStore(builder.getInt32(0), builder.CreateStructGEP(wndClassTy, wndClass, 3));
    builder.CreateStore(hInstance, builder.CreateStructGEP(wndClassTy, wndClass, 4));
    builder.CreateStore(nullPtr, builder.CreateStructGEP(wndClassTy, wndClass, 5));
    builder.CreateStore(cursor, builder.CreateStructGEP(wndClassTy, wndClass, 6));
    builder.CreateStore(nullPtr, builder.CreateStructGEP(wndClassTy, wndClass, 7));
    builder.CreateStore(nullPtr, builder.CreateStructGEP(wndClassTy, wndClass, 8));
    builder.CreateStore(className, builder.CreateStructGEP(wndClassTy, wndClass, 9));

    builder.CreateCall(registerClassFunc, {wndClass});

    llvm::Value* hwnd = builder.CreateCall(
        createWindowFunc,
        {builder.getInt32(0), className, title, builder.getInt32(0x00CF0000),
         builder.getInt32(-2147483648), builder.getInt32(-2147483648),
         builder.getInt32(windowDecl->width), builder.getInt32(windowDecl->height),
         nullPtr, nullPtr, hInstance, nullPtr},
        "hwnd");

    builder.CreateCall(showWindowFunc, {hwnd, builder.getInt32(10)});
    builder.CreateCall(updateWindowFunc, {hwnd});
    llvm::GlobalVariable* windowCount = getWindowCountGlobal();
    llvm::Value* currentCount = builder.CreateLoad(i32Ty, windowCount, "window_count");
    llvm::Value* nextCount = builder.CreateAdd(currentCount, builder.getInt32(1), "window_count_next");
    builder.CreateStore(nextCount, windowCount);
    builder.CreateRetVoid();

    return mainFunc;
}

llvm::Value* LLVMCodegen::codegen(WindowStatement* windowStmt) {
    if (!windowStmt) return nullptr;

    hasWindowStatements = true;
    int windowId = generatedWindowCount++;
    llvm::IRBuilderBase::InsertPoint savedInsertPoint = builder.saveIP();
    llvm::Function* wndProc = createWindowProc(windowStmt, windowId);
    llvm::Function* runtimeFunc = createWindowRuntime(windowStmt, windowId, wndProc);
    builder.restoreIP(savedInsertPoint);
    return builder.CreateCall(runtimeFunc, {});
}

llvm::Value* LLVMCodegen::codegen(Expression* expr) {
    if (auto* e = dynamic_cast<NumberLiteral*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<FloatLiteral*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<BooleanLiteral*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<StringLiteral*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<VariableExpression*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<UnaryOp*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<BinaryOp*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<CallExpression*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<ArrayRangeExpression*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<ArrayLiteral*>(expr)) return codegen(e);
    if (auto* e = dynamic_cast<ArrayIndexExpression*>(expr)) return codegen(e);
    return nullptr;
}

bool LLVMCodegen::emitObjectFile(const std::string& filename) {
    return emitObjectFile(filename, false, false, "", "");
}

bool LLVMCodegen::emitObjectFile(const std::string& filename, bool keepLL, bool emitAsm, 
                                  const std::string& llOutputPath, const std::string& asmOutputPath) {
    runOptimizations();
    
    if (module->getTargetTriple().empty()) {
        module->setTargetTriple(llvm::Triple(llvm::sys::getDefaultTargetTriple()));
    }
    
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

    auto targetMachine = createTargetMachine(*module, optLevel, errors);
    if (!targetMachine) {
        if (!keepLL) {
            std::remove(llFile.c_str());
        }
        return false;
    }

    if (emitAsm) {
        if (!emitMachineCode(*module, *targetMachine, asmFile, llvm::CodeGenFileType::AssemblyFile, errors)) {
            addError("ASM file generation failed");
            return false;
        }

        if (!emitMachineCode(*module, *targetMachine, objFile, llvm::CodeGenFileType::ObjectFile, errors)) {
            addError("Object file generation from ASM failed");
            return false;
        }
        
        if (!keepLL) {
            std::remove(llFile.c_str());
        }
        
        return true;
    }

    if (!emitMachineCode(*module, *targetMachine, objFile, llvm::CodeGenFileType::ObjectFile, errors)) {
        addError("Object file generation failed");
        return false;
    }
    
    if (!keepLL) {
        std::remove(llFile.c_str());
    }
    
    return true;
}

bool LLVMCodegen::linkExecutable(const std::string& objFile, const std::string& outFile) {
    auto vcToolsDir = getEnvPath("VCToolsInstallDir");
    auto universalCrtDir = getEnvPath("UniversalCRTSdkDir");
    auto windowsSdkDir = getEnvPath("WindowsSdkDir");

    const char* ucrtVersionRaw = std::getenv("UCRTVersion");
    const char* windowsSdkVersionRaw = std::getenv("WindowsSDKLibVersion");

    std::vector<std::string> argStorage;
    argStorage.reserve(24);
    argStorage.push_back("lld-link");
    argStorage.push_back("/nologo");
    argStorage.push_back("/machine:x64");
    argStorage.push_back(hasWindowStatements ? "/subsystem:windows" : "/subsystem:console");
    argStorage.push_back("/entry:mainCRTStartup");
    argStorage.push_back("/out:" + outFile);
    argStorage.push_back(objFile);

    // If the user is already in a VS developer environment, use those paths.
    // Otherwise let embedded LLD auto-detect the MSVC and Windows SDK layout.
    if (vcToolsDir && universalCrtDir && windowsSdkDir && ucrtVersionRaw && windowsSdkVersionRaw) {
        std::filesystem::path vcLibDir = *vcToolsDir / "lib" / "x64";
        std::filesystem::path ucrtLibDir = *universalCrtDir / "Lib" / ucrtVersionRaw / "ucrt" / "x64";
        std::filesystem::path umLibDir = *windowsSdkDir / "Lib" / windowsSdkVersionRaw / "um" / "x64";

        if (!appendLibPath(argStorage, vcLibDir) ||
            !appendLibPath(argStorage, ucrtLibDir) ||
            !appendLibPath(argStorage, umLibDir)) {
            addError("Unable to locate required MSVC or Windows SDK library directories from the current environment");
            return false;
        }
    }

    argStorage.push_back("/defaultlib:libcmt");
    argStorage.push_back("/defaultlib:libvcruntime");
    argStorage.push_back("/defaultlib:libucrt");
    argStorage.push_back("/defaultlib:legacy_stdio_definitions");
    argStorage.push_back("/defaultlib:kernel32");
    argStorage.push_back("/defaultlib:user32");
    argStorage.push_back("/defaultlib:gdi32");
    argStorage.push_back("/defaultlib:advapi32");

    std::vector<const char*> args;
    args.reserve(argStorage.size());
    for (const auto& arg : argStorage) {
        args.push_back(arg.c_str());
    }

    std::string lldStdout;
    std::string lldStderr;
    llvm::raw_string_ostream stdoutStream(lldStdout);
    llvm::raw_string_ostream stderrStream(lldStderr);
    bool success = lld::coff::link(args, stdoutStream, stderrStream, false, false);
    stdoutStream.flush();
    stderrStream.flush();

    if (!success) {
        if (!lldStderr.empty()) {
            addError("LLD linking failed: " + lldStderr);
        } else if (!lldStdout.empty()) {
            addError("LLD linking failed: " + lldStdout);
        } else {
            addError("LLD linking failed with an unknown error");
        }
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

void LLVMCodegen::runOptimizations() {
    switch (optLevel) {
        case OptimizationLevel::O0:
            runO0Optimizations();
            break;
        case OptimizationLevel::O1:
            runO1Optimizations();
            break;
        case OptimizationLevel::O2:
            runO2Optimizations();
            break;
        case OptimizationLevel::O3:
            runO3Optimizations();
            break;
    }
}

void LLVMCodegen::runO0Optimizations() {
}

void LLVMCodegen::runO1Optimizations() {
    llvm::PassBuilder passBuilder;
    llvm::LoopAnalysisManager loopAnalysisManager;
    llvm::FunctionAnalysisManager functionAnalysisManager;
    llvm::CGSCCAnalysisManager cgsccAnalysisManager;
    llvm::ModuleAnalysisManager moduleAnalysisManager;
    
    passBuilder.registerModuleAnalyses(moduleAnalysisManager);
    passBuilder.registerCGSCCAnalyses(cgsccAnalysisManager);
    passBuilder.registerFunctionAnalyses(functionAnalysisManager);
    passBuilder.registerLoopAnalyses(loopAnalysisManager);
    passBuilder.crossRegisterProxies(loopAnalysisManager, functionAnalysisManager, cgsccAnalysisManager, moduleAnalysisManager);
    
    llvm::OptimizationLevel level = llvm::OptimizationLevel::O1;
    llvm::ModulePassManager modulePassManager = passBuilder.buildPerModuleDefaultPipeline(level);
    modulePassManager.run(*module, moduleAnalysisManager);
}

void LLVMCodegen::runO2Optimizations() {
    llvm::PassBuilder passBuilder;
    llvm::LoopAnalysisManager loopAnalysisManager;
    llvm::FunctionAnalysisManager functionAnalysisManager;
    llvm::CGSCCAnalysisManager cgsccAnalysisManager;
    llvm::ModuleAnalysisManager moduleAnalysisManager;
    
    passBuilder.registerModuleAnalyses(moduleAnalysisManager);
    passBuilder.registerCGSCCAnalyses(cgsccAnalysisManager);
    passBuilder.registerFunctionAnalyses(functionAnalysisManager);
    passBuilder.registerLoopAnalyses(loopAnalysisManager);
    passBuilder.crossRegisterProxies(loopAnalysisManager, functionAnalysisManager, cgsccAnalysisManager, moduleAnalysisManager);
    
    llvm::OptimizationLevel level = llvm::OptimizationLevel::O2;
    llvm::ModulePassManager modulePassManager = passBuilder.buildPerModuleDefaultPipeline(level);
    modulePassManager.run(*module, moduleAnalysisManager);
}

void LLVMCodegen::runO3Optimizations() {
    llvm::PassBuilder passBuilder;
    llvm::LoopAnalysisManager loopAnalysisManager;
    llvm::FunctionAnalysisManager functionAnalysisManager;
    llvm::CGSCCAnalysisManager cgsccAnalysisManager;
    llvm::ModuleAnalysisManager moduleAnalysisManager;
    
    passBuilder.registerModuleAnalyses(moduleAnalysisManager);
    passBuilder.registerCGSCCAnalyses(cgsccAnalysisManager);
    passBuilder.registerFunctionAnalyses(functionAnalysisManager);
    passBuilder.registerLoopAnalyses(loopAnalysisManager);
    passBuilder.crossRegisterProxies(loopAnalysisManager, functionAnalysisManager, cgsccAnalysisManager, moduleAnalysisManager);
    
    llvm::OptimizationLevel level = llvm::OptimizationLevel::O3;
    llvm::ModulePassManager modulePassManager = passBuilder.buildPerModuleDefaultPipeline(level);
    modulePassManager.run(*module, moduleAnalysisManager);
}

}
