#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <regex>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#define mkdir(path) _mkdir(path)
#else
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "xfawa_types.h"
#include "xfawa_ast.h"
#include "xfawa_lexer.h"
#include "xfawa_parser.h"
#include "xfawa_llvm_codegen.h"
#include "xfawa_config.h"
#include "xfawa_mods_system.h"
#include "xfawa_error.h"
#include "xfawa_ast_transform.h"
#include "xfawa_semantic_analyzer.h"

static int g_debug = 0;
static bool g_keep_temp = false;
static bool g_emit_llvm = false;
static bool g_emit_asm = false;
static bool g_use_config = true;

namespace xfawa {
    int g_debug_global = 0;
}

const char* COMPILER_VERSION = "1.0.0-a.5";
const char* MODS_KERNEL_VERSION = "mods-a-1.0.1";

xfawa::ErrorSystem* xfawa::ErrorReporter::instance = nullptr;

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << path << std::endl;
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

bool ensureDirectoryExists(const std::string& dirPath) {
#ifdef _WIN32
    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, dirPath.c_str(), -1, NULL, 0);
    if (wchars_num <= 0) return false;
    
    std::wstring wdirPath(wchars_num, 0);
    MultiByteToWideChar(CP_UTF8, 0, dirPath.c_str(), -1, &wdirPath[0], wchars_num);
    
    std::wstring::size_type pos = 0;
    while ((pos = wdirPath.find(L'\\', pos + 1)) != std::wstring::npos) {
        std::wstring subdir = wdirPath.substr(0, pos);
        if (!subdir.empty() && subdir != L":") {
            CreateDirectoryW(subdir.c_str(), NULL);
        }
    }
    
    return CreateDirectoryW(wdirPath.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    struct stat st;
    if (stat(dirPath.c_str(), &st) == -1) {
        if (mkdir(dirPath.c_str(), 0755) == -1) {
            return false;
        }
    }
    return true;
#endif
}

void printUsage(const char* programName) {
    printf("Usage: %s [options] <input_file>\n", programName);
    printf("Options:\n");
    printf("  -o, --output <file>    Specify output file name\n");
    printf("  -d, --debug            Enable debug output\n");
    printf("  -k, --keep             Keep temporary files\n");
    printf("  --emit-llvm            Emit LLVM IR (.ll file)\n");
    printf("  --emit-asm             Emit assembly (.asm file)\n");
    printf("  -c, --config <file>    Specify config file\n");
    printf("  -n, --no-config        Don't use config file\n");
    printf("  -v, --version          Show compiler version\n");
    printf("  -h, --help             Show this help message\n");
    printf("\n");
}

void printVersion() {
    printf("xfawaPL Compiler\n");
    printf("Version: %s\n", COMPILER_VERSION);
    printf("Mods Kernel: %s\n", MODS_KERNEL_VERSION);
    printf("Backend: LLVM 21.1.8\n");
}

void printConfig(const xfawa::CompilerConfig& config) {
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Debug info: " << (config.debug_info ? "yes" : "no") << std::endl;
    std::cout << "  Warnings: " << (config.warnings ? "yes" : "no") << std::endl;
    std::cout << "  Show warning types: " << (config.show_warning_types ? "yes" : "no") << std::endl;
    std::cout << "  Emit LLVM IR: " << (config.emit_ll ? "yes" : "no") << std::endl;
    std::cout << "  Emit ASM: " << (config.emit_asm ? "yes" : "no") << std::endl;
    std::cout << "  Output directory: " << config.output_dir << std::endl;
    std::cout << "  Intermediate directory: " << config.intermediate_dir << std::endl;
}

std::vector<std::string> extractModImports(const std::string& source) {
    std::vector<std::string> mods;
    std::regex importRegex("%import\\s+\"([^\"]+)\"");
    
    std::sregex_iterator it(source.begin(), source.end(), importRegex);
    std::sregex_iterator end;
    
    while (it != end) {
        std::string modName = (*it)[1].str();
        mods.push_back(modName);
        ++it;
    }
    
    return mods;
}

std::string removeImportStatements(const std::string& source) {
    std::regex importRegex("%import\\s+\"[^\"]+\"\\s*\\n?");
    return std::regex_replace(source, importRegex, "");
}

