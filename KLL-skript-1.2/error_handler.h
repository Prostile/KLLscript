// error_handler.h
#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <vector>
#include <string>
#include <iostream> // ��� ������ ������ �� ���������

// ��������� ��� �������� ���������� �� ������
struct ErrorInfo {
    enum class ErrorType {
        LEXICAL,
        SYNTAX,
        SEMANTIC, // ������, ���������� ��������/�������� �������� (��������, ������������� ����������)
        RUNTIME   // ������ ������� ���������� (��������, ������� �� ����)
    } type;

    std::string message;
    int line;
    int column;

    ErrorInfo(ErrorType t, std::string msg, int l = 0, int c = 0)
        : type(t), message(std::move(msg)), line(l), column(c) {
    }

    // ����� ��� ���������������� ������ ���������� �� ������
    void print() const {
        std::cerr << "Error";
        if (line > 0) {
            std::cerr << " (Line " << line;
            if (column > 0) {
                std::cerr << ", Col " << column;
            }
            std::cerr << ")";
        }
        std::cerr << ": ";

        switch (type) {
        case ErrorType::LEXICAL:  std::cerr << "Lexical: ";   break;
        case ErrorType::SYNTAX:   std::cerr << "Syntax: ";    break;
        case ErrorType::SEMANTIC: std::cerr << "Semantic: ";  break;
        case ErrorType::RUNTIME:  std::cerr << "Runtime: ";   break;
        }
        std::cerr << message << std::endl;
    }
};

// ����� ��� ���������������� ��������� ������
class ErrorHandler {
private:
    std::vector<ErrorInfo> errors;
    bool errorOccurred;

public:
    ErrorHandler() : errorOccurred(false) {}

    // ������ ��� ����������� ������ ������� ����
    void logLexicalError(const std::string& message, int line, int column);
    void logSyntaxError(const std::string& message, int line, int column);
    void logSemanticError(const std::string& message, int line = 0, int column = 0); // ������������� ������ ����� �� ������ ����� ������ ������� �������
    void logRuntimeError(const std::string& message); // ������ ������� ���������� ������ �� ��������� � ���������� ������/������� ��������� ����

    // ��������, ���� �� ���������������� ������
    bool hasErrors() const;

    // ��������� ���������� ������
    size_t getErrorCount() const;

    // ����� ���� ������������������ ������
    void printErrors() const;

    // ������� ������ ������ (����� ������������ ��� �������������� ������ ��� ������)
    void clearErrors();
};

#endif // ERROR_HANDLER_H