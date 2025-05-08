#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <vector>
#include <string>

#include "definitions.h"
#include "DataStructures.h" // RPNOperation, RuntimeStack, StackValue, SymbolValue etc.
#include "SymbolTable.h"
// #include "ErrorHandler.h"

// --- ����� �������������� ��� ---
class Interpreter {
private:
    const std::vector<RPNOperation>& rpnCode; // ������ �� ��� ���
    SymbolTable& symbolTable;                 // ������ �� ������� ��������
    // ErrorHandler& errorHandler;             // ������ �� ���������� ������

    RuntimeStack stack;                       // ���� ������� ����������
    int instructionPointer;                   // ��������� �� ������� ���������� ���

    // --- ��������������� ������ ---
    void runtimeError(const std::string& message);

    // ���������� ��������� �� ����� � ��������� ����
    StackValue popValue(); // ������ ���������
    int popIntValue();     // ��������� � ������� int
    SymbolAddress popSymbolAddress(); // ��������� ����� ����������
    ArrayElementAddress popArrayElementAddress(); // ��������� ����� �������� �������

    // ��������� �������� �� ������ (�� StackValue)
    SymbolValue getValueFromStackValue(const StackValue& sv);
    int getIntValueFromStackValue(const StackValue& sv);

public:
    // �����������
    Interpreter(const std::vector<RPNOperation>& code, SymbolTable& symTab /*, ErrorHandler& errHandler*/);

    // ������ ���������� ���� ���
    void execute();
};

#endif // INTERPRETER_H