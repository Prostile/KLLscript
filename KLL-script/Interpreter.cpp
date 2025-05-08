    #include "Interpreter.h"
#include <iostream> // ��� �����/������ � ������
#include <stdexcept> // ��� std::get<> ��� ������ � variant
#include <cmath>     // ��� ������� �� ����

// �����������
Interpreter::Interpreter(const std::vector<RPNOperation>& code, SymbolTable& symTab /*, ErrorHandler& errHandler*/)
    : rpnCode(code), symbolTable(symTab) /*, errorHandler(errHandler)*/, instructionPointer(0) {
}

// --- ��������������� ������ ---

void Interpreter::runtimeError(const std::string& message) {
    // TODO: ������������ ErrorHandler
    std::cerr << "Runtime Error (at RPN address " << instructionPointer << "): " << message << std::endl;
    // ��������, ����� ������� ��������� ����� � ����������
    exit(EXIT_FAILURE);
}

StackValue Interpreter::popValue() {
    if (stack.isEmpty()) {
        runtimeError("Stack underflow during popValue.");
    }
    return stack.pop();
}

int Interpreter::popIntValue() {
    StackValue sv = popValue();
    return getIntValueFromStackValue(sv);
}

SymbolAddress Interpreter::popSymbolAddress() {
    StackValue sv = popValue();
    if (sv.type != StackValue::Type::SYMBOL_ADDRESS) {
        runtimeError("Type mismatch: Expected a symbol address on stack.");
    }
    try {
        return std::get<SymbolAddress>(sv.value);
    }
    catch (const std::bad_variant_access& e) {
        runtimeError("Internal error accessing symbol address from stack value.");
        // ������� �������, ����� ���������� �� �������, ���� ���� �� ������ �����
        return SymbolAddress{ -1 };
    }
}

ArrayElementAddress Interpreter::popArrayElementAddress() {
    StackValue sv = popValue();
    if (sv.type != StackValue::Type::ARRAY_ELEMENT_ADDRESS) {
        runtimeError("Type mismatch: Expected an array element address on stack.");
    }
    try {
        return std::get<ArrayElementAddress>(sv.value);
    }
    catch (const std::bad_variant_access& e) {
        runtimeError("Internal error accessing array element address from stack value.");
        // ������� �������
        return ArrayElementAddress{ -1, -1 };
    }
}

// ��������� �������� �� StackValue (���� ������ int)
SymbolValue Interpreter::getValueFromStackValue(const StackValue& sv) {
    switch (sv.type) {
    case StackValue::Type::INT_VALUE:
        try {
            return std::get<int>(sv.value);
        }
        catch (const std::bad_variant_access& e) {
            runtimeError("Internal error accessing int value from stack value.");
            return 0; // ��� �����������
        }
    case StackValue::Type::SYMBOL_ADDRESS: {
        try {
            int symbolIndex = std::get<SymbolAddress>(sv.value).tableIndex;
            auto valueOpt = symbolTable.getVariableValue(symbolIndex);
            if (!valueOpt) {
                runtimeError("Failed to get value for variable " + symbolTable.getSymbolName(symbolIndex) + ".");
                return 0; // ��� �����������
            }
            return valueOpt.value();
        }
        catch (const std::bad_variant_access& e) {
            runtimeError("Internal error accessing symbol address from stack value.");
            return 0; // ��� �����������
        }
    }
    case StackValue::Type::ARRAY_ELEMENT_ADDRESS: {
        try {
            auto addr = std::get<ArrayElementAddress>(sv.value);
            auto valueOpt = symbolTable.getArrayElementValue(addr.tableIndex, addr.elementIndex);
            if (!valueOpt) {
                // ������ (����� �� �������) ��� ������ ���� ���� �������� � getArrayElementValue
                runtimeError("Failed to get value for array element."); // ���. ���������
                return 0; // ��� �����������
            }
            return valueOpt.value();
        }
        catch (const std::bad_variant_access& e) {
            runtimeError("Internal error accessing array element address from stack value.");
            return 0; // ��� �����������
        }
    }
                                                // �������� ��������� float, bool
    default:
        runtimeError("Unsupported stack value type for getValue operation.");
        return 0; // ��� �����������
    }
}

