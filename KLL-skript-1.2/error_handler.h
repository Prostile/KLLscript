// error_handler.h
#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <vector>
#include <string>
#include <iostream> // Для вывода ошибок по умолчанию

// Структура для хранения информации об ошибке
struct ErrorInfo {
    enum class ErrorType {
        LEXICAL,
        SYNTAX,
        SEMANTIC, // Ошибки, выявляемые парсером/таблицей символов (например, необъявленная переменная)
        RUNTIME   // Ошибки времени выполнения (например, деление на ноль)
    } type;

    std::string message;
    int line;
    int column;

    ErrorInfo(ErrorType t, std::string msg, int l = 0, int c = 0)
        : type(t), message(std::move(msg)), line(l), column(c) {
    }

    // Метод для форматированного вывода информации об ошибке
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

// Класс для централизованной обработки ошибок
class ErrorHandler {
private:
    std::vector<ErrorInfo> errors;
    bool errorOccurred;

public:
    ErrorHandler() : errorOccurred(false) {}

    // Методы для регистрации ошибок разного типа
    void logLexicalError(const std::string& message, int line, int column);
    void logSyntaxError(const std::string& message, int line, int column);
    void logSemanticError(const std::string& message, int line = 0, int column = 0); // Семантические ошибки могут не всегда иметь точную позицию символа
    void logRuntimeError(const std::string& message); // Ошибки времени выполнения обычно не привязаны к конкретной строке/столбцу исходного кода

    // Проверка, были ли зарегистрированы ошибки
    bool hasErrors() const;

    // Получение количества ошибок
    size_t getErrorCount() const;

    // Вывод всех зарегистрированных ошибок
    void printErrors() const;

    // Очистка списка ошибок (может понадобиться для интерактивного режима или тестов)
    void clearErrors();
};

#endif // ERROR_HANDLER_H