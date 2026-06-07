#include "xfawa_llvm_codegen.h"
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <sstream>
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif
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
    : context(ctx), module(mod), builder(ctx), hasMainFunction(false), hasWindowStatements(false), usesRandomBuiltin(false), loopEndBB(nullptr), optLevel(OptimizationLevel::O2), generatedWindowCount(0), activeWindowId(-1) {
    initBuiltins();
}

std::optional<std::filesystem::path> getCompilerAdjacentXraphicsLib() {
#if defined(_WIN32)
    char pathBuffer[MAX_PATH] = {};
    DWORD length = GetModuleFileNameA(nullptr, pathBuffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return std::nullopt;
    }

    std::filesystem::path exePath(pathBuffer);
    std::filesystem::path candidate = exePath.parent_path() / "internal" / "xraphics.lib";
    if (std::filesystem::exists(candidate)) {
        return candidate;
    }
#endif
    return std::nullopt;
}

LLVMCodegen::LLVMCodegen(llvm::LLVMContext& ctx, llvm::Module* mod, OptimizationLevel opt)
    : context(ctx), module(mod), builder(ctx), hasMainFunction(false), hasWindowStatements(false), usesRandomBuiltin(false), loopEndBB(nullptr), optLevel(opt), generatedWindowCount(0), activeWindowId(-1) {
    initBuiltins();
}

void LLVMCodegen::initBuiltins() {
    auto declareFunction = [this](const std::string& name, llvm::FunctionType* type) -> llvm::Function* {
        if (llvm::Function* existing = module->getFunction(name)) {
            return existing;
        }
        return llvm::Function::Create(type, llvm::Function::LinkageTypes::ExternalLinkage, 0, name, module);
    };

    llvm::Type* ptrTy = llvm::PointerType::get(context, 0);

    llvm::FunctionType* printfType = llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy}, true);
    llvm::Function* printfFunc = declareFunction("printf", printfType);
    printfFunc->setCallingConv(llvm::CallingConv::C);
    printfFunc->addFnAttr(llvm::Attribute::NoUnwind);
    declareFunction("snprintf", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy, builder.getInt64Ty(), ptrTy}, true));
    declareFunction("setvbuf", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy, ptrTy, builder.getInt32Ty(), builder.getInt64Ty()}, false));
    declareFunction("fflush", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy}, false));
    declareFunction("__acrt_iob_func", llvm::FunctionType::get(ptrTy, {builder.getInt32Ty()}, false));

    llvm::FunctionType* mallocType = llvm::FunctionType::get(ptrTy, {builder.getInt64Ty()}, false);
    declareFunction("malloc", mallocType);
    
    declareFunction("strtod", llvm::FunctionType::get(builder.getDoubleTy(), {ptrTy, ptrTy->getPointerTo()}, false));
    declareFunction("strtof", llvm::FunctionType::get(builder.getFloatTy(), {ptrTy, ptrTy->getPointerTo()}, false));
    declareFunction("strtol", llvm::FunctionType::get(builder.getInt64Ty(), {ptrTy, ptrTy->getPointerTo(), builder.getInt32Ty()}, false));
    declareFunction("strtoll", llvm::FunctionType::get(builder.getInt64Ty(), {ptrTy, ptrTy->getPointerTo(), builder.getInt32Ty()}, false));

    llvm::FunctionType* randType = llvm::FunctionType::get(builder.getInt32Ty(), false);
    declareFunction("rand", randType);

    llvm::FunctionType* srandType = llvm::FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty()}, false);
    declareFunction("srand", srandType);

    llvm::FunctionType* timeType = llvm::FunctionType::get(builder.getInt64Ty(), {llvm::PointerType::get(context, 0)}, false);
    declareFunction("time", timeType);

    llvm::FunctionType* clockType = llvm::FunctionType::get(builder.getInt64Ty(), false);
    declareFunction("clock", clockType);

    declareFunction("GetModuleHandleA", llvm::FunctionType::get(ptrTy, {ptrTy}, false));
    declareFunction("FreeConsole", llvm::FunctionType::get(builder.getInt32Ty(), false));
    declareFunction("LoadCursorA", llvm::FunctionType::get(ptrTy, {ptrTy, ptrTy}, false));
    declareFunction("RegisterClassA", llvm::FunctionType::get(builder.getInt16Ty(), {ptrTy}, false));
    declareFunction("CreateWindowExA", llvm::FunctionType::get(
        ptrTy,
        {builder.getInt32Ty(), ptrTy, ptrTy, builder.getInt32Ty(), builder.getInt32Ty(),
         builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), ptrTy, ptrTy, ptrTy, ptrTy},
        false));
    declareFunction("MessageBoxA", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy, ptrTy, ptrTy, builder.getInt32Ty()}, false));
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
    declareFunction("xfawa_window_begin", llvm::FunctionType::get(
        ptrTy,
        {builder.getInt32Ty(), builder.getInt32Ty(), ptrTy, builder.getInt32Ty()},
        false));
    declareFunction("xfawa_window_add_button", llvm::FunctionType::get(
        builder.getInt32Ty(),
        {ptrTy, builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), ptrTy, ptrTy},
        false));
    declareFunction("xfawa_window_add_text", llvm::FunctionType::get(
        builder.getInt32Ty(),
        {ptrTy, builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), ptrTy},
        false));
    declareFunction("xfawa_window_add_box", llvm::FunctionType::get(
        builder.getInt32Ty(),
        {ptrTy, builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), ptrTy, ptrTy},
        false));
    declareFunction("xfawa_window_append_box", llvm::FunctionType::get(
        builder.getInt32Ty(),
        {ptrTy, ptrTy, ptrTy},
        false));
    declareFunction("xfawa_window_show", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy}, false));
    declareFunction("xfawa_windows_run", llvm::FunctionType::get(builder.getInt32Ty(), false));
    declareFunction("xr_create_window", llvm::FunctionType::get(builder.getInt32Ty(), {builder.getInt32Ty(), builder.getInt32Ty(), ptrTy}, false));
    declareFunction("xr_show_window", llvm::FunctionType::get(builder.getInt32Ty(), false));
    declareFunction("xr_poll_events", llvm::FunctionType::get(builder.getInt32Ty(), false));
    declareFunction("xr_should_close", llvm::FunctionType::get(builder.getInt32Ty(), false));
    declareFunction("xr_begin_frame", llvm::FunctionType::get(builder.getInt32Ty(), false));
    declareFunction("xr_end_frame", llvm::FunctionType::get(builder.getInt32Ty(), false));
    declareFunction("xr_draw_rect", llvm::FunctionType::get(builder.getInt32Ty(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false));
    declareFunction("xr_draw_text", llvm::FunctionType::get(builder.getInt32Ty(), {builder.getInt32Ty(), builder.getInt32Ty(), ptrTy, builder.getInt32Ty()}, false));
    declareFunction("xr_draw_button", llvm::FunctionType::get(builder.getInt32Ty(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), ptrTy, ptrTy}, false));
    declareFunction("xr_draw_box", llvm::FunctionType::get(builder.getInt32Ty(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), ptrTy, ptrTy}, false));
    declareFunction("xr_draw_input", llvm::FunctionType::get(ptrTy, {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), ptrTy, ptrTy}, false));
    declareFunction("xr_append_box", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy, ptrTy}, false));
    declareFunction("xr_load_style", llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy}, false));
    declareFunction("xr_set_clear_color", llvm::FunctionType::get(builder.getInt32Ty(), {builder.getInt32Ty()}, false));
}

llvm::Type* LLVMCodegen::getLLVMType(VarType type) {
    switch (type) {
        case VarType::INT:
            return builder.getInt32Ty();
        case VarType::LONG:
            return builder.getInt64Ty();
        case VarType::FLOAT:
            return builder.getFloatTy();
        case VarType::BOOL:
            return builder.getInt1Ty();
        case VarType::STRING:
            return llvm::PointerType::get(context, 0);
        case VarType::ARRAY_INT:
            return llvm::PointerType::get(context, 0);
        case VarType::ARRAY_LONG:
            return llvm::PointerType::get(context, 0);
        case VarType::ARRAY_FLOAT:
            return llvm::PointerType::get(context, 0);
        case VarType::ARRAY_BOOL:
            return llvm::PointerType::get(context, 0);
        case VarType::ARRAY_STRING:
            return llvm::PointerType::get(context, 0);
        default:
            return builder.getInt32Ty();
    }
}

