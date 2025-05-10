// error_handler.cpp
#include "error_handler.h"

void ErrorHandler::logLexicalError(const std::string& message, int line, int column) {
    errors.emplace_back(ErrorInfo::ErrorType::LEXICAL, message, line, column);
    errorOccurred = true;
}

void ErrorHandler::logSyntaxError(const std::string& message, int line, int column) {
    errors.emplace_back(ErrorInfo::ErrorType::SYNTAX, message, line, column);
    errorOccurred = true;
}

void ErrorHandler::logSemanticError(const std::string& message, int line, int column) {
    errors.emplace_back(ErrorInfo::ErrorType::SEMANTIC, message, line, column);
    errorOccurred = true;
}

void ErrorHandler::logRuntimeError(const std::string& message) {
    errors.emplace_back(ErrorInfo::ErrorType::RUNTIME, message); // Строка/столбец здесь обычно нерелевантны
    errorOccurred = true;
}

bool ErrorHandler::hasErrors() const {
    return errorOccurred;
}

size_t ErrorHandler::getErrorCount() const {
    return errors.size();
}

void ErrorHandler::printErrors() const {
    if (errors.empty()) {
        std::cout << "No errors reported." << std::endl;
        return;
    }
    std::cerr << "--- Error Summary (" << errors.size() << " error(s)) ---" << std::endl;
    for (const auto& error : errors) {
        error.print();
    }
    std::cerr << "-----------------------------" << std::endl;
}

void ErrorHandler::clearErrors() {
    errors.clear();
    errorOccurred = false;
}