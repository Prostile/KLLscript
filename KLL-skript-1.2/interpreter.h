// interpreter.h
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <vector>
#include <string>
#include <stack>
#include <variant>
#include <optional>
#include <stdexcept> // ��� std::get � ����������

#include "definitions.h"    // RPNOpCode, SymbolType
#include "rpn_op.h"         // ��������� RPNOperation
#include "symbol_table.h"   // SymbolTable, StoredValue (��� ��������)
#include "error_handler.h"  // ErrorHandler

// --- ������� ����� ������� ���������� ---
// ����� ������� ���������������� �������� (int, float) ��� ����� (������ � ������� ��������)
// ��� ����� �������� �������.

// ��������� ��� ������ ���������� � ������� ��������
struct VarAddress {
    size_t table_index;
};

// ��������� ��� ������ �������� �������
struct ArrayElementAddress {
    size_t array_table_index; // ������ ������ ������� � ������� ��������
    int element_runtime_index;  // ����������� �� ����� ���������� ������ ��������
};


struct RuntimeStackItem {
    enum class ItemType {
        INT_VALUE,
        FLOAT_VALUE,
        VAR_ADDRESS,         // ����� ������� ����������
        ARRAY_ELEMENT_ADDRESS // ����� ����������� �������� �������
    } type;

    std::variant<int, float, VarAddress, ArrayElementAddress> value;

    // ������������
    RuntimeStackItem(int val) : type(ItemType::INT_VALUE), value(val) {}
    RuntimeStackItem(float val) : type(ItemType::FLOAT_VALUE), value(val) {}
    RuntimeStackItem(VarAddress addr) : type(ItemType::VAR_ADDRESS), value(addr) {}
    RuntimeStackItem(ArrayElementAddress arrAddr) : type(ItemType::ARRAY_ELEMENT_ADDRESS), value(arrAddr) {}
};


// --- ����� ����� �������������� ---
class RuntimeStack {
private:
    std::stack<RuntimeStackItem> stck;
    static const size_t MAX_STACK_SIZE = 1000; // ����������� �� ������� �����

public:
    RuntimeStack() = default;

    void push(const RuntimeStackItem& item);
    RuntimeStackItem pop();
    bool isEmpty() const;
    size_t size() const;
    void clear(); // ��� ������ ��������� ����� ��������� (���� �����)
};


// --- ����� �������������� ��� ---
class Interpreter {
private:
    const std::vector<RPNOperation>& rpnCode; // ������ �� ��� ���
    SymbolTable& symbolTable;                 // ������ �� ������� ��������
    ErrorHandler& errorHandler;               // ������ �� ���������� ������

    RuntimeStack stack;                       // ���� ������� ����������
    int instructionPointer;                   // ��������� �� ������� ���������� ��� (����� � rpnCode)

    // --- ��������������� ������ ��� ������ �� ������ � ���������� ---
    void runtimeError(const std::string& message); // �������� �� ������ ������� ����������

    // ���������� �� �����
    RuntimeStackItem popStack(); // ������� pop
    int popInt();         // ������� int ��� �������������� float
    float popFloat();       // ������� float ��� �������������� int
    VarAddress popVarAddress();
    ArrayElementAddress popArrayElementAddress();

    // ��������� �������� �� SymbolTable �� ������ �� �����
    StoredValue getValueFromStackItem(const RuntimeStackItem& item);
    // ��������� �������� � SymbolTable �� ������ �� �����
    void setValueAtStackItemAddress(const RuntimeStackItem& addressItem, const StoredValue& valueToSet);


public:
    Interpreter(const std::vector<RPNOperation>& code, SymbolTable& symTab, ErrorHandler& errHandler);

    void execute(); // ������ ���������� ���� ���
};

#endif // INTERPRETER_H