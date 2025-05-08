#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <vector>
#include <string>
#include <variant>
#include <stack>
#include <iostream>
#include <optional>
#include <utility> // ��� std::move

#include "definitions.h"

// --- ��������� ��� ������������� ������ (�������) ---
struct Token {
    TokenType type = TokenType::T_UNKNOWN;
    std::string text = "";      // ����� (��� ���������������, �������� ����)
    int intValue = 0;           // �������� ��� T_NUMBER_INT
    double floatValue = 0.0;    // �������� ��� T_NUMBER_FLOAT (���������)
    bool boolValue = false;     // �������� ��� T_KW_TRUE / T_KW_FALSE (���������)
    int line = 0;
    int column = 0;

    Token() = default;

    // ������������ ��� ������ ����� �������
    Token(TokenType t, int l, int c) : type(t), line(l), column(c) {} // ��� ������� �������
    Token(TokenType t, std::string txt, int l, int c) : type(t), text(std::move(txt)), line(l), column(c) {} // ��� ���������������/�������� ����
    Token(TokenType t, int val, int l, int c) : type(t), intValue(val), line(l), column(c) {} // ��� int
    Token(TokenType t, double val, int l, int c) : type(t), floatValue(val), line(l), column(c) {} // ��� float
    Token(TokenType t, bool val, int l, int c) : type(t), boolValue(val), line(l), column(c) {} // ��� bool (true/false)
};

// --- ��� ��� �������� �������� ���������� ��� ��������� ---
// ��������� variant ��� ��������� float � bool
using SymbolValue = std::variant<int, double, bool>;

// --- ������ � ������� �������� ---
// ��������� ���� ��� float � bool
enum class SymbolType {
    VARIABLE_INT,
    VARIABLE_FLOAT, // ���������
    VARIABLE_BOOL,  // ���������
    ARRAY_INT,
    ARRAY_FLOAT,    // ���������
    ARRAY_BOOL      // ���������
};

// ������� ��� ��������� ���� �������� �� ��������� ��� SymbolType
inline SymbolValue getDefaultValueForType(SymbolType type) {
    switch (type) {
    case SymbolType::VARIABLE_INT:
    case SymbolType::ARRAY_INT:    return SymbolValue(0);
    case SymbolType::VARIABLE_FLOAT:
    case SymbolType::ARRAY_FLOAT:  return SymbolValue(0.0);
    case SymbolType::VARIABLE_BOOL:
    case SymbolType::ARRAY_BOOL:   return SymbolValue(false);
    default:                       return SymbolValue(0); // ��� ������
    }
}


struct SymbolInfo {
    std::string name;
    SymbolType type;
    int declarationLine = 0;

    // �������������� ��������� �� ��������� ��� ������� ����
    SymbolValue value; // �������� ��� ����������

    std::vector<SymbolValue> arrayData; // ������ ��� ��������
    size_t arraySize = 0;

    SymbolInfo(std::string n, SymbolType t, int line)
        : name(std::move(n)), type(t), declarationLine(line), value(getDefaultValueForType(t)) {
    }
};


// --- ��������� ��� �������� ��� (RPN) ---
struct RPNOperation {
    RPNOpCode opCode;
    // Operand ������ ����� ������� int, double ��� bool
    std::optional<SymbolValue> operand;
    std::optional<int> symbolIndex;
    std::optional<int> jumpTarget = -1; // ���������� optional ������ -1 ��� �������

    RPNOperation(RPNOpCode code) : opCode(code) {}

    RPNOperation(RPNOpCode code, SymbolValue val) : opCode(code), operand(val) {}

    RPNOperation(RPNOpCode code, int symIdx) : opCode(code), symbolIndex(symIdx) {}

    // ����������� ��� ���������
    RPNOperation(RPNOpCode code, bool isJumpPlaceholder) : opCode(code) {
        if (code == RPNOpCode::JUMP || code == RPNOpCode::JUMP_FALSE) {
            jumpTarget = std::nullopt; // ���������� nullopt ������ -1
        }
    }
};


// --- ������� ����� �������������� ---

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
        FLOAT_VALUE,        // ���������
        BOOL_VALUE,         // ���������
        SYMBOL_ADDRESS,     // ����� ����������
        ARRAY_ELEMENT_ADDRESS // ����� �������� �������
    } type;

    // ��������� variant ��� �������� ����� ����� ��������
    std::variant<int, double, bool, SymbolAddress, ArrayElementAddress> value;

    // ������������
    StackValue(int val) : type(Type::INT_VALUE), value(val) {}
    StackValue(double val) : type(Type::FLOAT_VALUE), value(val) {} // ���������
    StackValue(bool val) : type(Type::BOOL_VALUE), value(val) {}   // ���������

    // ����������� ��� ������ ����������
    StackValue(int symIndex, Type t) : type(t) {
        if (t == Type::SYMBOL_ADDRESS) {
            value = SymbolAddress{ symIndex };
        }
        else {
            std::cerr << "Internal Error: Invalid usage of StackValue symbol index constructor." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    // ����������� ��� ������ �������� �������
    StackValue(int tblIdx, int elmIdx) : type(Type::ARRAY_ELEMENT_ADDRESS), value(ArrayElementAddress{ tblIdx, elmIdx }) {}
};


// --- ����� ����� �������������� ---
// (��� ���������, �� ������ ����� �������� � ����������� StackValue)
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