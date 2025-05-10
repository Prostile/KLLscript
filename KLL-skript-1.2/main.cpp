// main.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

// Заголовочные файлы будущих модулей
// Пока они не существуют, компиляция этого файла не пройдет,
// но это отражает финальную структуру.
#include "definitions.h"
#include "token.h" // Если Token будет в отдельном файле
#include "error_handler.h"
#include "symbol_table.h"
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "rpn_op.h" // Если RPNOperation будет в отдельном файле


int main(int argc, char* argv[]) {
    // 1. Обработка аргументов командной строки
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }

    std::string sourceFileName = argv[1];
    std::ifstream sourceFile(sourceFileName);

    if (!sourceFile.is_open()) {
        std::cerr << "Error: Could not open file '" << sourceFileName << "'" << std::endl;
        return 1;
    }

    // 2. Чтение исходного кода из файла
    std::stringstream buffer;
    buffer << sourceFile.rdbuf();
    std::string sourceCode = buffer.str();
    sourceFile.close();

    // 3. Инициализация компонентов компилятора
    ErrorHandler errorHandler; // Создаем обработчик ошибок
    SymbolTable symbolTable(errorHandler);   // Создаем таблицу символов
    Lexer lexer(sourceCode, symbolTable, errorHandler); // Создаем лексер
    Parser parser(lexer, symbolTable, errorHandler);     // Создаем парсер

    std::cout << "Starting compilation of file: " << sourceFileName << std::endl;

    // 4. Фаза компиляции (Лексический + Синтаксический анализ + Генерация ОПС)
    bool parseSuccess = parser.parse();

    if (!parseSuccess || errorHandler.hasErrors()) {
        std::cerr << "Compilation failed." << std::endl;
        errorHandler.printErrors(); // Выводим все накопленные ошибки
        return 1; // Завершаем, если парсинг не удался или были ошибки
    }

    std::cout << "Compilation successful. RPN code generated." << std::endl;

    // (Опционально) Вывод сгенерированного ОПС для отладки
    parser.printRPN(); // Этот метод нужно будет реализовать в Parser

    // 5. Фаза интерпретации
    std::cout << "\nStarting execution..." << std::endl;
    std::cout << "---------------------" << std::endl;

    const std::vector<RPNOperation>& rpnCode = parser.getRPNCode();

    // Проверка, что ОПС не пуст, если парсинг был успешен, но ОПС пуст (например, пустая программа)
    if (rpnCode.empty() && parseSuccess) {
        std::cout << "Program is empty. Nothing to execute." << std::endl;
        std::cout << "---------------------" << std::endl;
        std::cout << "Execution finished." << std::endl;
        return 0;
    }


    Interpreter interpreter(rpnCode, symbolTable, errorHandler);

    // Запускаем выполнение
    interpreter.execute();

    if (errorHandler.hasErrors()) {
        std::cerr << "Execution failed with runtime errors." << std::endl;
        errorHandler.printErrors(); // Выводим ошибки времени выполнения
        return 1;
    }

    std::cout << "---------------------" << std::endl;
    std::cout << "Execution finished." << std::endl;

    // (Опционально) Вывод таблицы символов в конце для отладки
    // symbolTable.print(); // Этот метод нужно будет реализовать в SymbolTable

    return 0; // Успешное завершение
}