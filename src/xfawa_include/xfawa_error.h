#ifndef XFAWA_ERROR_H
#define XFAWA_ERROR_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

namespace xfawa {

enum class ErrorType {
    SYNTAX,
    MOD,
    CONFIG
};

enum class ErrorSeverity {
    ERR,
    WARN
};

struct SourcePosition {
    std::string filename;
    int line;
    int column;
    
    SourcePosition(const std::string& file = "", int l = 0, int c = 0)
        : filename(file), line(l), column(c) {}
    
    std::string toString() const {
        if (filename.empty()) {
            return "line " + std::to_string(line) + ", column " + std::to_string(column);
        }
        return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

struct Diagnostic {
    ErrorType type;
    ErrorSeverity severity;
    SourcePosition position;
    std::string message;
    
    Diagnostic(ErrorType t, ErrorSeverity s, const SourcePosition& pos, const std::string& msg)
        : type(t), severity(s), position(pos), message(msg) {}
    
    std::string toString(bool showType = true) const {
        std::ostringstream oss;
        
        if (severity == ErrorSeverity::ERR) {
            oss << "[error";
        } else {
            oss << "[warning";
        }
        
        if (showType) {
            oss << ":" << typeToString();
        }
        
        oss << "] ";
        
        if (position.line > 0) {
            oss << position.toString() << ": ";
        }
        
        oss << message;
        
        return oss.str();
    }
    
private:
    std::string typeToString() const {
        switch (type) {
            case ErrorType::SYNTAX: return "syntax";
            case ErrorType::MOD: return "mod";
            case ErrorType::CONFIG: return "config";
            default: return "unknown";
        }
    }
};

class ErrorSystem {
private:
    std::vector<Diagnostic> diagnostics;
    std::string currentFile;
    bool showWarningTypes;
    bool warningsEnabled;
    int errorCount;
    int warningCount;
    
public:
    ErrorSystem() : showWarningTypes(true), warningsEnabled(true), errorCount(0), warningCount(0) {}
    
    void setCurrentFile(const std::string& file) {
        currentFile = file;
    }
    
    const std::string& getCurrentFile() const {
        return currentFile;
    }
    
    void setShowWarningTypes(bool show) {
        showWarningTypes = show;
    }
    
    void setWarningsEnabled(bool enabled) {
        warningsEnabled = enabled;
    }
    
    void addError(ErrorType type, int line, int column, const std::string& message) {
        SourcePosition pos(currentFile, line, column);
        diagnostics.push_back(Diagnostic(type, ErrorSeverity::ERR, pos, message));
        errorCount++;
    }
    
    void addError(ErrorType type, const std::string& message) {
        SourcePosition pos(currentFile, 0, 0);
        diagnostics.push_back(Diagnostic(type, ErrorSeverity::ERR, pos, message));
        errorCount++;
    }
    
    void addWarning(ErrorType type, int line, int column, const std::string& message) {
        if (!warningsEnabled) return;
        
        SourcePosition pos(currentFile, line, column);
        diagnostics.push_back(Diagnostic(type, ErrorSeverity::WARN, pos, message));
        warningCount++;
    }
    
    void addWarning(ErrorType type, const std::string& message) {
        if (!warningsEnabled) return;
        
        SourcePosition pos(currentFile, 0, 0);
        diagnostics.push_back(Diagnostic(type, ErrorSeverity::WARN, pos, message));
        warningCount++;
    }
    
    void addSyntaxError(int line, int column, const std::string& message) {
        addError(ErrorType::SYNTAX, line, column, message);
    }
    
    void addSyntaxWarning(int line, int column, const std::string& message) {
        addWarning(ErrorType::SYNTAX, line, column, message);
    }
    
    void addModError(int line, int column, const std::string& message) {
        addError(ErrorType::MOD, line, column, message);
    }
    
    void addModWarning(int line, int column, const std::string& message) {
        addWarning(ErrorType::MOD, line, column, message);
    }
    
    void addConfigError(int line, int column, const std::string& message) {
        addError(ErrorType::CONFIG, line, column, message);
    }
    
    void addConfigWarning(int line, int column, const std::string& message) {
        addWarning(ErrorType::CONFIG, line, column, message);
    }
    
    bool hasErrors() const {
        return errorCount > 0;
    }
    
    bool hasWarnings() const {
        return warningCount > 0;
    }
    
    int getErrorCount() const {
        return errorCount;
    }
    
    int getWarningCount() const {
        return warningCount;
    }
    
    const std::vector<Diagnostic>& getDiagnostics() const {
        return diagnostics;
    }
    
    void printDiagnostics(std::ostream& os = std::cerr) const {
        for (const auto& diag : diagnostics) {
            if (diag.severity == ErrorSeverity::WARN) {
                os << diag.toString(showWarningTypes) << std::endl;
            } else {
                os << diag.toString(true) << std::endl;
            }
        }
    }
    
    void clear() {
        diagnostics.clear();
        errorCount = 0;
        warningCount = 0;
    }
    
    std::vector<std::string> getErrorMessages() const {
        std::vector<std::string> messages;
        for (const auto& diag : diagnostics) {
            if (diag.severity == ErrorSeverity::ERR) {
                messages.push_back(diag.toString(true));
            }
        }
        return messages;
    }
    
    std::vector<std::string> getWarningMessages() const {
        std::vector<std::string> messages;
        for (const auto& diag : diagnostics) {
            if (diag.severity == ErrorSeverity::WARN) {
                messages.push_back(diag.toString(showWarningTypes));
            }
        }
        return messages;
    }
    
    std::vector<std::string> getAllMessages() const {
        std::vector<std::string> messages;
        for (const auto& diag : diagnostics) {
            if (diag.severity == ErrorSeverity::WARN) {
                messages.push_back(diag.toString(showWarningTypes));
            } else {
                messages.push_back(diag.toString(true));
            }
        }
        return messages;
    }
};

class ErrorReporter {
private:
    static ErrorSystem* instance;
    
public:
    static ErrorSystem& get() {
        if (!instance) {
            instance = new ErrorSystem();
        }
        return *instance;
    }
    
    static void initialize() {
        if (instance) {
            delete instance;
        }
        instance = new ErrorSystem();
    }
    
    static void cleanup() {
        if (instance) {
            delete instance;
            instance = nullptr;
        }
    }
};

}

#endif
