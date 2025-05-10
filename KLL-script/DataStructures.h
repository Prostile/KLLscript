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
    std::string text = "";
    int intValue = 0;
    // double floatValue = 0.0;
    int line = 0;
    int column = 0;

    Token() = default;

    Token(TokenType t, int l, int c) : type(t), line(l), column(c) {}

    Token(TokenType t, std::string txt, int l, int c) : type(t), text(std::move(txt)), line(l), column(c) {}

    Token(TokenType t, int val, int l, int c) : type(t), intValue(val), line(l), column(c) {}
};

// --- Тип для хранения значения переменной или константы ---
using SymbolValue = std::variant<int /*, double*/>;

// --- Запись в таблице символов ---
enum class SymbolType {
    VARIABLE_INT,
    // VARIABLE_FLOAT,
    ARRAY_INT
    // ARRAY_FLOAT
};

struct SymbolInfo {
    std::string name;
    SymbolType type;
    int declarationLine = 0;

    SymbolValue value = 0;

    std::vector<SymbolValue> arrayData;
    size_t arraySize = 0;

    SymbolInfo(std::string n, SymbolType t, int line) : name(std::move(n)), type(t), declarationLine(line) {}
};


// --- Структура для элемента ОПС (RPN) ---
struct RPNOperation {
    RPNOpCode opCode;
    std::optional<SymbolValue> operand;
    std::optional<int> symbolIndex;
    std::optional<int> jumpTarget = -1;

    RPNOperation(RPNOpCode code) : opCode(code) {}

    RPNOperation(RPNOpCode code, SymbolValue val) : opCode(code), operand(val) {}

    RPNOperation(RPNOpCode code, int symIdx) : opCode(code), symbolIndex(symIdx) {}

    // Конструктор для переходов, который ЯВНО указывает, что это переход
    // чтобы можно было отличить от других конструкторов с одним RPNOpCode
    RPNOperation(RPNOpCode code, bool isJumpPlaceholder) : opCode(code) {
        if (code == RPNOpCode::JUMP || code == RPNOpCode::JUMP_FALSE) {
            jumpTarget = -1; // Инициализируем цель для переходов
        }
        else {
            // Можно добавить проверку или сообщение об ошибке,
            // если этот конструктор вызван не для JUMP/JUMP_FALSE
        }
    }
};


// --- Элемент стека интерпретатора ---

// Отдельная структура для адреса (индекса) переменной в таблице символов
struct SymbolAddress {
    int tableIndex;
};

// Отдельная структура для адреса элемента массива
struct ArrayElementAddress {
    int tableIndex;   // Индекс массива в таблице символов
    int elementIndex; // Индекс элемента внутри массива
};

struct StackValue {
    // Определяем, что хранится в элементе
    enum class Type {
        INT_VALUE,          // Непосредственно значение int
        // FLOAT_VALUE,
        SYMBOL_ADDRESS,     // Адрес переменной (индекс в таблице)
        ARRAY_ELEMENT_ADDRESS // Адрес элемента массива
    } type;

    // Делаем типы в variant уникальными
    std::variant<int, /* double */ SymbolAddress, ArrayElementAddress> value;

    // Конструкторы
    StackValue(int val) : type(Type::INT_VALUE), value(val) {}
    // StackValue(double val) : type(Type::FLOAT_VALUE), value(val) {}

    // Конструктор для адреса переменной
    StackValue(int symIndex, Type t) : type(t) {
        if (t == Type::SYMBOL_ADDRESS) {
            value = SymbolAddress{ symIndex }; // Используем новую структуру
        }
        else {
            // Генерируем ошибку или исключение, т.к. конструктор вызван некорректно
            std::cerr << "Internal Error: Invalid usage of StackValue symbol index constructor." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    // Конструктор для адреса элемента массива
    StackValue(int tblIdx, int elmIdx) : type(Type::ARRAY_ELEMENT_ADDRESS), value(ArrayElementAddress{ tblIdx, elmIdx }) {}
};


// --- Класс Стека Интерпретатора ---
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