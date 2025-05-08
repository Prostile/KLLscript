#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <vector>
#include <string>
#include <variant>
#include <stack>
#include <iostream>
#include <optional>
#include <utility> // Для std::move

#include "definitions.h"

// --- Структура для представления Токена (Лексемы) ---
struct Token {
    TokenType type = TokenType::T_UNKNOWN;
    std::string text = "";      // Текст (для идентификаторов, ключевых слов)
    int intValue = 0;           // Значение для T_NUMBER_INT
    double floatValue = 0.0;    // Значение для T_NUMBER_FLOAT (Добавлено)
    bool boolValue = false;     // Значение для T_KW_TRUE / T_KW_FALSE (Добавлено)
    int line = 0;
    int column = 0;

    Token() = default;

    // Конструкторы для разных типов токенов
    Token(TokenType t, int l, int c) : type(t), line(l), column(c) {} // Для простых токенов
    Token(TokenType t, std::string txt, int l, int c) : type(t), text(std::move(txt)), line(l), column(c) {} // Для идентификаторов/ключевых слов
    Token(TokenType t, int val, int l, int c) : type(t), intValue(val), line(l), column(c) {} // Для int
    Token(TokenType t, double val, int l, int c) : type(t), floatValue(val), line(l), column(c) {} // Для float
    Token(TokenType t, bool val, int l, int c) : type(t), boolValue(val), line(l), column(c) {} // Для bool (true/false)
};

// --- Тип для хранения значения переменной или константы ---
// Расширяем variant для поддержки float и bool
using SymbolValue = std::variant<int, double, bool>;

// --- Запись в таблице символов ---
// Добавляем типы для float и bool
enum class SymbolType {
    VARIABLE_INT,
    VARIABLE_FLOAT, // Добавлено
    VARIABLE_BOOL,  // Добавлено
    ARRAY_INT,
    ARRAY_FLOAT,    // Добавлено
    ARRAY_BOOL      // Добавлено
};

// Функция для получения типа значения по умолчанию для SymbolType
inline SymbolValue getDefaultValueForType(SymbolType type) {
    switch (type) {
    case SymbolType::VARIABLE_INT:
    case SymbolType::ARRAY_INT:    return SymbolValue(0);
    case SymbolType::VARIABLE_FLOAT:
    case SymbolType::ARRAY_FLOAT:  return SymbolValue(0.0);
    case SymbolType::VARIABLE_BOOL:
    case SymbolType::ARRAY_BOOL:   return SymbolValue(false);
    default:                       return SymbolValue(0); // Или ошибка
    }
}


struct SymbolInfo {
    std::string name;
    SymbolType type;
    int declarationLine = 0;

    // Инициализируем значением по умолчанию для данного типа
    SymbolValue value; // Значение для переменных

    std::vector<SymbolValue> arrayData; // Данные для массивов
    size_t arraySize = 0;

    SymbolInfo(std::string n, SymbolType t, int line)
        : name(std::move(n)), type(t), declarationLine(line), value(getDefaultValueForType(t)) {
    }
};


// --- Структура для элемента ОПС (RPN) ---
struct RPNOperation {
    RPNOpCode opCode;
    // Operand теперь может хранить int, double или bool
    std::optional<SymbolValue> operand;
    std::optional<int> symbolIndex;
    std::optional<int> jumpTarget = -1; // Используем optional вместо -1 для ясности

    RPNOperation(RPNOpCode code) : opCode(code) {}

    RPNOperation(RPNOpCode code, SymbolValue val) : opCode(code), operand(val) {}

    RPNOperation(RPNOpCode code, int symIdx) : opCode(code), symbolIndex(symIdx) {}

    // Конструктор для переходов
    RPNOperation(RPNOpCode code, bool isJumpPlaceholder) : opCode(code) {
        if (code == RPNOpCode::JUMP || code == RPNOpCode::JUMP_FALSE) {
            jumpTarget = std::nullopt; // Используем nullopt вместо -1
        }
    }
};


// --- Элемент стека интерпретатора ---

struct SymbolAddress {
    int tableIndex;
};

struct ArrayElementAddress {
    int tableIndex;
    int elementIndex;
};

struct StackValue {
    enum class Type {
        INT_VALUE,
        FLOAT_VALUE,        // Добавлено
        BOOL_VALUE,         // Добавлено
        SYMBOL_ADDRESS,     // Адрес переменной
        ARRAY_ELEMENT_ADDRESS // Адрес элемента массива
    } type;

    // Расширяем variant для хранения новых типов значений
    std::variant<int, double, bool, SymbolAddress, ArrayElementAddress> value;

    // Конструкторы
    StackValue(int val) : type(Type::INT_VALUE), value(val) {}
    StackValue(double val) : type(Type::FLOAT_VALUE), value(val) {} // Добавлено
    StackValue(bool val) : type(Type::BOOL_VALUE), value(val) {}   // Добавлено

    // Конструктор для адреса переменной
    StackValue(int symIndex, Type t) : type(t) {
        if (t == Type::SYMBOL_ADDRESS) {
            value = SymbolAddress{ symIndex };
        }
        else {
            std::cerr << "Internal Error: Invalid usage of StackValue symbol index constructor." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    // Конструктор для адреса элемента массива
    StackValue(int tblIdx, int elmIdx) : type(Type::ARRAY_ELEMENT_ADDRESS), value(ArrayElementAddress{ tblIdx, elmIdx }) {}
};


// --- Класс Стека Интерпретатора ---
// (Без изменений, но теперь будет работать с расширенным StackValue)
class RuntimeStack {
private:
    std::stack<StackValue> stck;
    static const size_t MAX_STACK_SIZE = 500;

public:
    RuntimeStack() = default;

    void push(const StackValue& val) {
        if (stck.size() >= MAX_STACK_SIZE) {
            std::cerr << "Runtime Error: Stack overflow." << std::endl;
            exit(EXIT_FAILURE);
        }
        stck.push(val);
    }

    StackValue pop() {
        if (stck.empty()) {
            std::cerr << "Runtime Error: Stack underflow." << std::endl;
            exit(EXIT_FAILURE);
        }
        StackValue topVal = stck.top();
        stck.pop();
        return topVal;
    }

    bool isEmpty() const {
        return stck.empty();
    }

    size_t size() const {
        return stck.size();
    }
};


#endif // DATA_STRUCTURES_H