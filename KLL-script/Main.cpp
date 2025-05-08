#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream> // Для чтения файла в строку

#include "definitions.h"
#include "DataStructures.h"
#include "SymbolTable.h"
#include "Lexer.h"
#include "Parser.h"
#include "Interpreter.h"
// #include "ErrorHandler.h" // В будущем

int main(int argc, char* argv[]) {
    // --- 1. Обработка аргументов и чтение файла ---
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

    // Читаем весь файл в строку (можно сделать и построчное чтение, если файлы очень большие)
    std::stringstream buffer;
    buffer << sourceFile.rdbuf();
    std::string sourceCode = buffer.str();
    sourceFile.close();

    // Добавляем символ конца файла, если его нет (для удобства лексера)
    // Лексер ожидает '$' как маркер конца
    // sourceCode += '$'; // Лексер сам обрабатывает конец строки

    // --- 2. Инициализация компонентов ---
    // ErrorHandler errorHandler; // Создаем обработчик ошибок (когда он будет)
    SymbolTable symbolTable;   // Создаем таблицу символов
    Lexer lexer(sourceCode, symbolTable /*, errorHandler*/); // Создаем лексер
    Parser parser(lexer, symbolTable /*, errorHandler*/);     // Создаем парсер

    // --- 3. Фаза трансляции (Лексический + Синтаксический анализ + Генерация ОПС) ---
    std::cout << "Starting compilation..." << std::endl;
    bool parseSuccess = parser.parse();

    if (!parseSuccess) {
        std::cerr << "Compilation failed due to syntax errors." << std::endl;
        // errorHandler.printSummary(); // В будущем
        return 1; // Завершаем, если парсинг не удался
    }

    std::cout << "Parsing successful. RPN code generated." << std::endl;

    // (Опционально) Вывод сгенерированного ОПС для отладки
    parser.printRPN();


    // --- 4. Фаза интерпретации ---
    std::cout << "\nStarting execution..." << std::endl;
    std::cout << "---------------------" << std::endl;

    // Получаем сгенерированный код ОПС
    const auto& rpnCode = parser.getRPNCode();

    // Создаем интерпретатор
    Interpreter interpreter(rpnCode, symbolTable /*, errorHandler*/);

    // Запускаем выполнение
    try {
        interpreter.execute();
    }
    catch (const std::exception& e) {
        std::cerr << "Caught exception during execution: " << e.what() << std::endl;
        // errorHandler.logRuntimeError(...) // В будущем
        return 1;
    }
    catch (...) {
        std::cerr << "Caught unknown exception during execution." << std::endl;
        // errorHandler.logRuntimeError(...) // В будущем
        return 1;
    }


    std::cout << "---------------------" << std::endl;
    std::cout << "Execution finished." << std::endl;

    // (Опционально) Вывод таблицы символов в конце для отладки
    // TODO: Добавить метод print() в SymbolTable

    return 0; // Успешное завершение
}