llvm::Type* LLVMCodegen::getArrayElementType(VarType type) {
    switch (type) {
        case VarType::ARRAY_INT:
            return builder.getInt32Ty();
        case VarType::ARRAY_LONG:
            return builder.getInt64Ty();
        case VarType::ARRAY_FLOAT:
            return builder.getFloatTy();
        case VarType::ARRAY_BOOL:
            return builder.getInt1Ty();
        case VarType::ARRAY_STRING:
            return builder.getInt8Ty()->getPointerTo();
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
    constexpr int64_t INT32_MAX_VAL = 2147483647LL;
    constexpr int64_t INT32_MIN_VAL = -2147483648LL;
    
    if (expr->value > INT32_MAX_VAL || expr->value < INT32_MIN_VAL) {
        return createConstInt(context, llvm::Type::getInt64Ty(context), expr->value);
    }
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
    
    auto globalIt = windowInputGlobals.find(expr->name);
    if (globalIt != windowInputGlobals.end()) {
        return builder.CreateLoad(globalIt->second->getValueType(), globalIt->second, expr->name.c_str());
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
    
    bool isFloatOp = leftType->isFloatTy() || rightType->isFloatTy();
    bool isLongOp = leftType->isIntegerTy(64) || rightType->isIntegerTy(64);
    bool isPtrOp = leftType->isPointerTy() && rightType->isPointerTy();
    
    bool leftIsPtr = leftType->isPointerTy();
    bool rightIsPtr = rightType->isPointerTy();
    bool leftIsNum = leftType->isIntegerTy() || leftType->isFloatTy();
    bool rightIsNum = rightType->isIntegerTy() || rightType->isFloatTy();
    
    if (expr->op == BinaryOpType::ADD && ((leftIsPtr && rightIsNum) || (leftIsNum && rightIsPtr))) {
        auto convertNumToStr = [this](llvm::Value* numVal, llvm::Type* numType) -> llvm::Value* {
            llvm::Function* mallocFunc = module->getFunction("malloc");
            if (!mallocFunc) {
                llvm::FunctionType* mallocType = llvm::FunctionType::get(
                    builder.getInt8Ty()->getPointerTo(),
                    {builder.getInt64Ty()},
                    false);
                mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, 0, "malloc", module);
            }
            
            llvm::Value* buffer = builder.CreateCall(mallocFunc, {builder.getInt64(64)}, "num_str_buf");
            
            llvm::Function* sprintfFunc = module->getFunction("sprintf");
            if (!sprintfFunc) {
                llvm::FunctionType* sprintfType = llvm::FunctionType::get(
                    builder.getInt32Ty(),
                    {builder.getInt8Ty()->getPointerTo(), builder.getInt8Ty()->getPointerTo()},
                    true);
                sprintfFunc = llvm::Function::Create(sprintfType, llvm::Function::ExternalLinkage, 0, "sprintf", module);
            }
            
            llvm::Value* formatStr = nullptr;
            llvm::Value* valToPrint = numVal;
            if (numType->isFloatTy()) {
                formatStr = builder.CreateGlobalStringPtr("%g", "float_fmt");
                valToPrint = builder.CreateFPExt(numVal, builder.getDoubleTy(), "float_to_double");
            } else if (numType->isIntegerTy(64)) {
                formatStr = builder.CreateGlobalStringPtr("%lld", "long_fmt");
            } else {
                formatStr = builder.CreateGlobalStringPtr("%d", "int_fmt");
            }
            
            builder.CreateCall(sprintfFunc, {buffer, formatStr, valToPrint}, "sprintf_num");
            return buffer;
        };
        
        if (leftIsPtr && rightIsNum) {
            rightVal = convertNumToStr(rightVal, rightType);
        } else {
            leftVal = convertNumToStr(leftVal, leftType);
        }
        
        llvm::Function* strlenFunc = module->getFunction("strlen");
        if (!strlenFunc) {
            llvm::FunctionType* strlenType = llvm::FunctionType::get(builder.getInt64Ty(), {builder.getPtrTy()}, false);
            strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, 0, "strlen", module);
        }
        
        llvm::Value* leftLen = builder.CreateCall(strlenFunc, {leftVal}, "left.len");
        llvm::Value* rightLen = builder.CreateCall(strlenFunc, {rightVal}, "right.len");
        
        llvm::Value* totalLen = builder.CreateAdd(leftLen, rightLen, "total.len");
        llvm::Value* totalLenPlus1 = builder.CreateAdd(totalLen, createConstInt(context, builder.getInt64Ty(), 1), "total.len.plus1");
        
        llvm::Function* mallocFunc = module->getFunction("malloc");
        llvm::Value* resultPtr = builder.CreateCall(mallocFunc, {totalLenPlus1}, "concat.alloc");
        
        llvm::Function* strcpyFunc = module->getFunction("strcpy");
        if (!strcpyFunc) {
            llvm::FunctionType* strcpyType = llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false);
            strcpyFunc = llvm::Function::Create(strcpyType, llvm::Function::ExternalLinkage, 0, "strcpy", module);
        }
        builder.CreateCall(strcpyFunc, {resultPtr, leftVal}, "strcpy.left");
        
        llvm::Function* strcatFunc = module->getFunction("strcat");
        if (!strcatFunc) {
            llvm::FunctionType* strcatType = llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false);
            strcatFunc = llvm::Function::Create(strcatType, llvm::Function::ExternalLinkage, 0, "strcat", module);
        }
        builder.CreateCall(strcatFunc, {resultPtr, rightVal}, "strcat.right");
        
        return resultPtr;
    }
    
    if (isPtrOp && expr->op == BinaryOpType::ADD) {
        llvm::Function* strlenFunc = module->getFunction("strlen");
        if (!strlenFunc) {
            llvm::FunctionType* strlenType = llvm::FunctionType::get(builder.getInt64Ty(), {builder.getPtrTy()}, false);
            strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, 0, "strlen", module);
        }
        
        llvm::Value* leftLen = builder.CreateCall(strlenFunc, {leftVal}, "left.len");
        llvm::Value* rightLen = builder.CreateCall(strlenFunc, {rightVal}, "right.len");
        
        llvm::Value* totalLen = builder.CreateAdd(leftLen, rightLen, "total.len");
        llvm::Value* totalLenPlus1 = builder.CreateAdd(totalLen, createConstInt(context, builder.getInt64Ty(), 1), "total.len.plus1");
        
        llvm::Function* mallocFunc = module->getFunction("malloc");
        llvm::Value* resultPtr = builder.CreateCall(mallocFunc, {totalLenPlus1}, "concat.alloc");
        
        llvm::Function* strcpyFunc = module->getFunction("strcpy");
        if (!strcpyFunc) {
            llvm::FunctionType* strcpyType = llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false);
            strcpyFunc = llvm::Function::Create(strcpyType, llvm::Function::ExternalLinkage, 0, "strcpy", module);
        }
        builder.CreateCall(strcpyFunc, {resultPtr, leftVal}, "strcpy.left");
        
        llvm::Function* strcatFunc = module->getFunction("strcat");
        if (!strcatFunc) {
            llvm::FunctionType* strcatType = llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false);
            strcatFunc = llvm::Function::Create(strcatType, llvm::Function::ExternalLinkage, 0, "strcat", module);
        }
        builder.CreateCall(strcatFunc, {resultPtr, rightVal}, "strcat.right");
        
        return resultPtr;
    }
    
    if (leftType->isIntegerTy(1)) {
        llvm::Type* i32 = llvm::Type::getInt32Ty(context);
        leftVal = builder.CreateZExtOrTrunc(leftVal, i32, "booltoi32");
    }
    if (rightType->isIntegerTy(1)) {
        llvm::Type* i32 = llvm::Type::getInt32Ty(context);
        rightVal = builder.CreateZExtOrTrunc(rightVal, i32, "booltoi32");
    }
    
    if (isFloatOp) {
        if (!leftType->isFloatTy()) {
            leftVal = builder.CreateSIToFP(leftVal, builder.getFloatTy(), "inttofp");
        }
        if (!rightType->isFloatTy()) {
            rightVal = builder.CreateSIToFP(rightVal, builder.getFloatTy(), "inttofp");
        }
        
        switch (expr->op) {
            case BinaryOpType::ADD:
                return builder.CreateFAdd(leftVal, rightVal, "addtmp");
            case BinaryOpType::SUB:
                return builder.CreateFSub(leftVal, rightVal, "subtmp");
            case BinaryOpType::MUL:
                return builder.CreateFMul(leftVal, rightVal, "multmp");
            case BinaryOpType::DIV:
                return builder.CreateFDiv(leftVal, rightVal, "divtmp");
            case BinaryOpType::MOD:
                return builder.CreateFRem(leftVal, rightVal, "modtmp");
            case BinaryOpType::EQUAL:
                return builder.CreateFCmpOEQ(leftVal, rightVal, "eqtmp");
            case BinaryOpType::NOT_EQUAL:
                return builder.CreateFCmpONE(leftVal, rightVal, "netmp");
            case BinaryOpType::LESS:
                return builder.CreateFCmpOLT(leftVal, rightVal, "lttmp");
            case BinaryOpType::LESS_EQUAL:
                return builder.CreateFCmpOLE(leftVal, rightVal, "letmp");
            case BinaryOpType::GREATER:
                return builder.CreateFCmpOGT(leftVal, rightVal, "gttmp");
            case BinaryOpType::GREATER_EQUAL:
                return builder.CreateFCmpOGE(leftVal, rightVal, "getmp");
            case BinaryOpType::AND:
                return builder.CreateAnd(leftVal, rightVal, "andtmp");
            case BinaryOpType::OR:
                return builder.CreateOr(leftVal, rightVal, "ortmp");
            default:
                return nullptr;
        }
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
        usesRandomBuiltin = true;
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
    
    if (expr->name == "input") {
        llvm::Function* fgetsFunc = module->getFunction("fgets");
        if (!fgetsFunc) {
            llvm::FunctionType* fgetsType = llvm::FunctionType::get(
                builder.getInt8Ty()->getPointerTo(),
                {builder.getInt8Ty()->getPointerTo(), builder.getInt32Ty(), builder.getInt8Ty()->getPointerTo()},
                false);
            fgetsFunc = llvm::Function::Create(fgetsType, llvm::Function::ExternalLinkage, 0, "fgets", module);
        }
        
        llvm::Function* stdinFunc = module->getFunction("__acrt_iob_func");
        if (!stdinFunc) {
            llvm::FunctionType* stdinType = llvm::FunctionType::get(
                builder.getInt8Ty()->getPointerTo(),
                {builder.getInt32Ty()},
                false);
            stdinFunc = llvm::Function::Create(stdinType, llvm::Function::ExternalLinkage, 0, "__acrt_iob_func", module);
        }
        
        llvm::Function* mallocFunc = module->getFunction("malloc");
        if (!mallocFunc) {
            llvm::FunctionType* mallocType = llvm::FunctionType::get(
                builder.getInt8Ty()->getPointerTo(),
                {builder.getInt64Ty()},
                false);
            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, 0, "malloc", module);
        }
        
        llvm::Value* bufferSize = builder.getInt64(1024);
        llvm::Value* buffer = builder.CreateCall(mallocFunc, {bufferSize}, "input_buffer");
        
        llvm::Value* stdinValue = builder.CreateCall(stdinFunc, {builder.getInt32(0)}, "stdin_val");
        builder.CreateCall(fgetsFunc, {buffer, builder.getInt32(1023), stdinValue}, "fgets_call");
        
        llvm::Function* strlenFunc = module->getFunction("strlen");
        if (!strlenFunc) {
            llvm::FunctionType* strlenType = llvm::FunctionType::get(
                builder.getInt64Ty(),
                {builder.getInt8Ty()->getPointerTo()},
                false);
            strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, 0, "strlen", module);
        }
        
        llvm::Value* len = builder.CreateCall(strlenFunc, {buffer}, "input_len");
        llvm::Value* lenNotZero = builder.CreateICmpNE(len, builder.getInt64(0), "len_not_zero");
        
        llvm::Function* currentFunc = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* checkNewlineBB = llvm::BasicBlock::Create(context, "check_newline", currentFunc);
        llvm::BasicBlock* trimBB = llvm::BasicBlock::Create(context, "trim_newline", currentFunc);
        llvm::BasicBlock* noTrimBB = llvm::BasicBlock::Create(context, "no_trim_newline", currentFunc);
        llvm::BasicBlock* doneBB = llvm::BasicBlock::Create(context, "input_done", currentFunc);
        
        builder.CreateCondBr(lenNotZero, checkNewlineBB, noTrimBB);
        
        builder.SetInsertPoint(checkNewlineBB);
        llvm::Value* lastIdx = builder.CreateSub(len, builder.getInt64(1), "last_idx");
        llvm::Value* lastCharPtr = builder.CreateGEP(builder.getInt8Ty(), buffer, lastIdx, "last_char_ptr");
        llvm::Value* lastChar = builder.CreateLoad(builder.getInt8Ty(), lastCharPtr, "last_char");
        llvm::Value* isNewline = builder.CreateICmpEQ(lastChar, builder.getInt8('\n'), "is_newline");
        builder.CreateCondBr(isNewline, trimBB, noTrimBB);
        
        builder.SetInsertPoint(trimBB);
        builder.CreateStore(builder.getInt8('\0'), lastCharPtr);
        builder.CreateBr(doneBB);
        
        builder.SetInsertPoint(noTrimBB);
        builder.CreateBr(doneBB);
        
        builder.SetInsertPoint(doneBB);
        
        return buffer;
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
    std::vector<VarType> argTypes;
    for (auto& arg : expr->args) {
        VarType argType = VarType::UNKNOWN;
        if (auto* strLit = dynamic_cast<StringLiteral*>(arg.get())) {
            argType = VarType::STRING;
        } else if (auto* numLit = dynamic_cast<NumberLiteral*>(arg.get())) {
            constexpr int64_t INT32_MAX_VAL = 2147483647LL;
            if (numLit->value > INT32_MAX_VAL || numLit->value < -INT32_MAX_VAL - 1) {
                argType = VarType::LONG;
            } else {
                argType = VarType::INT;
            }
        } else if (auto* varExpr = dynamic_cast<VariableExpression*>(arg.get())) {
            auto typeIt = localTypes.find(varExpr->name);
            if (typeIt != localTypes.end()) {
                argType = typeIt->second;
            }
        }
        argTypes.push_back(argType);
        
        llvm::Value* argVal = codegen(arg.get());
        if (!argVal) return nullptr;
        
        if (argVal->getType()->isIntegerTy(1)) {
            argVal = builder.CreateZExtOrTrunc(argVal, llvm::Type::getInt64Ty(context), "argbool");
            argVal = builder.CreateIntToPtr(argVal, builder.getPtrTy(), "argboolptr");
        } else if (argVal->getType()->isIntegerTy(32)) {
            argVal = builder.CreateSExt(argVal, llvm::Type::getInt64Ty(context), "argext");
            argVal = builder.CreateIntToPtr(argVal, builder.getPtrTy(), "argextptr");
        } else if (argVal->getType()->isIntegerTy(64)) {
            argVal = builder.CreateIntToPtr(argVal, builder.getPtrTy(), "argptr");
        }
        
        args.push_back(argVal);
    }
    
    callArgTypes[funcName] = argTypes;
    
    llvm::CallInst* callInst = builder.CreateCall(callee->getFunctionType(), callee, args);
    
    auto retTypeIt = funcReturnTypes.find(funcName);
    if (retTypeIt != funcReturnTypes.end()) {
        if (retTypeIt->second == VarType::FLOAT) {
            llvm::Value* bits = callInst;
            bits = builder.CreateBitCast(bits, builder.getDoubleTy(), "float.ret.bits");
            return builder.CreateFPTrunc(bits, builder.getFloatTy(), "float.ret");
        } else if (retTypeIt->second == VarType::BOOL) {
            return builder.CreateTrunc(callInst, builder.getInt1Ty(), "bool.ret");
        }
    }
    
    return callInst;
}

