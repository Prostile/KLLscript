#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <vector>
#include <string>

#include "definitions.h"
#include "DataStructures.h" // RPNOperation, RuntimeStack, StackValue, SymbolValue etc.
#include "SymbolTable.h"
// #include "ErrorHandler.h"

// --- Класс Интерпретатора ОПС ---
class Interpreter {
private:
    const std::vector<RPNOperation>& rpnCode; // Ссылка на код ОПС
    SymbolTable& symbolTable;                 // Ссылка на таблицу символов
    // ErrorHandler& errorHandler;             // Ссылка на обработчик ошибок

    RuntimeStack stack;                       // Стек времени выполнения
    int instructionPointer;                   // Указатель на текущую инструкцию ОПС

    // --- Вспомогательные методы ---
    void runtimeError(const std::string& message);

    // Извлечение операндов из стека с проверкой типа
    StackValue popValue(); // Просто извлекает
    int popIntValue();     // Извлекает и ожидает int
    SymbolAddress popSymbolAddress(); // Извлекает адрес переменной
    ArrayElementAddress popArrayElementAddress(); // Извлекает адрес элемента массива

    // Получение значения по адресу (из StackValue)
    SymbolValue getValueFromStackValue(const StackValue& sv);
    int getIntValueFromStackValue(const StackValue& sv);

public:
    // Конструктор
    Interpreter(const std::vector<RPNOperation>& code, SymbolTable& symTab /*, ErrorHandler& errHandler*/);

    // Запуск выполнения кода ОПС
    void execute();
};

#endif // INTERPRETER_H