bool processMods(xfawa::ModsSystem& modsSystem, const std::string& source, std::string& processedSource) {
    std::vector<std::string> modImports = extractModImports(source);
    
    if (modImports.empty()) {
        processedSource = source;
        return true;
    }
    
    if (g_debug) {
        std::cout << "[debug] Found " << modImports.size() << " mod import(s)" << std::endl;
        for (const auto& mod : modImports) {
            std::cout << "[debug]   - " << mod << std::endl;
        }
    }
    
    for (const auto& modName : modImports) {
        if (!modsSystem.loadMod(modName)) {
            xfawa::ErrorReporter::get().addModError(0, 0, "Failed to load mod: " + modName);
            for (const auto& err : modsSystem.getErrors()) {
                xfawa::ErrorReporter::get().addModError(0, 0, err);
            }
            return false;
        }
        
        if (g_debug) {
            std::cout << "[debug] Loaded mod: " << modName << std::endl;
            std::cout << "[debug]   Modifications: " << modsSystem.getModifications().size() << std::endl;
            std::cout << "[debug]   Added syntaxes: " << modsSystem.getAddedSyntaxes().size() << std::endl;
            std::cout << "[debug]   Public functions: " << modsSystem.getPublicFunctions().size() << std::endl;
        }
    }
    
    processedSource = removeImportStatements(source);
    
    if (modsSystem.hasModifications()) {
        processedSource = modsSystem.applyModifications(processedSource);
        if (g_debug) {
            std::cout << "[debug] Applied syntax modifications" << std::endl;
        }
    }
    
    if (modsSystem.hasAddedSyntaxes()) {
        processedSource = modsSystem.expandSyntax(processedSource);
        if (g_debug) {
            std::cout << "[debug] Expanded added syntaxes" << std::endl;
        }
    }
    
    if (modsSystem.hasPublicFunctions()) {
        std::string publicFuncsCode;
        publicFuncsCode += "\n#_pub_funcs {\n";
        
        for (const auto& pubFunc : modsSystem.getPublicFunctions()) {
            if (pubFunc.ns.empty()) {
                std::string funcCode = "    fn " + pubFunc.name + "() { " + pubFunc.body + " }\n";
                publicFuncsCode += funcCode;
                if (g_debug) {
                    std::cout << "[debug] Injecting internal function: " << pubFunc.name << std::endl;
                }
            }
        }
        
        for (const auto& pubFunc : modsSystem.getPublicFunctions()) {
            if (!pubFunc.ns.empty()) {
                std::string funcCode = "    fn " + pubFunc.ns + ":" + pubFunc.name + "() { " + pubFunc.body + " }\n";
                publicFuncsCode += funcCode;
                if (g_debug) {
                    std::cout << "[debug] Injecting public function: " << pubFunc.ns << ":" << pubFunc.name << std::endl;
                    std::cout << "[debug]   Body: " << pubFunc.body << std::endl;
                }
            }
        }
        
        publicFuncsCode += "}\n";
        processedSource = publicFuncsCode + processedSource;
        if (g_debug) {
            std::cout << "[debug] Injected functions" << std::endl;
        }
    }
    
    return true;
}