llvm::Value* LLVMCodegen::codegen(ArrayRangeExpression* expr) {
    if (expr->isSlice && expr->array) {
        llvm::Value* arrVal = codegen(expr->array.get());
        if (!arrVal) return nullptr;
        
        llvm::Value* startVal = codegen(expr->start.get());
        if (!startVal) return nullptr;
        
        llvm::Value* endVal = codegen(expr->end.get());
        if (!endVal) return nullptr;
        
        int64_t knownArrayLen = 0;
        if (auto* varExpr = dynamic_cast<VariableExpression*>(expr->array.get())) {
            auto lenIt = arrayLengths.find(varExpr->name);
            if (lenIt != arrayLengths.end()) {
                knownArrayLen = lenIt->second;
            }
        }
        
        llvm::Value* sliceSize = builder.CreateSub(endVal, startVal, "slice.size");
        sliceSize = builder.CreateAdd(sliceSize, createConstInt(context, builder.getInt32Ty(), 1), "slice.size.adj");
        
        llvm::Value* sliceSizeExt = builder.CreateSExt(sliceSize, builder.getInt64Ty(), "slice.size.ext");
        llvm::Value* allocSize = builder.CreateMul(sliceSizeExt, createConstInt(context, builder.getInt64Ty(), 4), "alloc.size");
        
        llvm::Function* mallocFunc = module->getFunction("malloc");
        if (!mallocFunc) {
            llvm::FunctionType* mallocType = llvm::FunctionType::get(llvm::PointerType::get(context, 0), {builder.getInt64Ty()}, false);
            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, 0, "malloc", module);
        }
        llvm::Value* rawPtr = builder.CreateCall(mallocFunc, {allocSize}, "slice.alloc");
        llvm::Value* slicePtr = rawPtr;
        
        llvm::Function* func = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* condBB = llvm::BasicBlock::Create(context, "slice.cond", func);
        llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(context, "slice.body", func);
        llvm::BasicBlock* endBB = llvm::BasicBlock::Create(context, "slice.end", func);
        
        llvm::AllocaInst* iAlloca = builder.CreateAlloca(builder.getInt32Ty(), nullptr, "slice.i");
        builder.CreateStore(createConstInt(context, builder.getInt32Ty(), 0), iAlloca);
        
        builder.CreateBr(condBB);
        
        builder.SetInsertPoint(condBB);
        llvm::Value* iVal = builder.CreateLoad(builder.getInt32Ty(), iAlloca, "slice.i.val");
        llvm::Value* cond = builder.CreateICmpSLT(iVal, sliceSize, "slice.cond");
        builder.CreateCondBr(cond, bodyBB, endBB);
        
        builder.SetInsertPoint(bodyBB);
        llvm::Value* srcIdx = builder.CreateAdd(startVal, iVal, "src.idx");
        llvm::Value* srcIdxExt = builder.CreateSExt(srcIdx, builder.getInt64Ty(), "src.idx.ext");
        llvm::Value* srcElemPtr = builder.CreateGEP(builder.getInt32Ty(), arrVal, srcIdxExt, "src.elem.ptr");
        llvm::Value* elem = builder.CreateLoad(builder.getInt32Ty(), srcElemPtr, "elem");
        
        llvm::Value* dstElemPtr = builder.CreateGEP(builder.getInt32Ty(), slicePtr, builder.CreateSExt(iVal, builder.getInt64Ty()), "dst.elem.ptr");
        builder.CreateStore(elem, dstElemPtr);
        
        llvm::Value* nextI = builder.CreateAdd(iVal, createConstInt(context, builder.getInt32Ty(), 1), "next.i");
        builder.CreateStore(nextI, iAlloca);
        builder.CreateBr(condBB);
        
        builder.SetInsertPoint(endBB);
        
        return slicePtr;
    }
    
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
    usesRandomBuiltin = true;
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
                    llvm::Value* arrPtr = builder.CreateCall(mallocFunc, {allocSize}, "arr.alloc");
                    
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
        return llvm::ConstantPointerNull::get(builder.getPtrTy());
    }
    
    bool isStringArray = false;
    bool isLongArray = false;
    size_t elemSize = 4;
    
    if (!expr->elements.empty()) {
        if (auto* strLit = dynamic_cast<StringLiteral*>(expr->elements[0].get())) {
            isStringArray = true;
            elemSize = 8;
            (void)strLit;
        } else if (auto* numLit = dynamic_cast<NumberLiteral*>(expr->elements[0].get())) {
            constexpr int64_t INT32_MAX_VAL = 2147483647LL;
            if (numLit->value > INT32_MAX_VAL || numLit->value < -INT32_MAX_VAL - 1) {
                isLongArray = true;
                elemSize = 8;
            }
        }
    }
    
    llvm::Function* mallocFunc = module->getFunction("malloc");
    llvm::Value* allocSize = createConstInt(context, builder.getInt64Ty(), size * elemSize);
    llvm::Value* arrPtr = builder.CreateCall(mallocFunc, {allocSize}, "arr.alloc");
    
    llvm::Type* elemType = builder.getInt32Ty();
    if (isStringArray) {
        elemType = static_cast<llvm::Type*>(builder.getInt8Ty()->getPointerTo());
    } else if (isLongArray) {
        elemType = static_cast<llvm::Type*>(builder.getInt64Ty());
    }
    
    for (size_t i = 0; i < size; i++) {
        llvm::Value* elemVal = codegen(expr->elements[i].get());
        if (!elemVal) return nullptr;
        
        if (!isStringArray) {
            if (elemVal->getType()->isIntegerTy(1)) {
                elemVal = builder.CreateZExtOrTrunc(elemVal, isLongArray ? builder.getInt64Ty() : builder.getInt32Ty(), "boolext");
            }
        }
        
        llvm::Value* idx = createConstInt(context, builder.getInt64Ty(), i);
        llvm::Value* elemPtr = builder.CreateGEP(elemType, arrPtr, idx, "elem.ptr");
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
    VarType arrType = VarType::UNKNOWN;
    if (auto* varExpr = dynamic_cast<VariableExpression*>(expr->array.get())) {
        auto lenIt = arrayLengths.find(varExpr->name);
        if (lenIt != arrayLengths.end()) {
            knownArrayLen = lenIt->second;
        }
        auto typeIt = localTypes.find(varExpr->name);
        if (typeIt != localTypes.end()) {
            arrType = typeIt->second;
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
    
    llvm::Type* elemType = getArrayElementType(arrType);
    llvm::Value* elemPtr = builder.CreateGEP(elemType, arrVal, idxExt, "arr.elem.ptr");
    return builder.CreateLoad(elemType, elemPtr, "arr.elem");
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
    } else if (auto* arrSlice = dynamic_cast<ArrayRangeExpression*>(stmt->value.get())) {
        if (arrSlice->isSlice) {
            if (auto* startInt = dynamic_cast<NumberLiteral*>(arrSlice->start.get())) {
                if (auto* endInt = dynamic_cast<NumberLiteral*>(arrSlice->end.get())) {
                    arrayLen = endInt->value - startInt->value + 1;
                }
            }
        }
    }
    
    llvm::Value* value = nullptr;
    
    if (auto* callExpr = dynamic_cast<CallExpression*>(stmt->value.get())) {
        if (callExpr->name == "input" && stmt->hasExplicitType) {
            llvm::Function* fgetsFunc = module->getFunction("fgets");
            if (!fgetsFunc) {
                llvm::FunctionType* fgetsType = llvm::FunctionType::get(
                    builder.getInt8Ty()->getPointerTo(),
                    {builder.getInt8Ty()->getPointerTo(), builder.getInt32Ty(), builder.getInt8Ty()->getPointerTo()},
                    false);
                fgetsFunc = llvm::Function::Create(fgetsType, llvm::Function::ExternalLinkage, 0, "fgets", module);
            }
            
            llvm::Function* stdinFunc = module->getFunction("__acrt_iob_func");
            if (!stdinFunc) {
                llvm::FunctionType* stdinType = llvm::FunctionType::get(
                    builder.getInt8Ty()->getPointerTo(),
                    {builder.getInt32Ty()},
                    false);
                stdinFunc = llvm::Function::Create(stdinType, llvm::Function::ExternalLinkage, 0, "__acrt_iob_func", module);
            }
            
            llvm::Function* mallocFunc = module->getFunction("malloc");
            if (!mallocFunc) {
                llvm::FunctionType* mallocType = llvm::FunctionType::get(
                    builder.getInt8Ty()->getPointerTo(),
                    {builder.getInt64Ty()},
                    false);
                mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, 0, "malloc", module);
            }
            
            llvm::Value* bufferSize = builder.getInt64(256);
            llvm::Value* buffer = builder.CreateCall(mallocFunc, {bufferSize}, "input_buffer");
            
            llvm::Value* stdinValue = builder.CreateCall(stdinFunc, {builder.getInt32(0)}, "stdin_val");
            
            llvm::Function* strlenFunc = module->getFunction("strlen");
            if (!strlenFunc) {
                llvm::FunctionType* strlenType = llvm::FunctionType::get(
                    builder.getInt64Ty(),
                    {builder.getInt8Ty()->getPointerTo()},
                    false);
                strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, 0, "strlen", module);
            }
            
            llvm::Function* currentFunc = builder.GetInsertBlock()->getParent();
            llvm::BasicBlock* readLoopBB = llvm::BasicBlock::Create(context, "read_loop", currentFunc);
            llvm::BasicBlock* checkBB = llvm::BasicBlock::Create(context, "check_input", currentFunc);
            llvm::BasicBlock* doneInputBB = llvm::BasicBlock::Create(context, "done_input", currentFunc);
            llvm::BasicBlock* eofBB = llvm::BasicBlock::Create(context, "eof_handler", currentFunc);
            
            builder.CreateBr(readLoopBB);
            
            builder.SetInsertPoint(readLoopBB);
            llvm::Value* fgetsResult = builder.CreateCall(fgetsFunc, {buffer, builder.getInt32(255), stdinValue}, "fgets_call");
            llvm::Value* fgetsOk = builder.CreateICmpNE(fgetsResult, llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0)), "fgets_ok");
            builder.CreateCondBr(fgetsOk, checkBB, eofBB);
            
            builder.SetInsertPoint(checkBB);
            llvm::Value* len = builder.CreateCall(strlenFunc, {buffer}, "input_len");
            llvm::Value* lenIsZero = builder.CreateICmpEQ(len, builder.getInt64(0), "len_is_zero");
            llvm::Value* firstChar = builder.CreateLoad(builder.getInt8Ty(), buffer, "first_char");
            llvm::Value* isNewline = builder.CreateICmpEQ(firstChar, builder.getInt8('\n'), "is_newline_char");
            llvm::Value* isEmpty = builder.CreateOr(lenIsZero, isNewline, "is_empty");
            builder.CreateCondBr(isEmpty, readLoopBB, doneInputBB);
            
            builder.SetInsertPoint(eofBB);
            builder.CreateStore(builder.getInt8('\0'), buffer);
            builder.CreateBr(doneInputBB);
            
            builder.SetInsertPoint(doneInputBB);
            
            llvm::Value* len2 = builder.CreateCall(strlenFunc, {buffer}, "input_len2");
            llvm::Value* len2NotZero = builder.CreateICmpNE(len2, builder.getInt64(0), "len2_not_zero");
            
            llvm::BasicBlock* trimNewlineBB2 = llvm::BasicBlock::Create(context, "trim_newline2", currentFunc);
            llvm::BasicBlock* checkCRBB = llvm::BasicBlock::Create(context, "check_cr", currentFunc);
            llvm::BasicBlock* trimCRBB = llvm::BasicBlock::Create(context, "trim_cr", currentFunc);
            llvm::BasicBlock* doneTrimBB = llvm::BasicBlock::Create(context, "done_trim", currentFunc);
            
            builder.CreateCondBr(len2NotZero, trimNewlineBB2, doneTrimBB);
            
            builder.SetInsertPoint(trimNewlineBB2);
            llvm::Value* lastIdx2 = builder.CreateSub(len2, builder.getInt64(1), "last_idx2");
            llvm::Value* lastCharPtr2 = builder.CreateGEP(builder.getInt8Ty(), buffer, lastIdx2, "last_char_ptr2");
            llvm::Value* lastChar2 = builder.CreateLoad(builder.getInt8Ty(), lastCharPtr2, "last_char2");
            llvm::Value* isNewline2 = builder.CreateICmpEQ(lastChar2, builder.getInt8('\n'), "is_newline2");
            builder.CreateCondBr(isNewline2, checkCRBB, doneTrimBB);
            
            builder.SetInsertPoint(checkCRBB);
            builder.CreateStore(builder.getInt8('\0'), lastCharPtr2);
            llvm::Value* len3 = builder.CreateCall(strlenFunc, {buffer}, "input_len3");
            llvm::Value* len3NotZero = builder.CreateICmpNE(len3, builder.getInt64(0), "len3_not_zero");
            builder.CreateCondBr(len3NotZero, trimCRBB, doneTrimBB);
            
            builder.SetInsertPoint(trimCRBB);
            llvm::Value* lastIdx3 = builder.CreateSub(len3, builder.getInt64(1), "last_idx3");
            llvm::Value* lastCharPtr3 = builder.CreateGEP(builder.getInt8Ty(), buffer, lastIdx3, "last_char_ptr3");
            llvm::Value* lastChar3 = builder.CreateLoad(builder.getInt8Ty(), lastCharPtr3, "last_char3");
            llvm::Value* isCR = builder.CreateICmpEQ(lastChar3, builder.getInt8('\r'), "is_cr");
            llvm::BasicBlock* doTrimCRBB = llvm::BasicBlock::Create(context, "do_trim_cr", currentFunc);
            builder.CreateCondBr(isCR, doTrimCRBB, doneTrimBB);
            
            builder.SetInsertPoint(doTrimCRBB);
            builder.CreateStore(builder.getInt8('\0'), lastCharPtr3);
            builder.CreateBr(doneTrimBB);
            
            builder.SetInsertPoint(doneTrimBB);
            
            if (stmt->declaredType == VarType::INT) {
                llvm::Function* strtolFunc = module->getFunction("strtol");
                if (!strtolFunc) {
                    llvm::FunctionType* strtolType = llvm::FunctionType::get(
                        builder.getInt64Ty(),
                        {builder.getInt8Ty()->getPointerTo(), builder.getInt8Ty()->getPointerTo()->getPointerTo(), builder.getInt32Ty()},
                        false);
                    strtolFunc = llvm::Function::Create(strtolType, llvm::Function::ExternalLinkage, 0, "strtol", module);
                }
                llvm::Value* nullPtr = llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0));
                llvm::Value* longVal = builder.CreateCall(strtolFunc, {buffer, nullPtr, builder.getInt32(10)}, "strtol_call");
                value = builder.CreateTrunc(longVal, builder.getInt32Ty(), "int_trunc");
            }
            else if (stmt->declaredType == VarType::LONG) {
                llvm::Function* strtollFunc = module->getFunction("strtoll");
                if (!strtollFunc) {
                    llvm::FunctionType* strtollType = llvm::FunctionType::get(
                        builder.getInt64Ty(),
                        {builder.getInt8Ty()->getPointerTo(), builder.getInt8Ty()->getPointerTo()->getPointerTo(), builder.getInt32Ty()},
                        false);
                    strtollFunc = llvm::Function::Create(strtollType, llvm::Function::ExternalLinkage, 0, "strtoll", module);
                }
                llvm::Value* nullPtr = llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0));
                value = builder.CreateCall(strtollFunc, {buffer, nullPtr, builder.getInt32(10)}, "strtoll_call");
            }
            else if (stmt->declaredType == VarType::FLOAT) {
                llvm::Function* strtodFunc = module->getFunction("strtod");
                if (!strtodFunc) {
                    llvm::FunctionType* strtodType = llvm::FunctionType::get(
                        builder.getDoubleTy(),
                        {builder.getInt8Ty()->getPointerTo(), builder.getInt8Ty()->getPointerTo()->getPointerTo()},
                        false);
                    strtodFunc = llvm::Function::Create(strtodType, llvm::Function::ExternalLinkage, 0, "strtod", module);
                }
                llvm::Value* nullPtr = llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0));
                llvm::CallInst* call = builder.CreateCall(strtodFunc, {buffer, nullPtr}, "strtod_call");
                call->setTailCallKind(llvm::CallInst::TCK_None);
                llvm::Value* doubleVal = call;
                value = builder.CreateFPTrunc(doubleVal, builder.getFloatTy(), "double_to_float");
            }
            else {
                llvm::Function* strlenFunc = module->getFunction("strlen");
                if (!strlenFunc) {
                    llvm::FunctionType* strlenType = llvm::FunctionType::get(
                        builder.getInt64Ty(),
                        {builder.getInt8Ty()->getPointerTo()},
                        false);
                    strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, 0, "strlen", module);
                }
                
                llvm::Value* len = builder.CreateCall(strlenFunc, {buffer}, "input_len");
                llvm::Value* lenNotZero = builder.CreateICmpNE(len, builder.getInt64(0), "len_not_zero");
                
                llvm::Function* currentFunc = builder.GetInsertBlock()->getParent();
                llvm::BasicBlock* checkNewlineBB = llvm::BasicBlock::Create(context, "check_newline", currentFunc);
                llvm::BasicBlock* trimBB = llvm::BasicBlock::Create(context, "trim_newline", currentFunc);
                llvm::BasicBlock* noTrimBB = llvm::BasicBlock::Create(context, "no_trim_newline", currentFunc);
                llvm::BasicBlock* doneBB = llvm::BasicBlock::Create(context, "input_done", currentFunc);
                
                builder.CreateCondBr(lenNotZero, checkNewlineBB, noTrimBB);
                
                builder.SetInsertPoint(checkNewlineBB);
                llvm::Value* lastIdx = builder.CreateSub(len, builder.getInt64(1), "last_idx");
                llvm::Value* lastCharPtr = builder.CreateGEP(builder.getInt8Ty(), buffer, lastIdx, "last_char_ptr");
                llvm::Value* lastChar = builder.CreateLoad(builder.getInt8Ty(), lastCharPtr, "last_char");
                llvm::Value* isNewline = builder.CreateICmpEQ(lastChar, builder.getInt8('\n'), "is_newline");
                builder.CreateCondBr(isNewline, trimBB, noTrimBB);
                
                builder.SetInsertPoint(trimBB);
                builder.CreateStore(builder.getInt8('\0'), lastCharPtr);
                builder.CreateBr(doneBB);
                
                builder.SetInsertPoint(noTrimBB);
                builder.CreateBr(doneBB);
                
                builder.SetInsertPoint(doneBB);
                value = buffer;
            }
        }
    }
    
    if (!value) {
        value = codegen(stmt->value.get());
    }
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
                if (valueType->isPointerTy() || dynamic_cast<ArrayRangeExpression*>(stmt->value.get())) {
                    if (stmt->declaredType == VarType::INT) {
                        actualType = VarType::ARRAY_INT;
                    } else if (stmt->declaredType == VarType::LONG) {
                        actualType = VarType::ARRAY_LONG;
                    } else if (stmt->declaredType == VarType::FLOAT) {
                        actualType = VarType::ARRAY_FLOAT;
                    } else if (stmt->declaredType == VarType::BOOL) {
                        actualType = VarType::ARRAY_BOOL;
                    } else if (stmt->declaredType == VarType::STRING) {
                        actualType = VarType::ARRAY_STRING;
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
                } else if (valueType->isPointerTy() || dynamic_cast<ArrayRangeExpression*>(stmt->value.get())) {
                    valueType = llvm::PointerType::get(context, 0);
                    if (auto* arrLit = dynamic_cast<ArrayLiteral*>(stmt->value.get())) {
                        if (!arrLit->elements.empty()) {
                            if (dynamic_cast<StringLiteral*>(arrLit->elements[0].get())) {
                                localTypes[stmt->name] = VarType::ARRAY_STRING;
                            } else if (auto* numLit = dynamic_cast<NumberLiteral*>(arrLit->elements[0].get())) {
                                constexpr int64_t INT32_MAX_VAL = 2147483647LL;
                                if (numLit->value > INT32_MAX_VAL || numLit->value < -INT32_MAX_VAL - 1) {
                                    localTypes[stmt->name] = VarType::ARRAY_LONG;
                                } else {
                                    localTypes[stmt->name] = VarType::ARRAY_INT;
                                }
                            } else {
                                localTypes[stmt->name] = VarType::ARRAY_INT;
                            }
                        } else {
                            localTypes[stmt->name] = VarType::ARRAY_INT;
                        }
                    } else {
                        localTypes[stmt->name] = VarType::ARRAY_INT;
                    }
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
    VarType exprType = VarType::UNKNOWN;
    if (auto* strLit = dynamic_cast<StringLiteral*>(stmt->expr.get())) {
        exprType = VarType::STRING;
    } else if (auto* numLit = dynamic_cast<NumberLiteral*>(stmt->expr.get())) {
        constexpr int64_t INT32_MAX_VAL = 2147483647LL;
        if (numLit->value > INT32_MAX_VAL || numLit->value < -INT32_MAX_VAL - 1) {
            exprType = VarType::LONG;
        } else {
            exprType = VarType::INT;
        }
    } else if (auto* floatLit = dynamic_cast<FloatLiteral*>(stmt->expr.get())) {
        exprType = VarType::FLOAT;
    } else if (auto* boolLit = dynamic_cast<BooleanLiteral*>(stmt->expr.get())) {
        exprType = VarType::BOOL;
    } else if (auto* varExpr = dynamic_cast<VariableExpression*>(stmt->expr.get())) {
        auto typeIt = localTypes.find(varExpr->name);
        if (typeIt != localTypes.end()) {
            exprType = typeIt->second;
        } else {
            auto globalTypeIt = windowInputTypes.find(varExpr->name);
            if (globalTypeIt != windowInputTypes.end()) {
                exprType = globalTypeIt->second;
            }
        }
    } else if (auto* callExpr = dynamic_cast<CallExpression*>(stmt->expr.get())) {
        std::string funcName = callExpr->ns.empty() ? callExpr->name : (callExpr->ns + ":" + callExpr->name);
        auto retTypeIt = funcReturnTypes.find(funcName);
        if (retTypeIt != funcReturnTypes.end()) {
            exprType = retTypeIt->second;
        } else if (callExpr->name == "input") {
            exprType = VarType::STRING;
        }
    } else if (auto* arrIdx = dynamic_cast<ArrayIndexExpression*>(stmt->expr.get())) {
        if (auto* varExpr = dynamic_cast<VariableExpression*>(arrIdx->array.get())) {
            auto typeIt = localTypes.find(varExpr->name);
            if (typeIt != localTypes.end()) {
                VarType arrType = typeIt->second;
                if (arrType == VarType::ARRAY_STRING) {
                    exprType = VarType::STRING;
                } else if (arrType == VarType::ARRAY_INT) {
                    exprType = VarType::INT;
                } else if (arrType == VarType::ARRAY_LONG) {
                    exprType = VarType::LONG;
                } else if (arrType == VarType::ARRAY_FLOAT) {
                    exprType = VarType::FLOAT;
                } else if (arrType == VarType::ARRAY_BOOL) {
                    exprType = VarType::BOOL;
                }
            }
        }
    } else if (auto* binOp = dynamic_cast<BinaryOp*>(stmt->expr.get())) {
        if (binOp->op == BinaryOpType::ADD) {
            exprType = VarType::STRING;
        }
    }
    
    llvm::Value* arg = codegen(stmt->expr.get());
    if (!arg) return nullptr;

    if (hasWindowStatements) {
        llvm::Function* messageBoxFunc = module->getFunction("MessageBoxA");
        llvm::Function* snprintfFunc = module->getFunction("snprintf");
        llvm::Function* appendBoxFunc = module->getFunction("xr_append_box");
        llvm::Value* titlePtr = builder.CreateGlobalStringPtr("xfawa print", "print_msgbox_title");
        llvm::Value* nullPtr = llvm::Constant::getNullValue(builder.getInt8Ty()->getPointerTo());
        llvm::Value* textPtr = nullptr;

        if (arg->getType()->isPointerTy() && exprType == VarType::STRING) {
            textPtr = builder.CreateBitCast(arg, builder.getInt8Ty()->getPointerTo(), "window_print_text_ptr");
        } else {
            llvm::AllocaInst* buffer = builder.CreateAlloca(builder.getInt8Ty(), builder.getInt32(256), "print_buffer");
            llvm::Value* bufferPtr = builder.CreateBitCast(buffer, builder.getInt8Ty()->getPointerTo(), "print_buffer_ptr");
            llvm::Value* formatPtr;
            llvm::Value* printArg = arg;

            if (arg->getType()->isFloatTy()) {
                formatPtr = builder.CreateGlobalStringPtr("%f", "print_format_float");
                printArg = builder.CreateFPExt(arg, builder.getDoubleTy(), "print_float_ext");
            } else if (arg->getType()->isIntegerTy(1)) {
                formatPtr = builder.CreateGlobalStringPtr("%d", "print_format_bool");
                printArg = builder.CreateZExtOrTrunc(arg, builder.getInt32Ty(), "print_bool_ext");
            } else if (arg->getType()->isIntegerTy(64)) {
                formatPtr = builder.CreateGlobalStringPtr("%lld", "print_format_long");
            } else if (arg->getType()->isPointerTy()) {
                if (exprType == VarType::LONG || exprType == VarType::UNKNOWN) {
                    formatPtr = builder.CreateGlobalStringPtr("%lld", "print_format_long");
                    printArg = builder.CreatePtrToInt(arg, builder.getInt64Ty(), "ptrtolong");
                } else if (exprType == VarType::INT) {
                    formatPtr = builder.CreateGlobalStringPtr("%d", "print_format_int");
                    printArg = builder.CreatePtrToInt(arg, builder.getInt32Ty(), "ptrtoint");
                } else if (exprType == VarType::BOOL) {
                    formatPtr = builder.CreateGlobalStringPtr("%d", "print_format_bool");
                    printArg = builder.CreatePtrToInt(arg, builder.getInt32Ty(), "ptrtobool");
                } else if (exprType == VarType::FLOAT) {
                    formatPtr = builder.CreateGlobalStringPtr("%f", "print_format_float");
                    printArg = builder.CreatePtrToInt(arg, builder.getInt64Ty(), "ptrtofloat");
                    printArg = builder.CreateSIToFP(printArg, builder.getDoubleTy(), "inttofp");
                } else {
                    formatPtr = builder.CreateGlobalStringPtr("%s", "print_format_str");
                    printArg = arg;
                }
            } else {
                formatPtr = builder.CreateGlobalStringPtr("%d", "print_format_int");
                if (!arg->getType()->isIntegerTy(32)) {
                    printArg = builder.CreateSExtOrTrunc(arg, builder.getInt32Ty(), "print_int_cast");
                }
            }

            builder.CreateCall(snprintfFunc, {bufferPtr, builder.getInt64(256), formatPtr, printArg}, "snprintfcall");
            textPtr = bufferPtr;
        }

        if (!stmt->outputTarget.empty() && activeWindowId >= 0 && appendBoxFunc) {
            llvm::Value* boxIdPtr = builder.CreateGlobalStringPtr(
                stmt->outputTarget,
                "xr_window_box_id_" + std::to_string(activeWindowId) + "_" + std::to_string(stmt->location.line) + "_" + std::to_string(stmt->location.column));
            llvm::Value* appendResult = builder.CreateCall(appendBoxFunc, {boxIdPtr, textPtr}, "append_box_result");
            llvm::Value* appended = builder.CreateICmpNE(appendResult, builder.getInt32(0), "append_box_ok");

            llvm::BasicBlock* appendOkBB = llvm::BasicBlock::Create(context, "append_box_ok", builder.GetInsertBlock()->getParent());
            llvm::BasicBlock* appendFallbackBB = llvm::BasicBlock::Create(context, "append_box_fallback", builder.GetInsertBlock()->getParent());
            llvm::BasicBlock* appendContinueBB = llvm::BasicBlock::Create(context, "append_box_continue", builder.GetInsertBlock()->getParent());

            builder.CreateCondBr(appended, appendOkBB, appendFallbackBB);

            builder.SetInsertPoint(appendOkBB);
            builder.CreateBr(appendContinueBB);

            builder.SetInsertPoint(appendFallbackBB);
            builder.CreateCall(messageBoxFunc, {nullPtr, textPtr, titlePtr, builder.getInt32(0)}, "msgboxcall");
            builder.CreateBr(appendContinueBB);

            builder.SetInsertPoint(appendContinueBB);
            return builder.getInt32(1);
        }

        return builder.CreateCall(messageBoxFunc, {nullPtr, textPtr, titlePtr, builder.getInt32(0)}, "msgboxcall");
    }

    llvm::Function* printfFunc = module->getFunction("printf");
    if (printfFunc) {
        llvm::Value* formatPtr;
        llvm::Value* printArg = arg;
        
        if (arg->getType()->isFloatTy()) {
            formatPtr = builder.CreateGlobalStringPtr("%f\n", "format");
            printArg = builder.CreateFPExt(arg, builder.getDoubleTy(), "float.ext");
        } else if (arg->getType()->isIntegerTy(1)) {
            formatPtr = builder.CreateGlobalStringPtr("%d\n", "format");
            printArg = builder.CreateZExtOrTrunc(arg, builder.getInt32Ty(), "bool.ext");
        } else if (arg->getType()->isIntegerTy(64)) {
            formatPtr = builder.CreateGlobalStringPtr("%lld\n", "format");
        } else if (arg->getType()->isIntegerTy()) {
            formatPtr = builder.CreateGlobalStringPtr("%d\n", "format");
        } else if (arg->getType()->isPointerTy()) {
            if (exprType == VarType::LONG || exprType == VarType::UNKNOWN) {
                formatPtr = builder.CreateGlobalStringPtr("%lld\n", "format");
                printArg = builder.CreatePtrToInt(arg, builder.getInt64Ty(), "ptrtolong");
            } else if (exprType == VarType::INT) {
                formatPtr = builder.CreateGlobalStringPtr("%d\n", "format");
                printArg = builder.CreatePtrToInt(arg, builder.getInt32Ty(), "ptrtoint");
            } else if (exprType == VarType::BOOL) {
                formatPtr = builder.CreateGlobalStringPtr("%d\n", "format");
                printArg = builder.CreatePtrToInt(arg, builder.getInt32Ty(), "ptrtobool");
            } else if (exprType == VarType::FLOAT) {
                formatPtr = builder.CreateGlobalStringPtr("%f\n", "format");
                printArg = builder.CreatePtrToInt(arg, builder.getInt64Ty(), "ptrtofloat");
                printArg = builder.CreateSIToFP(printArg, builder.getDoubleTy(), "inttofp");
            } else {
                formatPtr = builder.CreateGlobalStringPtr("%s\n", "format");
            }
        } else {
            formatPtr = builder.CreateGlobalStringPtr("%d\n", "format");
        }
        
        builder.CreateCall(printfFunc->getFunctionType(), printfFunc, {formatPtr, printArg}, "printfcall");
        
        llvm::Function* fflushFunc = module->getFunction("fflush");
        if (!fflushFunc) {
            llvm::FunctionType* fflushType = llvm::FunctionType::get(
                builder.getInt32Ty(),
                {builder.getInt8Ty()->getPointerTo()},
                false);
            fflushFunc = llvm::Function::Create(fflushType, llvm::Function::ExternalLinkage, 0, "fflush", module);
        }
        llvm::Value* stdoutPtr = builder.CreateCall(module->getFunction("__acrt_iob_func"), {builder.getInt32(1)}, "stdout");
        builder.CreateCall(fflushFunc, {stdoutPtr}, "fflush_stdout");
        
        return nullptr;
    }
    
    return arg;
}

llvm::Value* LLVMCodegen::codegen(ReturnStatement* stmt) {
    llvm::Value* value = codegen(stmt->value.get());
    if (!value) return nullptr;
    
    llvm::Type* retType = llvm::Type::getInt64Ty(context);
    
    if (value->getType()->isIntegerTy(1)) {
        value = builder.CreateZExt(value, retType, "retbool");
    } else if (value->getType()->isIntegerTy(32)) {
        value = builder.CreateSExt(value, retType, "retint");
    } else if (value->getType()->isFloatTy()) {
        value = builder.CreateFPExt(value, builder.getDoubleTy(), "float.ext");
        value = builder.CreateBitCast(value, retType, "float.bits");
    } else if (value->getType()->isDoubleTy()) {
        value = builder.CreateBitCast(value, retType, "double.bits");
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
    
    if (auto* varExpr = dynamic_cast<VariableExpression*>(stmt->iterable.get())) {
        auto typeIt = localTypes.find(varExpr->name);
        if (typeIt != localTypes.end()) {
            VarType varType = typeIt->second;
            if (varType == VarType::ARRAY_INT || varType == VarType::ARRAY_LONG || 
                varType == VarType::ARRAY_FLOAT || varType == VarType::ARRAY_BOOL || 
                varType == VarType::ARRAY_STRING) {
                if (!iterable->getType()->isPointerTy()) {
                    auto it = locals.find(varExpr->name);
                    if (it != locals.end()) {
                        llvm::AllocaInst* alloca = it->second;
                        iterable = builder.CreateLoad(llvm::PointerType::get(context, 0), alloca, "arr.ptr.load");
                    }
                }
            }
        }
    }
    
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

VarType LLVMCodegen::getExpressionType(Expression* expr) {
    if (auto* strLit = dynamic_cast<StringLiteral*>(expr)) {
        return VarType::STRING;
    } else if (auto* numLit = dynamic_cast<NumberLiteral*>(expr)) {
        constexpr int64_t INT32_MAX_VAL = 2147483647LL;
        if (numLit->value > INT32_MAX_VAL || numLit->value < -INT32_MAX_VAL - 1) {
            return VarType::LONG;
        }
        return VarType::INT;
    } else if (auto* floatLit = dynamic_cast<FloatLiteral*>(expr)) {
        return VarType::FLOAT;
    } else if (auto* boolLit = dynamic_cast<BooleanLiteral*>(expr)) {
        return VarType::BOOL;
    } else if (auto* varExpr = dynamic_cast<VariableExpression*>(expr)) {
        auto typeIt = localTypes.find(varExpr->name);
        if (typeIt != localTypes.end()) {
            return typeIt->second;
        }
    } else if (auto* callExpr = dynamic_cast<CallExpression*>(expr)) {
        return VarType::UNKNOWN;
    } else if (auto* binOp = dynamic_cast<BinaryOp*>(expr)) {
        VarType leftType = getExpressionType(binOp->left.get());
        VarType rightType = getExpressionType(binOp->right.get());
        if (leftType == VarType::STRING || rightType == VarType::STRING) {
            return VarType::STRING;
        }
        if (leftType == VarType::FLOAT || rightType == VarType::FLOAT) {
            return VarType::FLOAT;
        }
        if (leftType == VarType::LONG || rightType == VarType::LONG) {
            return VarType::LONG;
        }
        return VarType::INT;
    }
    return VarType::UNKNOWN;
}

void LLVMCodegen::collectCallArgTypes(Expression* expr) {
    if (auto* callExpr = dynamic_cast<CallExpression*>(expr)) {
        std::string funcName = callExpr->ns.empty() ? callExpr->name : (callExpr->ns + ":" + callExpr->name);
        std::vector<VarType> argTypes;
        for (auto& arg : callExpr->args) {
            argTypes.push_back(getExpressionType(arg.get()));
            collectCallArgTypes(arg.get());
        }
        callArgTypes[funcName] = argTypes;
    } else if (auto* binOp = dynamic_cast<BinaryOp*>(expr)) {
        collectCallArgTypes(binOp->left.get());
        collectCallArgTypes(binOp->right.get());
    } else if (auto* unaryOp = dynamic_cast<UnaryOp*>(expr)) {
        collectCallArgTypes(unaryOp->expr.get());
    } else if (auto* arrLit = dynamic_cast<ArrayLiteral*>(expr)) {
        for (auto& elem : arrLit->elements) {
            collectCallArgTypes(elem.get());
        }
    } else if (auto* arrIdx = dynamic_cast<ArrayIndexExpression*>(expr)) {
        collectCallArgTypes(arrIdx->array.get());
        collectCallArgTypes(arrIdx->index.get());
    } else if (auto* arrRange = dynamic_cast<ArrayRangeExpression*>(expr)) {
        if (arrRange->array) {
            collectCallArgTypes(arrRange->array.get());
        }
        collectCallArgTypes(arrRange->start.get());
        collectCallArgTypes(arrRange->end.get());
    }
}

void LLVMCodegen::collectCallArgTypes(Statement* stmt) {
    if (auto* exprStmt = dynamic_cast<ExpressionStatement*>(stmt)) {
        collectCallArgTypes(exprStmt->expr.get());
    } else if (auto* assignStmt = dynamic_cast<AssignmentStatement*>(stmt)) {
        collectCallArgTypes(assignStmt->value.get());
    } else if (auto* printStmt = dynamic_cast<PrintStatement*>(stmt)) {
        collectCallArgTypes(printStmt->expr.get());
    } else if (auto* returnStmt = dynamic_cast<ReturnStatement*>(stmt)) {
        collectCallArgTypes(returnStmt->value.get());
    } else if (auto* blockStmt = dynamic_cast<BlockStatement*>(stmt)) {
        for (auto& s : blockStmt->statements) {
            collectCallArgTypes(s.get());
        }
    } else if (auto* ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        collectCallArgTypes(ifStmt->condition.get());
        collectCallArgTypes(ifStmt->thenBranch.get());
        if (ifStmt->elseBranch) {
            collectCallArgTypes(ifStmt->elseBranch.get());
        }
        for (auto& elseIf : ifStmt->elseIfBranches) {
            collectCallArgTypes(elseIf.first.get());
            collectCallArgTypes(elseIf.second.get());
        }
    } else if (auto* whileStmt = dynamic_cast<WhileStatement*>(stmt)) {
        collectCallArgTypes(whileStmt->condition.get());
        collectCallArgTypes(whileStmt->body.get());
    } else if (auto* forInStmt = dynamic_cast<ForInStatement*>(stmt)) {
        collectCallArgTypes(forInStmt->iterable.get());
        collectCallArgTypes(forInStmt->body.get());
    }
}

void LLVMCodegen::collectCallArgTypes(Program* program) {
    for (auto& mod : program->modules) {
        for (auto& func : mod->functions) {
            bool isMain = (func->name == "main");
            std::string funcName = isMain ? func->name : (func->ns.empty() ? func->name : (func->ns + ":" + func->name));
            
            if (func->body) {
                collectCallArgTypes(func->body.get());
                
                if (auto* blockStmt = dynamic_cast<BlockStatement*>(func->body.get())) {
                    for (auto& stmt : blockStmt->statements) {
                        if (auto* returnStmt = dynamic_cast<ReturnStatement*>(stmt.get())) {
                            VarType retType = getExpressionType(returnStmt->value.get());
                            funcReturnTypes[funcName] = retType;
                            break;
                        }
                    }
                }
            }
        }
    }
}

bool LLVMCodegen::codegenProgram(Program* program) {
    collectCallArgTypes(program);
    
    for (auto& imp : program->imports) {
        if (!codegen(imp.get())) {
            return false;
        }
    }
    
    for (auto& mod : program->modules) {
        for (auto& func : mod->functions) {
            std::vector<llvm::Type*> paramTypes;
            for (size_t i = 0; i < func->params.size(); i++) {
                paramTypes.push_back(builder.getPtrTy());
            }
            
            bool isMain = (func->name == "main");
            std::string funcName = isMain ? func->name : (func->ns.empty() ? func->name : (func->ns + ":" + func->name));
            llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getInt64Ty(context), paramTypes, false);
            
            llvm::Function* llvmFunc = llvm::Function::Create(funcType, llvm::Function::LinkageTypes::ExternalLinkage, 0, funcName, module);
            
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
    auto savedLocals = locals;
    auto savedLocalTypes = localTypes;
    auto savedArrayLengths = arrayLengths;
    llvm::BasicBlock* savedInsertBlock = builder.GetInsertBlock();
    llvm::Function* savedInsertFunction = savedInsertBlock ? savedInsertBlock->getParent() : nullptr;
    
    locals.clear();
    localTypes.clear();
    
    bool isMain = (func->name == "main");
    std::string funcName = isMain ? func->name : (func->ns.empty() ? func->name : (func->ns + ":" + func->name));
    llvm::Function* llvmFunc = module->getFunction(funcName);
    if (!llvmFunc) {
        std::vector<llvm::Type*> paramTypes;
        for (size_t i = 0; i < func->params.size(); i++) {
            paramTypes.push_back(builder.getPtrTy());
        }
        
        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getInt64Ty(context), paramTypes, false);
        
        llvmFunc = llvm::Function::Create(funcType, llvm::Function::LinkageTypes::ExternalLinkage, 0, funcName, module);
        
        if (isMain) {
            hasMainFunction = true;
        }
    }
    
    if (llvmFunc->empty()) {
        llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", llvmFunc);
        builder.SetInsertPoint(entryBB);
        
        if (isMain) {
            llvm::Function* setvbufFunc = module->getFunction("setvbuf");
            if (setvbufFunc) {
                llvm::Value* stdoutPtr = builder.CreateCall(module->getFunction("__acrt_iob_func"), {builder.getInt32(1)}, "stdout");
                llvm::Value* nullBuf = llvm::ConstantPointerNull::get(builder.getInt8Ty()->getPointerTo());
                builder.CreateCall(setvbufFunc, {stdoutPtr, nullBuf, builder.getInt32(4), builder.getInt64(0)});
            }
        }
        
        if (isMain && usesRandomBuiltin) {
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
        auto argTypesIt = callArgTypes.find(funcName);
        for (auto& arg : llvmFunc->args()) {
            std::string paramName = func->params[i]->name;
            llvm::AllocaInst* alloca = builder.CreateAlloca(builder.getPtrTy(), nullptr, paramName.c_str());
            builder.CreateStore(&arg, alloca);
            locals[paramName] = alloca;
            
            VarType paramType = VarType::UNKNOWN;
            if (argTypesIt != callArgTypes.end() && i < argTypesIt->second.size()) {
                paramType = argTypesIt->second[i];
            }
            localTypes[paramName] = paramType;
            i++;
        }
        
        if (func->body) {
            codegen(func->body.get());
        }

        if (builder.GetInsertBlock() && !builder.GetInsertBlock()->getTerminator()) {
            builder.CreateRet(createConstInt(context, llvm::Type::getInt64Ty(context), 0));
        }
    }
    
    locals = savedLocals;
    localTypes = savedLocalTypes;
    arrayLengths = savedArrayLengths;
    
    if (savedInsertBlock && savedInsertFunction) {
        builder.SetInsertPoint(savedInsertBlock);
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
    if (dynamic_cast<ButtonStatement*>(stmt)) return codegen(dynamic_cast<ButtonStatement*>(stmt)) != nullptr;
    if (dynamic_cast<TextStatement*>(stmt)) return codegen(dynamic_cast<TextStatement*>(stmt)) != nullptr;
    if (dynamic_cast<BoxStatement*>(stmt)) return codegen(dynamic_cast<BoxStatement*>(stmt)) != nullptr;
    if (dynamic_cast<InputStatement*>(stmt)) return codegen(dynamic_cast<InputStatement*>(stmt)) != nullptr;
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

llvm::GlobalVariable* LLVMCodegen::getWindowHandleGlobal(int windowId) {
    std::string globalName = "__xfawa_window_handle_" + std::to_string(windowId);
    if (llvm::GlobalVariable* existing = module->getNamedGlobal(globalName)) {
        return existing;
    }

    llvm::Type* ptrTy = builder.getInt8Ty()->getPointerTo();
    return new llvm::GlobalVariable(
        *module,
        ptrTy,
        false,
        llvm::GlobalValue::InternalLinkage,
        llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptrTy)),
        globalName);
}

llvm::Function* LLVMCodegen::createButtonHandler(ButtonStatement* buttonStmt, int windowId, int buttonId, int printWindowId) {
    llvm::FunctionType* handlerTy = llvm::FunctionType::get(builder.getVoidTy(), false);
    llvm::Function* handler = llvm::Function::Create(
        handlerTy,
        llvm::Function::LinkageTypes::InternalLinkage,
        "__xfawa_button_handler_" + std::to_string(windowId) + "_" + std::to_string(buttonId),
        module);

    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", handler);

    auto savedLocals = locals;
    auto savedLocalTypes = localTypes;
    auto savedArrayLengths = arrayLengths;
    auto savedLoopEndBB = loopEndBB;
    int savedActiveWindowId = activeWindowId;
    llvm::IRBuilderBase::InsertPoint savedInsertPoint = builder.saveIP();

    locals.clear();
    localTypes.clear();
    arrayLengths.clear();
    loopEndBB = nullptr;
    activeWindowId = printWindowId;
    builder.SetInsertPoint(entryBB);

    for (auto& stmt : buttonStmt->body) {
        if (!codegen(stmt.get())) {
            break;
        }
        if (builder.GetInsertBlock()->getTerminator()) {
            break;
        }
    }

    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateRetVoid();
    }

    builder.restoreIP(savedInsertPoint);
    locals = std::move(savedLocals);
    localTypes = std::move(savedLocalTypes);
    arrayLengths = std::move(savedArrayLengths);
    loopEndBB = savedLoopEndBB;
    activeWindowId = savedActiveWindowId;

    return handler;
}

llvm::Function* LLVMCodegen::createWindowProc(WindowStatement* windowDecl, int windowId, const std::vector<llvm::Function*>& buttonHandlers) {
    llvm::Type* ptrTy = builder.getInt8Ty()->getPointerTo();
    llvm::Type* i32Ty = builder.getInt32Ty();
    llvm::Type* i64Ty = builder.getInt64Ty();

    llvm::FunctionType* wndProcTy = llvm::FunctionType::get(i64Ty, {ptrTy, i32Ty, i64Ty, i64Ty}, false);
    llvm::Function* wndProc = llvm::Function::Create(
        wndProcTy,
        llvm::Function::LinkageTypes::ExternalLinkage,
        "__xfawa_window_proc_" + std::to_string(windowId),
        module);
    wndProc->setCallingConv(llvm::CallingConv::Win64);
    wndProc->addFnAttr(llvm::Attribute::NoInline);

    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", wndProc);
    llvm::BasicBlock* paintBB = llvm::BasicBlock::Create(context, "paint", wndProc);
    llvm::BasicBlock* commandBB = llvm::BasicBlock::Create(context, "command", wndProc);
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
    llvm::Value* isCommand = builder.CreateICmpEQ(msg, builder.getInt32(273), "is_command");
    llvm::Value* isDestroy = builder.CreateICmpEQ(msg, builder.getInt32(2), "is_destroy");
    llvm::BasicBlock* destroyOrDefaultBB = llvm::BasicBlock::Create(context, "destroy_or_default", wndProc);
    builder.CreateCondBr(isPaint, paintBB, destroyOrDefaultBB);

    builder.SetInsertPoint(destroyOrDefaultBB);
    llvm::BasicBlock* commandOrDefaultBB = llvm::BasicBlock::Create(context, "command_or_default", wndProc);
    builder.CreateCondBr(isCommand, commandBB, commandOrDefaultBB);

    builder.SetInsertPoint(commandOrDefaultBB);
    builder.CreateCondBr(isDestroy, destroyBB, defaultBB);

    builder.SetInsertPoint(paintBB);
    llvm::Function* defWindowProcFunc = module->getFunction("DefWindowProcA");
    builder.CreateRet(builder.CreateCall(defWindowProcFunc, {hwnd, msg, wparam, lparam}, "paint_defproc"));

    builder.SetInsertPoint(commandBB);
    if (buttonHandlers.empty()) {
        builder.CreateRet(builder.getInt64(0));
    } else {
        llvm::Value* commandWord = builder.CreateTrunc(wparam, i32Ty, "command_word");
        llvm::Value* commandId = builder.CreateAnd(commandWord, builder.getInt32(0xFFFF), "command_id");

        llvm::BasicBlock* nextCheckBB = nullptr;
        for (size_t i = 0; i < buttonHandlers.size(); ++i) {
            llvm::BasicBlock* handlerBB = llvm::BasicBlock::Create(context, "button_handler_" + std::to_string(i), wndProc);
            nextCheckBB = llvm::BasicBlock::Create(context, "button_next_" + std::to_string(i), wndProc);
            llvm::Value* matches = builder.CreateICmpEQ(commandId, builder.getInt32(1000 + static_cast<int>(i)), "button_match_" + std::to_string(i));
            builder.CreateCondBr(matches, handlerBB, nextCheckBB);

            builder.SetInsertPoint(handlerBB);
            builder.CreateCall(buttonHandlers[i], {});
            builder.CreateRet(builder.getInt64(0));

            builder.SetInsertPoint(nextCheckBB);
        }

        builder.CreateRet(builder.getInt64(0));
    }

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
    llvm::Function* freeConsoleFunc = module->getFunction("FreeConsole");
    llvm::Function* loadCursorFunc = module->getFunction("LoadCursorA");
    llvm::Function* registerClassFunc = module->getFunction("RegisterClassA");
    llvm::Function* createWindowFunc = module->getFunction("CreateWindowExA");
    llvm::Function* createSolidBrushFunc = module->getFunction("CreateSolidBrush");
    llvm::Function* messageBoxFunc = module->getFunction("MessageBoxA");
    llvm::Function* showWindowFunc = module->getFunction("ShowWindow");
    llvm::Function* updateWindowFunc = module->getFunction("UpdateWindow");
    llvm::Value* nullPtr = llvm::Constant::getNullValue(ptrTy);
    if (windowId == 0) {
        builder.CreateCall(freeConsoleFunc, {});
    }
    llvm::Value* hInstance = builder.CreateCall(getModuleHandleFunc, {nullPtr}, "hinstance");
    llvm::Value* arrowCursorId = llvm::ConstantExpr::getIntToPtr(builder.getInt64(32512), llvm::cast<llvm::PointerType>(ptrTy));
    llvm::Value* cursor = builder.CreateCall(loadCursorFunc, {nullPtr, arrowCursorId}, "cursor");
    llvm::Value* backgroundBrush = builder.CreateCall(createSolidBrushFunc, {builder.getInt32(resolveWindowColor(windowDecl->color))}, "background_brush");

    builder.CreateStore(builder.getInt32(3), builder.CreateStructGEP(wndClassTy, wndClass, 0));
    builder.CreateStore(builder.CreateBitCast(wndProc, ptrTy), builder.CreateStructGEP(wndClassTy, wndClass, 1));
    builder.CreateStore(builder.getInt32(0), builder.CreateStructGEP(wndClassTy, wndClass, 2));
    builder.CreateStore(builder.getInt32(0), builder.CreateStructGEP(wndClassTy, wndClass, 3));
    builder.CreateStore(hInstance, builder.CreateStructGEP(wndClassTy, wndClass, 4));
    builder.CreateStore(nullPtr, builder.CreateStructGEP(wndClassTy, wndClass, 5));
    builder.CreateStore(cursor, builder.CreateStructGEP(wndClassTy, wndClass, 6));
    builder.CreateStore(backgroundBrush, builder.CreateStructGEP(wndClassTy, wndClass, 7));
    builder.CreateStore(nullPtr, builder.CreateStructGEP(wndClassTy, wndClass, 8));
    builder.CreateStore(className, builder.CreateStructGEP(wndClassTy, wndClass, 9));

    llvm::Value* registerResult = builder.CreateCall(registerClassFunc, {wndClass}, "register_result");

    llvm::Value* hwnd = builder.CreateCall(
        createWindowFunc,
        {builder.getInt32(0), className, title, builder.getInt32(0x10CF0000),
         builder.getInt32(-2147483648), builder.getInt32(-2147483648),
         builder.getInt32(windowDecl->width), builder.getInt32(windowDecl->height),
         nullPtr, nullPtr, hInstance, nullPtr},
        "hwnd");

    llvm::BasicBlock* creationOkBB = llvm::BasicBlock::Create(context, "window_create_ok", mainFunc);
    llvm::BasicBlock* creationFailBB = llvm::BasicBlock::Create(context, "window_create_fail", mainFunc);
    llvm::Value* registerOk = builder.CreateICmpNE(registerResult, builder.getInt16(0), "register_ok");
    llvm::Value* hwndOk = builder.CreateICmpNE(hwnd, nullPtr, "hwnd_ok");
    llvm::Value* windowReady = builder.CreateAnd(registerOk, hwndOk, "window_ready");
    builder.CreateCondBr(windowReady, creationOkBB, creationFailBB);

    builder.SetInsertPoint(creationFailBB);
    llvm::Value* errorTitle = builder.CreateGlobalStringPtr("xfawa window error", "window_error_title_" + std::to_string(windowId));
    llvm::Value* errorText = builder.CreateGlobalStringPtr("Failed to create xfawa window", "window_error_text_" + std::to_string(windowId));
    builder.CreateCall(messageBoxFunc, {nullPtr, errorText, errorTitle, builder.getInt32(0x10)});
    builder.CreateRetVoid();

    builder.SetInsertPoint(creationOkBB);
    builder.CreateCall(showWindowFunc, {hwnd, builder.getInt32(1)});
    builder.CreateCall(updateWindowFunc, {hwnd});

    for (size_t i = 0; i < windowDecl->buttons.size(); ++i) {
        const auto& button = windowDecl->buttons[i];
        llvm::Value* buttonClass = builder.CreateGlobalStringPtr("BUTTON", "button_class_name_" + std::to_string(windowId) + "_" + std::to_string(i));
        llvm::Value* buttonText = builder.CreateGlobalStringPtr(button->text, "button_text_" + std::to_string(windowId) + "_" + std::to_string(i));
        llvm::Value* buttonIdPtr = llvm::ConstantExpr::getIntToPtr(builder.getInt64(1000 + static_cast<int>(i)), llvm::cast<llvm::PointerType>(ptrTy));
        builder.CreateCall(
            createWindowFunc,
            {builder.getInt32(0), buttonClass, buttonText, builder.getInt32(0x50000000),
             builder.getInt32(button->x), builder.getInt32(button->y),
             builder.getInt32(button->width), builder.getInt32(button->height),
             hwnd, buttonIdPtr, hInstance, nullPtr});
    }

    for (size_t i = 0; i < windowDecl->texts.size(); ++i) {
        const auto& textItem = windowDecl->texts[i];
        llvm::Value* textClass = builder.CreateGlobalStringPtr("STATIC", "text_class_name_" + std::to_string(windowId) + "_" + std::to_string(i));
        llvm::Value* textValue = builder.CreateGlobalStringPtr(textItem->text, "text_value_" + std::to_string(windowId) + "_" + std::to_string(i));
        builder.CreateCall(
            createWindowFunc,
            {builder.getInt32(0), textClass, textValue, builder.getInt32(0x50000000),
             builder.getInt32(textItem->x), builder.getInt32(textItem->y),
             builder.getInt32(textItem->width), builder.getInt32(textItem->height),
             hwnd, nullPtr, hInstance, nullPtr});
    }

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
    llvm::Type* ptrTy = builder.getInt8Ty()->getPointerTo();
    llvm::Function* createWindowFunc = module->getFunction("xr_create_window");
    llvm::Function* showWindowFunc = module->getFunction("xr_show_window");
    llvm::Function* loadStyleFunc = module->getFunction("xr_load_style");
    llvm::Function* pollEventsFunc = module->getFunction("xr_poll_events");
    llvm::Function* shouldCloseFunc = module->getFunction("xr_should_close");
    llvm::Function* beginFrameFunc = module->getFunction("xr_begin_frame");
    llvm::Function* endFrameFunc = module->getFunction("xr_end_frame");
    llvm::Function* drawButtonFunc = module->getFunction("xr_draw_button");
    llvm::Function* drawBoxFunc = module->getFunction("xr_draw_box");
    llvm::Function* drawTextFunc = module->getFunction("xr_draw_text");
    llvm::Function* drawRectFunc = module->getFunction("xr_draw_rect");
    llvm::Function* setClearColorFunc = module->getFunction("xr_set_clear_color");

    llvm::Function* currentFunction = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* loopCondBB = llvm::BasicBlock::Create(context, "xr_loop_cond_" + std::to_string(windowId), currentFunction);
    llvm::BasicBlock* loopBodyBB = llvm::BasicBlock::Create(context, "xr_loop_body_" + std::to_string(windowId), currentFunction);
    llvm::BasicBlock* loopEndBB = llvm::BasicBlock::Create(context, "xr_loop_end_" + std::to_string(windowId), currentFunction);

    for (const auto& input : windowStmt->inputs) {
        if (!input->varName.empty()) {
            std::string globalName = "__xfawa_input_" + input->varName;
            llvm::GlobalVariable* inputGlobal = new llvm::GlobalVariable(
                *module,
                llvm::PointerType::get(context, 0),
                false,
                llvm::GlobalValue::InternalLinkage,
                llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0)),
                globalName);
            windowInputGlobals[input->varName] = inputGlobal;
            windowInputTypes[input->varName] = VarType::STRING;
        }
    }

    std::vector<llvm::Function*> buttonHandlers;
    buttonHandlers.reserve(windowStmt->buttons.size());
    for (size_t i = 0; i < windowStmt->buttons.size(); ++i) {
        buttonHandlers.push_back(createButtonHandler(windowStmt->buttons[i].get(), windowId, 1000 + static_cast<int>(i), windowId));
    }

    llvm::Value* titlePtr = builder.CreateGlobalStringPtr(windowStmt->title, "xr_window_title_" + std::to_string(windowId));
    builder.CreateCall(createWindowFunc, {builder.getInt32(windowStmt->width), builder.getInt32(windowStmt->height), titlePtr});
    builder.CreateCall(setClearColorFunc, {builder.getInt32(resolveWindowColor(windowStmt->color))});

    if (!windowStmt->style.empty()) {
        llvm::Value* stylePtr = builder.CreateGlobalStringPtr(windowStmt->style, "xr_window_style_" + std::to_string(windowId));
        builder.CreateCall(loadStyleFunc, {stylePtr});
    }

    builder.CreateCall(showWindowFunc, {});
    builder.CreateBr(loopCondBB);

    builder.SetInsertPoint(loopCondBB);
    llvm::Value* shouldClose = builder.CreateCall(shouldCloseFunc, {}, "xr_should_close");
    llvm::Value* continueLoop = builder.CreateICmpEQ(shouldClose, builder.getInt32(0), "xr_continue_loop");
    builder.CreateCondBr(continueLoop, loopBodyBB, loopEndBB);

    builder.SetInsertPoint(loopBodyBB);
    builder.CreateCall(pollEventsFunc, {});
    builder.CreateCall(beginFrameFunc, {});

    for (size_t i = 0; i < windowStmt->texts.size(); ++i) {
        const auto& textItem = windowStmt->texts[i];
        llvm::Value* textPtr = builder.CreateGlobalStringPtr(
            textItem->text,
            "xr_window_text_" + std::to_string(windowId) + "_" + std::to_string(i));
        builder.CreateCall(
            drawTextFunc,
            {
                builder.getInt32(textItem->x),
                builder.getInt32(textItem->y),
                textPtr,
                builder.getInt32(0x00000000)
            });
    }

    for (size_t i = 0; i < windowStmt->boxes.size(); ++i) {
        const auto& box = windowStmt->boxes[i];
        llvm::Value* boxIdPtr = builder.CreateGlobalStringPtr(
            box->id,
            "xr_window_box_id_" + std::to_string(windowId) + "_" + std::to_string(i));
        llvm::Value* boxTextPtr = builder.CreateGlobalStringPtr(
            box->text,
            "xr_window_box_text_" + std::to_string(windowId) + "_" + std::to_string(i));
        builder.CreateCall(
            drawBoxFunc,
            {
                builder.getInt32(box->x),
                builder.getInt32(box->y),
                builder.getInt32(box->width),
                builder.getInt32(box->height),
                boxIdPtr,
                boxTextPtr
            });
    }

    for (size_t i = 0; i < windowStmt->buttons.size(); ++i) {
        const auto& button = windowStmt->buttons[i];
        llvm::Value* buttonText = builder.CreateGlobalStringPtr(
            button->text,
            "xr_window_button_" + std::to_string(windowId) + "_" + std::to_string(i));
        llvm::Value* handlerPtr = builder.CreateBitCast(buttonHandlers[i], ptrTy);
        builder.CreateCall(
            drawButtonFunc,
            {
                builder.getInt32(button->x),
                builder.getInt32(button->y),
                builder.getInt32(button->width),
                builder.getInt32(button->height),
                buttonText,
                handlerPtr
            });
    }

    llvm::Function* drawInputFunc = module->getFunction("xr_draw_input");
    for (size_t i = 0; i < windowStmt->inputs.size(); ++i) {
        const auto& input = windowStmt->inputs[i];
        llvm::Value* inputIdPtr = builder.CreateGlobalStringPtr(
            input->id,
            "xr_window_input_id_" + std::to_string(windowId) + "_" + std::to_string(i));
        llvm::Value* inputVarPtr = builder.CreateGlobalStringPtr(
            input->varName,
            "xr_window_input_var_" + std::to_string(windowId) + "_" + std::to_string(i));
        llvm::Value* inputTextPtr = builder.CreateCall(
            drawInputFunc,
            {
                builder.getInt32(input->x),
                builder.getInt32(input->y),
                builder.getInt32(input->width),
                builder.getInt32(input->height),
                inputIdPtr,
                inputVarPtr
            });
        if (!input->varName.empty()) {
            auto globalIt = windowInputGlobals.find(input->varName);
            if (globalIt != windowInputGlobals.end()) {
                builder.CreateStore(inputTextPtr, globalIt->second);
            }
            llvm::AllocaInst* inputStorage = builder.CreateAlloca(
                llvm::PointerType::get(context, 0),
                nullptr,
                "input_storage_" + input->varName);
            builder.CreateStore(inputTextPtr, inputStorage);
            locals[input->varName] = inputStorage;
            localTypes[input->varName] = VarType::STRING;
        }
    }

    builder.CreateCall(endFrameFunc, {});
    builder.CreateBr(loopCondBB);

    builder.SetInsertPoint(loopEndBB);
    
    for (const auto& input : windowStmt->inputs) {
        if (!input->varName.empty()) {
            windowInputGlobals.erase(input->varName);
            windowInputTypes.erase(input->varName);
        }
    }
    
    return builder.getInt32(0);
}

llvm::Value* LLVMCodegen::codegen(ButtonStatement* stmt) {
    addError("button blocks can only appear inside window blocks");
    return nullptr;
}

llvm::Value* LLVMCodegen::codegen(TextStatement* stmt) {
    addError("text blocks can only appear inside window blocks");
    return nullptr;
}

llvm::Value* LLVMCodegen::codegen(BoxStatement* stmt) {
    addError("box blocks can only appear inside window blocks");
    return nullptr;
}

llvm::Value* LLVMCodegen::codegen(InputStatement* stmt) {
    addError("input blocks can only appear inside window blocks");
    return nullptr;
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
    if (hasWindowStatements) {
        if (auto xraphicsLib = getCompilerAdjacentXraphicsLib()) {
            argStorage.push_back(xraphicsLib->string());
        } else {
            addError("Xraphics component missing. Please reinstall xfawa.");
            return false;
        }
    }
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
