// interpreter.h
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <vector>
#include <string>
#include <stack>
#include <variant>
#include <optional>
#include <stdexcept> // Для std::get и исключений

#include "definitions.h"    // RPNOpCode, SymbolType
#include "rpn_op.h"         // Структура RPNOperation
#include "symbol_table.h"   // SymbolTable, StoredValue (для значений)
#include "error_handler.h"  // ErrorHandler

// --- Элемент стека времени выполнения ---
// Может хранить непосредственное значение (int, float) или адрес (индекс в таблице символов)
// или адрес элемента массива.

// Структура для адреса переменной в таблице символов
struct VarAddress {
    size_t table_index;
};

// Структура для адреса элемента массива
struct ArrayElementAddress {
    size_t array_table_index; // Индекс самого массива в таблице символов
    int element_runtime_index;  // Вычисленный во время выполнения индекс элемента
};


struct RuntimeStackItem {
    enum class ItemType {
        INT_VALUE,
        FLOAT_VALUE,
        VAR_ADDRESS,         // Адрес простой переменной
        ARRAY_ELEMENT_ADDRESS // Адрес конкретного элемента массива
    } type;

    std::variant<int, float, VarAddress, ArrayElementAddress> value;

    // Конструкторы
    RuntimeStackItem(int val) : type(ItemType::INT_VALUE), value(val) {}
    RuntimeStackItem(float val) : type(ItemType::FLOAT_VALUE), value(val) {}
    RuntimeStackItem(VarAddress addr) : type(ItemType::VAR_ADDRESS), value(addr) {}
    RuntimeStackItem(ArrayElementAddress arrAddr) : type(ItemType::ARRAY_ELEMENT_ADDRESS), value(arrAddr) {}
};


// --- Класс Стека Интерпретатора ---
class RuntimeStack {
private:
    std::stack<RuntimeStackItem> stck;
    static const size_t MAX_STACK_SIZE = 1000; // Ограничение на глубину стека

public:
    RuntimeStack() = default;

    void push(const RuntimeStackItem& item);
    RuntimeStackItem pop();
    bool isEmpty() const;
    size_t size() const;
    void clear(); // Для сброса состояния между запусками (если нужно)
};


// --- Класс Интерпретатора ОПС ---
class Interpreter {
private:
    const std::vector<RPNOperation>& rpnCode; // Ссылка на код ОПС
    SymbolTable& symbolTable;                 // Ссылка на таблицу символов
    ErrorHandler& errorHandler;               // Ссылка на обработчик ошибок

    RuntimeStack stack;                       // Стек времени выполнения
    int instructionPointer;                   // Указатель на текущую инструкцию ОПС (адрес в rpnCode)

    // --- Вспомогательные методы для работы со стеком и значениями ---
    void runtimeError(const std::string& message); // Сообщает об ошибке времени выполнения

    // Извлечение со стека
    RuntimeStackItem popStack(); // Базовый pop
    int popInt();         // Ожидает int или конвертируемый float
    float popFloat();       // Ожидает float или конвертируемый int
    VarAddress popVarAddress();
    ArrayElementAddress popArrayElementAddress();

    // Получение значения из SymbolTable по адресу со стека
    StoredValue getValueFromStackItem(const RuntimeStackItem& item);
    // Установка значения в SymbolTable по адресу со стека
    void setValueAtStackItemAddress(const RuntimeStackItem& addressItem, const StoredValue& valueToSet);


public:
    Interpreter(const std::vector<RPNOperation>& code, SymbolTable& symTab, ErrorHandler& errHandler);

    void execute(); // Запуск выполнения кода ОПС
};

#endif // INTERPRETER_H