int Interpreter::getIntValueFromStackValue(const StackValue& sv) {
    SymbolValue val = getValueFromStackValue(sv);
    try {
        return std::get<int>(val); // ���� ������������ ������ int
    }
    catch (const std::bad_variant_access& e) {
        runtimeError("Type mismatch: Expected an integer value but found another type.");
        return 0; // ��� �����������
    }
}


// --- ������ ���������� ��� ---
void Interpreter::execute() {
    instructionPointer = 0;
    int maxInstructions = rpnCode.size() * 100; // ������ �� ������������ �����
    int executedCount = 0;

    while (instructionPointer < rpnCode.size() && executedCount < maxInstructions) {
        if (instructionPointer < 0) {
            runtimeError("Invalid instruction pointer.");
            break;
        }
        const RPNOperation& op = rpnCode[instructionPointer];
        int currentIP = instructionPointer; // ��������� IP �� ���������� ��������
        instructionPointer++; // �� ��������� ��������� � ���������
        executedCount++;

        try { // ������� ��������� �������� � try-catch ��� std::get
            switch (op.opCode) {
                // --- �������� ---
            case RPNOpCode::PUSH_CONST_INT:
                if (!op.operand) runtimeError("PUSH_CONST operation missing operand.");
                stack.push(StackValue(std::get<int>(op.operand.value()))); // ������ ��������
                break;
            case RPNOpCode::PUSH_VAR:
                if (!op.symbolIndex) runtimeError("PUSH_VAR operation missing symbol index.");
                // ������ ����� (������) ���������� � �������
                stack.push(StackValue(op.symbolIndex.value(), StackValue::Type::SYMBOL_ADDRESS));
                break;
            case RPNOpCode::PUSH_ARRAY:
                if (!op.symbolIndex) runtimeError("PUSH_ARRAY operation missing symbol index.");
                // ������ ����� (������) ������� � �������
                // ��������: ���������� �� PUSH_VAR, ����� ������ ��� � StackValue,
                // �� �� ��� ���������� ������ � INDEX, ��� ��� ���� ������� SYMBOL_ADDRESS
                // ����� �������� StackValue::Type::ARRAY_BASE_ADDRESS
                stack.push(StackValue(op.symbolIndex.value(), StackValue::Type::SYMBOL_ADDRESS));
                break;


                // --- ���������� ---
            case RPNOpCode::ADD:
            case RPNOpCode::SUB:
            case RPNOpCode::MUL:
            case RPNOpCode::DIV: {
                int rightOperand = popIntValue();
                int leftOperand = popIntValue();
                int result = 0;
                if (op.opCode == RPNOpCode::ADD) result = leftOperand + rightOperand;
                else if (op.opCode == RPNOpCode::SUB) { std::cout << leftOperand << " " << rightOperand << std::endl; result = leftOperand - rightOperand; }
                else if (op.opCode == RPNOpCode::MUL) result = leftOperand * rightOperand;
                else if (op.opCode == RPNOpCode::DIV) {
                    if (rightOperand == 0) runtimeError("Division by zero.");
                    result = leftOperand / rightOperand; // ������������� �������
                }
                stack.push(StackValue(result)); // ������ ���������
                break;
            }

                               // --- ��������� ---
            case RPNOpCode::CMP_EQ:
            case RPNOpCode::CMP_NE:
            case RPNOpCode::CMP_GT:
            case RPNOpCode::CMP_LT: {
                int rightOperand = popIntValue();
                int leftOperand = popIntValue();
                bool result = false;
                if (op.opCode == RPNOpCode::CMP_EQ) result = (leftOperand == rightOperand);
                else if (op.opCode == RPNOpCode::CMP_NE) result = (leftOperand != rightOperand);
                else if (op.opCode == RPNOpCode::CMP_GT) result = (leftOperand > rightOperand);
                else if (op.opCode == RPNOpCode::CMP_LT) result = (leftOperand < rightOperand);
                stack.push(StackValue(result ? 1 : 0)); // ��������� ��������� (1 - true, 0 - false)
                break;
            }

                                  // --- ������������ ---
            case RPNOpCode::ASSIGN: {
                SymbolValue valueToAssign = getValueFromStackValue(popValue()); // ��������
                StackValue target = popValue(); // ����� (���������� ��� ������� �������)

                if (target.type == StackValue::Type::SYMBOL_ADDRESS) {
                    int targetIndex = std::get<SymbolAddress>(target.value).tableIndex;
                    if (!symbolTable.setVariableValue(targetIndex, valueToAssign)) {
                        runtimeError("Failed to assign value to variable " + symbolTable.getSymbolName(targetIndex) + ".");
                    }
                }
                else if (target.type == StackValue::Type::ARRAY_ELEMENT_ADDRESS) {
                    auto addr = std::get<ArrayElementAddress>(target.value);
                    if (!symbolTable.setArrayElementValue(addr.tableIndex, addr.elementIndex, valueToAssign)) {
                        // ������ ��� �������� � setArrayElementValue
                        runtimeError("Failed to assign value to array element.");
                    }
                }
                else {
                    runtimeError("Invalid target type for assignment.");
                }
                break;
            }

                                  // --- ���������� ---
            case RPNOpCode::INDEX: {
                int elementIdxValue = popIntValue(); // ������ ��������
                StackValue arrayBase = popValue(); // ����� (������) �������

                if (arrayBase.type != StackValue::Type::SYMBOL_ADDRESS) {
                    runtimeError("Expected array base address for indexing.");
                }
                int arrayTableIndex = std::get<SymbolAddress>(arrayBase.value).tableIndex;

                // �������� �� ������������� ������
                if (elementIdxValue < 0) {
                    runtimeError("Array index cannot be negative.");
                }

                // ������ � ���� ����� �������� �������
                stack.push(StackValue(arrayTableIndex, elementIdxValue));
                break;
            }

                                 // --- ����/����� ---
            case RPNOpCode::READ: {
                StackValue target = popValue(); // �����, ���� ������
                int valueRead = 0;
                std::cout << "? "; // ����������� � �����
                std::cin >> valueRead;
                if (std::cin.fail()) {
                    runtimeError("Invalid input. Please enter an integer.");
                    // ������� ��������� ������ cin
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                }

                SymbolValue symValueToSet = valueRead; // ���� ������ int

                if (target.type == StackValue::Type::SYMBOL_ADDRESS) {
                    int targetIndex = std::get<SymbolAddress>(target.value).tableIndex;
                    if (!symbolTable.setVariableValue(targetIndex, symValueToSet)) {
                        runtimeError("Failed to read value into variable " + symbolTable.getSymbolName(targetIndex) + ".");
                    }
                }
                else if (target.type == StackValue::Type::ARRAY_ELEMENT_ADDRESS) {
                    auto addr = std::get<ArrayElementAddress>(target.value);
                    if (!symbolTable.setArrayElementValue(addr.tableIndex, addr.elementIndex, symValueToSet)) {
                        runtimeError("Failed to read value into array element.");
                    }
                }
                else {
                    runtimeError("Invalid target type for read operation.");
                }
                break;
            }
            case RPNOpCode::WRITE: {
                int valueToWrite = popIntValue(); // �������� ��� ������
                std::cout << valueToWrite << std::endl;
                break;
            }

                                 // --- �������� ---
            case RPNOpCode::JUMP:
                if (!op.jumpTarget.has_value() || op.jumpTarget.value() < 0) {
                    runtimeError("JUMP target address not set.");
                }
                instructionPointer = op.jumpTarget.value(); // ������������� IP
                break;
            case RPNOpCode::JUMP_FALSE: {
                int condition = popIntValue(); // ��������� ������� (0 ��� 1)
                if (!op.jumpTarget.has_value() || op.jumpTarget.value() < 0) {
                    runtimeError("JUMP_FALSE target address not set.");
                }
                if (condition == 0) { // ���� ������� �����
                    instructionPointer = op.jumpTarget.value(); // ������������� IP
                }
                // ���� �������, IP ��� ���������������
                break;
            }
            default:
                runtimeError("Unknown RPN operation code encountered.");
                break;
            }
        }
        catch (const std::bad_variant_access& e) {
            runtimeError("Internal error processing RPN operation: " + std::string(e.what()));
        }
        catch (const std::exception& e) {
            runtimeError("Standard exception during execution: " + std::string(e.what()));
        }
        catch (...) {
            runtimeError("Unknown exception during execution.");
        }
    } // end while

    if (executedCount >= maxInstructions) {
        runtimeError("Maximum instruction limit exceeded. Possible infinite loop.");
    }

    // �������� �� ������ ���� � ����� (�����������)
    // if (!stack.isEmpty()) {
    //     std::cerr << "Warning: Stack is not empty at the end of execution." << std::endl;
    // }
}