int main(int argc, char** argv) {
    xfawa::ErrorReporter::initialize();
    
    std::string inputFile;
    std::string outputFile = "";
    std::string configFile;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                outputFile = argv[i + 1];
                i++;
            }
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            g_debug = 1;
            xfawa::g_debug_global = 1;
        } else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--keep") == 0) {
            g_keep_temp = true;
        } else if (strcmp(argv[i], "--emit-llvm") == 0) {
            g_emit_llvm = true;
        } else if (strcmp(argv[i], "--emit-asm") == 0) {
            g_emit_asm = true;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 < argc) {
                configFile = argv[i + 1];
                i++;
            }
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-config") == 0) {
            g_use_config = false;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printVersion();
            xfawa::ErrorReporter::cleanup();
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            xfawa::ErrorReporter::cleanup();
            return 0;
        } else if (argv[i][0] != '-') {
            inputFile = argv[i];
        }
    }
    
    if (inputFile.empty()) {
        xfawa::ErrorReporter::get().addSyntaxError(0, 0, "No input file specified");
        printUsage(argv[0]);
        xfawa::ErrorReporter::cleanup();
        return 1;
    }
    
    xfawa::ErrorReporter::get().setCurrentFile(inputFile);
    
    xfawa::CompilerConfig config;
    if (g_use_config) {
        if (!configFile.empty()) {
            config = xfawa::ConfigLoader::loadFromFile(configFile);
        } else {
            config = xfawa::ConfigLoader::load(inputFile);
        }
        
        if (config.debug_info) {
            g_debug = 1;
            xfawa::g_debug_global = 1;
        }
        if (config.emit_ll) {
            g_emit_llvm = true;
        }
        if (config.emit_asm) {
            g_emit_asm = true;
        }
        
        xfawa::ErrorReporter::get().setShowWarningTypes(config.show_warning_types);
        xfawa::ErrorReporter::get().setWarningsEnabled(config.warnings);
    }
    
    if (g_debug) {
        printConfig(config);
    }
    
    ensureDirectoryExists(config.output_dir);
    ensureDirectoryExists(config.intermediate_dir);
    
    std::string source = readFile(inputFile);
    
    if (source.empty()) {
        xfawa::ErrorReporter::cleanup();
        return 1;
    }
    
    xfawa::ModsSystem modsSystem;
    std::string processedSource;
    
    if (!processMods(modsSystem, source, processedSource)) {
        xfawa::ErrorReporter::get().printDiagnostics();
        xfawa::ErrorReporter::cleanup();
        return 1;
    }
    
    if (g_debug && modsSystem.hasModifications()) {
        std::cout << "[debug] Source after mod processing:" << std::endl;
        std::cout << processedSource << std::endl;
    }
    
    if (g_debug && modsSystem.hasAddedSyntaxes()) {
        std::cout << "[debug] Added syntaxes details:" << std::endl;
        for (const auto& syntax : modsSystem.getAddedSyntaxes()) {
            std::cout << "  Name: " << syntax.name << std::endl;
            std::cout << "  Pattern: " << syntax.syntaxPattern << std::endl;
            std::cout << "  Logic: " << syntax.logicCode << std::endl;
            std::cout << "  Parameters: ";
            for (const auto& param : syntax.parameterOrder) {
                std::cout << param << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "[debug] Source after syntax expansion:" << std::endl;
        std::cout << processedSource << std::endl;
    }
    
    xfawa::Lexer lexer(processedSource);
    std::vector<xfawa::Token> tokens = lexer.tokenize();
    
    if (lexer.hasErrors()) {
        for (const auto& error : lexer.getErrors()) {
            xfawa::ErrorReporter::get().addSyntaxError(0, 0, error);
        }
        xfawa::ErrorReporter::get().printDiagnostics();
        xfawa::ErrorReporter::cleanup();
        return 1;
    }
    
    if (g_debug) {
        std::cout << "[debug] Tokens:" << std::endl;
        for (const auto& token : tokens) {
            std::cout << "  " << token.toString() << std::endl;
        }
    }
    
    xfawa::Parser parser(tokens);
    std::unique_ptr<xfawa::Program> program = parser.parseProgram();
    
    if (g_debug) {
        std::cout << "[debug] Parser errors: " << parser.getErrors().size() << std::endl;
        for (const auto& error : parser.getErrors()) {
            std::cout << "[debug]   - " << error << std::endl;
        }
    }
    
    if (parser.hasErrors()) {
        for (const auto& error : parser.getErrors()) {
            xfawa::ErrorReporter::get().addSyntaxError(0, 0, error);
        }
        xfawa::ErrorReporter::get().printDiagnostics();
        xfawa::ErrorReporter::cleanup();
        return 1;
    }
    
    if (parser.hasWarnings()) {
        for (const auto& warning : parser.getWarnings()) {
            xfawa::ErrorReporter::get().addSyntaxWarning(0, 0, warning);
        }
    }
    
    if (!program) {
        xfawa::ErrorReporter::get().addSyntaxError(0, 0, "Failed to parse program");
        xfawa::ErrorReporter::get().printDiagnostics();
        xfawa::ErrorReporter::cleanup();
        return 1;
    }
    
    if (g_debug) {
        std::cout << "[debug] AST:" << std::endl;
        std::cout << program->toString() << std::endl;
    }
    
    xfawa::ASTTransformer transformer;
    transformer.setErrorSystem(&xfawa::ErrorReporter::get());
    
    if (!transformer.transform(program.get())) {
        xfawa::ErrorReporter::get().printDiagnostics();
        xfawa::ErrorReporter::cleanup();
        return 1;
    }
    
    xfawa::SemanticAnalyzer semanticAnalyzer;
    if (!semanticAnalyzer.analyze(program.get())) {
        for (const auto& error : semanticAnalyzer.getErrors()) {
            xfawa::ErrorReporter::get().addSyntaxError(0, 0, error);
        }
        xfawa::ErrorReporter::get().printDiagnostics();
        xfawa::ErrorReporter::cleanup();
        return 1;
    }
    
    if (semanticAnalyzer.hasWarnings()) {
        for (const auto& warning : semanticAnalyzer.getWarnings()) {
            xfawa::ErrorReporter::get().addSyntaxWarning(0, 0, warning);
        }
    }
    
    xfawa::LLVMCodegen::initializeTargets();
    
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module = std::make_unique<llvm::Module>("xfawa_module", context);
    
    xfawa::LLVMCodegen codegen(context, module.get());
    
    if (!codegen.codegenProgram(program.get())) {
        for (const auto& error : codegen.getErrors()) {
            xfawa::ErrorReporter::get().addSyntaxError(0, 0, error);
        }
        xfawa::ErrorReporter::get().printDiagnostics();
        xfawa::ErrorReporter::cleanup();
        return 1;
    }
    
    if (!codegen.verifyModule()) {
        for (const auto& error : codegen.getErrors()) {
            xfawa::ErrorReporter::get().addSyntaxError(0, 0, error);
        }
        xfawa::ErrorReporter::get().printDiagnostics();
        xfawa::ErrorReporter::cleanup();
        return 1;
    }
    
    if (codegen.hasWarnings()) {
        for (const auto& warning : codegen.getWarnings()) {
            xfawa::ErrorReporter::get().addSyntaxWarning(0, 0, warning);
        }
    }
    
    if (g_debug) {
        module->print(llvm::outs(), nullptr);
    }
    
    std::string inputBaseName = inputFile.substr(inputFile.find_last_of("/\\") + 1);
    if (inputBaseName.find('.') != std::string::npos) {
        inputBaseName = inputBaseName.substr(0, inputBaseName.find_last_of('.'));
    }
    
    if (outputFile.empty()) {
        outputFile = config.output_dir + "/" + inputBaseName + ".exe";
    }
    
    std::string outputBaseName = outputFile.substr(outputFile.find_last_of("/\\") + 1);
    if (outputBaseName.find('.') != std::string::npos) {
        outputBaseName = outputBaseName.substr(0, outputBaseName.find_last_of('.'));
    }
    
    std::string objFile = config.intermediate_dir + "/" + outputBaseName + ".o";
    
    std::string llFile = config.output_dir + "/" + outputBaseName + ".exe.ll";
    std::string asmFile = config.output_dir + "/" + outputBaseName + ".exe.asm";
    
    if (!codegen.emitObjectFile(objFile, g_emit_llvm, g_emit_asm, llFile, asmFile)) {
        for (const auto& error : codegen.getErrors()) {
            xfawa::ErrorReporter::get().addSyntaxError(0, 0, error);
        }
        xfawa::ErrorReporter::get().printDiagnostics();
        xfawa::ErrorReporter::cleanup();
        return 1;
    }
    
    if (!codegen.linkExecutable(objFile, outputFile)) {
        for (const auto& error : codegen.getErrors()) {
            xfawa::ErrorReporter::get().addSyntaxError(0, 0, error);
        }
        xfawa::ErrorReporter::get().printDiagnostics();
        xfawa::ErrorReporter::cleanup();
        return 1;
    }
    
    if (!g_keep_temp) {
        std::remove(objFile.c_str());
    }
    
    if (xfawa::ErrorReporter::get().hasWarnings()) {
        xfawa::ErrorReporter::get().printDiagnostics();
    }
    
    std::cout << "Compilation successful: " << outputFile << std::endl;
    
    xfawa::ErrorReporter::cleanup();
    return 0;
}
