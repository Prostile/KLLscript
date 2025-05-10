// interpreter.cpp
#include "interpreter.h"
#include <iostream> // ��� cin/cout � �������
#include <iomanip>  
#include <limits>   // ��� std::numeric_limits (������� cin)
#include <cmath>    // ��� std::floor (��� ����������� float � int)

// --- ���������� RuntimeStack ---
void RuntimeStack::push(const RuntimeStackItem& item) {
    if (stck.size() >= MAX_STACK_SIZE) {
        // ��� ������ ������ ������� � ���������� Interpreter ����� runtimeError
        throw std::runtime_error("Runtime Stack overflow.");
    }
    stck.push(item);
}

RuntimeStackItem RuntimeStack::pop() {
    //std::cout << "DEBUG: RuntimeStack::pop() called. Stack size before pop: " << stck.size() << std::endl;
    if (stck.empty()) {
        //std::cout << "ERROR DEBUG: Attempt to pop from empty stack!" << std::endl; // �������
        // ��� ������ ������ ������� � ���������� Interpreter ����� runtimeError
        throw std::runtime_error("Runtime Stack underflow.");
    }
    RuntimeStackItem topItem = stck.top();
    stck.pop();
    return topItem;
}

bool RuntimeStack::isEmpty() const {
    return stck.empty();
}

size_t RuntimeStack::size() const {
    return stck.size();
}

void RuntimeStack::clear() {
    while (!stck.empty()) {
        stck.pop();
    }
}

// --- ���������� Interpreter ---

Interpreter::Interpreter(const std::vector<RPNOperation>& code, SymbolTable& symTab, ErrorHandler& errHandler)
    : rpnCode(code), symbolTable(symTab), errorHandler(errHandler), instructionPointer(0) {
}

void Interpreter::runtimeError(const std::string& message) {
    errorHandler.logRuntimeError("RPN[" + std::to_string(instructionPointer - 1) + "]: " + message); // -1 �.�. IP ��� ���������������
    // ��� ��������� ������ ����� ������� ����������, ����� �������� execute()
    throw std::runtime_error("Fatal runtime error occurred.");
}

// --- ��������������� ������ ��� ������ �� ������ � ���������� ---

RuntimeStackItem Interpreter::popStack() {
    try {
        return stack.pop();
    }
    catch (const std::runtime_error& e) { // ����� stack underflow �� RuntimeStack
        runtimeError(e.what()); // �������� ������ � ��� ErrorHandler
        // ���������� ��������� ��������, ����� ���������� ��� �������, �� throw ��� ���
        return RuntimeStackItem(0);
    }
}

int Interpreter::popInt() {
    RuntimeStackItem item = popStack();
    if (item.type == RuntimeStackItem::ItemType::INT_VALUE) {
        return std::get<int>(item.value);
    }
    else if (item.type == RuntimeStackItem::ItemType::FLOAT_VALUE) {
        // ������� �������� float �� int ��� ���������� ��� int
        // ����� �������� ��������������, ���� ��� ������������� ���������
        // errorHandler.logRuntimeError("Warning: Implicit conversion from float to int during popInt. Value truncated.");
        return static_cast<int>(std::floor(std::get<float>(item.value))); // �������� � �������� (��� � C)
    }
    else if (item.type == RuntimeStackItem::ItemType::VAR_ADDRESS || item.type == RuntimeStackItem::ItemType::ARRAY_ELEMENT_ADDRESS) {
        StoredValue storedVal = getValueFromStackItem(item);
        if (std::holds_alternative<int>(storedVal)) return std::get<int>(storedVal);
        if (std::holds_alternative<float>(storedVal)) {
            // errorHandler.logRuntimeError("Warning: Implicit conversion from float to int for variable/array element during popInt. Value truncated.");
            return static_cast<int>(std::floor(std::get<float>(storedVal)));
        }
        if (std::holds_alternative<std::monostate>(storedVal)) {
            runtimeError("Attempted to use uninitialized variable or array element as integer.");
        }
    }
    runtimeError("Type mismatch on stack: Expected integer or address of integer.");
    return 0; // ����������� ��-�� runtimeError
}

float Interpreter::popFloat() {
    RuntimeStackItem item = popStack();
    if (item.type == RuntimeStackItem::ItemType::FLOAT_VALUE) {
        return std::get<float>(item.value);
    }
    else if (item.type == RuntimeStackItem::ItemType::INT_VALUE) {
        // ������� �������������� int �� float
        return static_cast<float>(std::get<int>(item.value));
    }
    else if (item.type == RuntimeStackItem::ItemType::VAR_ADDRESS || item.type == RuntimeStackItem::ItemType::ARRAY_ELEMENT_ADDRESS) {
        StoredValue storedVal = getValueFromStackItem(item);
        if (std::holds_alternative<float>(storedVal)) return std::get<float>(storedVal);
        if (std::holds_alternative<int>(storedVal)) return static_cast<float>(std::get<int>(storedVal));
        if (std::holds_alternative<std::monostate>(storedVal)) {
            runtimeError("Attempted to use uninitialized variable or array element as float.");
        }
    }
    runtimeError("Type mismatch on stack: Expected float or address of float.");
    return 0.0f; // �����������
}

VarAddress Interpreter::popVarAddress() {
    RuntimeStackItem item = popStack();
    if (item.type == RuntimeStackItem::ItemType::VAR_ADDRESS) {
        return std::get<VarAddress>(item.value);
    }
    runtimeError("Type mismatch on stack: Expected variable address.");
    return VarAddress{ 0 }; // �����������
}

ArrayElementAddress Interpreter::popArrayElementAddress() {
    RuntimeStackItem item = popStack();
    if (item.type == RuntimeStackItem::ItemType::ARRAY_ELEMENT_ADDRESS) {
        return std::get<ArrayElementAddress>(item.value);
    }
    runtimeError("Type mismatch on stack: Expected array element address.");
    return ArrayElementAddress{ 0,0 }; // �����������
}


StoredValue Interpreter::getValueFromStackItem(const RuntimeStackItem& item) {
    if (item.type == RuntimeStackItem::ItemType::INT_VALUE) {
        return std::get<int>(item.value);
    }
    else if (item.type == RuntimeStackItem::ItemType::FLOAT_VALUE) {
        return std::get<float>(item.value);
    }
    else if (item.type == RuntimeStackItem::ItemType::VAR_ADDRESS) {
        size_t varIndex = std::get<VarAddress>(item.value).table_index;
        auto optVal = symbolTable.getVariableValue(varIndex);
        if (!optVal) { // symbolTable.getVariableValue ������ ��� �� ��� ������� errorHandler ��� ������
            runtimeError("Failed to retrieve value for variable (index: " + std::to_string(varIndex) + ").");
            return std::monostate{};
        }
        if (std::holds_alternative<std::monostate>(optVal.value())) {
            // �� ��������� ������ �����, ����� ���������� ��� ������ (��������, �������������� ��������)
            // errorHandler.logRuntimeError("Variable '" + symbolTable.getSymbolName(varIndex) + "' used before initialization.");
        }
        return optVal.value();
    }
    else if (item.type == RuntimeStackItem::ItemType::ARRAY_ELEMENT_ADDRESS) {
        const auto& addr = std::get<ArrayElementAddress>(item.value);
        auto optVal = symbolTable.getArrayElementValue(addr.array_table_index, static_cast<size_t>(addr.element_runtime_index));
        if (!optVal) { // getArrayElementValue ��� �������� ������ ������ �� �������
            runtimeError("Failed to retrieve value for array element '" +
                symbolTable.getSymbolName(addr.array_table_index) +
                "[" + std::to_string(addr.element_runtime_index) + "]'.");
            return std::monostate{};
        }
        if (std::holds_alternative<std::monostate>(optVal.value())) {
            // errorHandler.logRuntimeError("Array element '" + symbolTable.getSymbolName(addr.array_table_index) + 
            //                            "[" + std::to_string(addr.element_runtime_index) + "]' used before initialization.");
        }
        return optVal.value();
    }
    runtimeError("Invalid stack item type for getValue operation.");
    return std::monostate{}; // �����������
}

void Interpreter::setValueAtStackItemAddress(const RuntimeStackItem& addressItem, const StoredValue& valueToSet) {
    if (addressItem.type == RuntimeStackItem::ItemType::VAR_ADDRESS) {
        size_t varIndex = std::get<VarAddress>(addressItem.value).table_index;
        if (!symbolTable.setVariableValue(varIndex, valueToSet)) {
            // symbolTable.setVariableValue ������ ��� ������� errorHandler
            runtimeError("Failed to set value for variable (index: " + std::to_string(varIndex) + ").");
        }
    }
    else if (addressItem.type == RuntimeStackItem::ItemType::ARRAY_ELEMENT_ADDRESS) {
        const auto& addr = std::get<ArrayElementAddress>(addressItem.value);
        if (!symbolTable.setArrayElementValue(addr.array_table_index, static_cast<size_t>(addr.element_runtime_index), valueToSet)) {
            // symbolTable.setArrayElementValue ������ ��� ������� errorHandler
            runtimeError("Failed to set value for array element '" +
                symbolTable.getSymbolName(addr.array_table_index) +
                "[" + std::to_string(addr.element_runtime_index) + "]'.");
        }
    }
    else {
        runtimeError("Invalid address type on stack for setValue operation.");
    }
}

// ����� execute() ����� � ��������� �����
// interpreter.cpp (�����������)
// ... (��� RuntimeStack, ����������� Interpreter, ��������������� ������ �� ����� 1) ...

void Interpreter::execute() {
    instructionPointer = 0;
    stack.clear(); // ������� ���� ����� ����� ��������

    // ������ �� ������� �������� ���������� (������������ ����� � ���)
    const int MAX_EXECUTED_INSTRUCTIONS = 10000000; // 10 ��������� ��������
    int executedCounter = 0;

    try { // �������� ���� try-catch ��� ��������� runtimeError � ������ ����������
        while (instructionPointer >= 0 && static_cast<size_t>(instructionPointer) < rpnCode.size()) {
            if (executedCounter++ > MAX_EXECUTED_INSTRUCTIONS) {
                runtimeError("Maximum instruction execution limit reached. Possible infinite loop.");
                // runtimeError ������ ����������, ������� ������� ����
            }

            const RPNOperation& currentOp = rpnCode[instructionPointer];
            instructionPointer++; // �������������� �� ����������, ����� �������� �������� ���������

            switch (currentOp.opCode) {
                // --- �������� ---
            case RPNOpCode::PUSH_CONST_INT:
                if (!std::holds_alternative<int>(currentOp.operandValue)) {
                    runtimeError("Internal: PUSH_CONST_INT expects an int operand value.");
                }
                stack.push(RuntimeStackItem(std::get<int>(currentOp.operandValue)));
                break;

            case RPNOpCode::PUSH_CONST_FLOAT:
                if (!std::holds_alternative<float>(currentOp.operandValue)) {
                    runtimeError("Internal: PUSH_CONST_FLOAT expects a float operand value.");
                }
                stack.push(RuntimeStackItem(std::get<float>(currentOp.operandValue)));
                break;

            case RPNOpCode::PUSH_VAR_ADDR:
                if (!currentOp.symbolIndex.has_value()) {
                    runtimeError("Internal: PUSH_VAR_ADDR missing symbol index.");
                }
                stack.push(RuntimeStackItem(VarAddress{ currentOp.symbolIndex.value() }));
                break;

            case RPNOpCode::PUSH_ARRAY_ADDR:
                if (!currentOp.symbolIndex.has_value()) {
                    runtimeError("Internal: PUSH_ARRAY_ADDR missing symbol index.");
                }
                // �� ���� �������� ����� *����* ������� (������ � ������� ��������).
                // ��� ������ � ��� ��� � ArrayElementAddress ��� ����� ��� � runtime_index,
                // ������� ����� �������� ��������� INDEX.
                // ������� ����� �� ����� ���������� VarAddress, �.�. ��� ������ ������ � symbolTable.
                // �������� INDEX ����� ������� ���� ������� ����� � ������ ��������.
                stack.push(RuntimeStackItem(VarAddress{ currentOp.symbolIndex.value() }));
                break;

                // --- �������������� �������� ---
            case RPNOpCode::ADD: {
                // �������� ����������� ��� float, �.�. float "������" int
                // float right = popFloat();
                // float left = popFloat();
                // ���� ��� �������� ���� int, ��������� ������ ���� int.
                // ���� ���� �� ���� float, ��������� float.
                // popFloat ��� ������ �������������� int->float, ���� �����.
                // ������ ����� ����������, ��� �� ��������� ���������� int.
                // ��� ����� �������, �������� ���� �������� ��������� �� �����,
                // �� popFloat() �� ��� ��������.
                // �����: ���� ��������� �������� float-����� �������� �����, �� ������ ��� int.
                // �� ��� �� ������ ��������� (1.0 + 2.0 = 3.0, ������ ���� float 3.0, ���� ���� ���� ��� float)
                // �������: ���� ���� �� ���� ������� ��� float, ��������� float.
                // popFloat() ������ float. ���� �� ��� �� int, �� ���� float.
                // ������, ���� ���� ���� �� ��� ���������� ��� float, ��������� float.
                // �� �� ����� ��� ������ �������� ����� ��� ������� ����� �� popFloat.
                // �������: ������������� ��������� �������� �� float, ���� ��������,
                // � CONVERT_TO_INT ������������ ��� ������ ����������.
                // ���, ���� ���� ������� ���������� ������������� ����������:
                // RuntimeStackItem item_right = stack.pop(); // ����� ���� �� ���������, � �� �������
                // RuntimeStackItem item_left = stack.pop();   // (������ ��� peek)
                // ��������� � �������� ��������: �������� ����������� ��� �������� ����� ����� (float).
                // ���� ��� �������� �� ����� ���� int (�� popFloat), �� � ��������� ������ ���� int.

                // �������������� ������: ��������� ��� StoredValue, ����� �������� � ������
                StoredValue val_right = getValueFromStackItem(popStack());
                StoredValue val_left = getValueFromStackItem(popStack());

                if (std::holds_alternative<float>(val_left) || std::holds_alternative<float>(val_right)) {
                    float f_left = std::holds_alternative<int>(val_left) ? static_cast<float>(std::get<int>(val_left)) : std::get<float>(val_left);
                    float f_right = std::holds_alternative<int>(val_right) ? static_cast<float>(std::get<int>(val_right)) : std::get<float>(val_right);
                    stack.push(RuntimeStackItem(f_left + f_right));
                }
                else if (std::holds_alternative<int>(val_left) && std::holds_alternative<int>(val_right)) {
                    stack.push(RuntimeStackItem(std::get<int>(val_left) + std::get<int>(val_right)));
                }
                else {
                    runtimeError("Type error during ADD: Incompatible or uninitialized operands.");
                }
                break;
            }
            case RPNOpCode::SUB: {
                StoredValue val_right = getValueFromStackItem(popStack());
                StoredValue val_left = getValueFromStackItem(popStack());
                if (std::holds_alternative<float>(val_left) || std::holds_alternative<float>(val_right)) {
                    float f_left = std::holds_alternative<int>(val_left) ? static_cast<float>(std::get<int>(val_left)) : std::get<float>(val_left);
                    float f_right = std::holds_alternative<int>(val_right) ? static_cast<float>(std::get<int>(val_right)) : std::get<float>(val_right);
                    stack.push(RuntimeStackItem(f_left - f_right));
                }
                else if (std::holds_alternative<int>(val_left) && std::holds_alternative<int>(val_right)) {
                    stack.push(RuntimeStackItem(std::get<int>(val_left) - std::get<int>(val_right)));
                }
                else {
                    runtimeError("Type error during SUB: Incompatible or uninitialized operands.");
                }
                break;
            }
            case RPNOpCode::MUL: {
                StoredValue val_right = getValueFromStackItem(popStack());
                StoredValue val_left = getValueFromStackItem(popStack());
                if (std::holds_alternative<float>(val_left) || std::holds_alternative<float>(val_right)) {
                    float f_left = std::holds_alternative<int>(val_left) ? static_cast<float>(std::get<int>(val_left)) : std::get<float>(val_left);
                    float f_right = std::holds_alternative<int>(val_right) ? static_cast<float>(std::get<int>(val_right)) : std::get<float>(val_right);
                    stack.push(RuntimeStackItem(f_left * f_right));
                }
                else if (std::holds_alternative<int>(val_left) && std::holds_alternative<int>(val_right)) {
                    stack.push(RuntimeStackItem(std::get<int>(val_left) * std::get<int>(val_right)));
                }
                else {
                    runtimeError("Type error during MUL: Incompatible or uninitialized operands.");
                }
                break;
            }
            case RPNOpCode::DIV: {
                StoredValue val_right = getValueFromStackItem(popStack());
                StoredValue val_left = getValueFromStackItem(popStack());

                // ������� ������ ���������� �� float, ����� �������� ������ �������� ��� int/int,
                // ���� ���� ����� �� ������� ������ (��������, 5/2 = 2.5, � �� 2).
                // ���� ������ �������������, �� ����� ������ ������.
                // �����������, ��� / ������ ���� float, ���� ���� �� ���� ������� float, ��� ���� int/int ���� �������.
                // ��� ��������: ���� ��� int, ����� ��� int. ���� ���� ���� float, ����� ��� float.
                if (std::holds_alternative<float>(val_left) || std::holds_alternative<float>(val_right)) {
                    float f_left = std::holds_alternative<int>(val_left) ? static_cast<float>(std::get<int>(val_left)) : std::get<float>(val_left);
                    float f_right = std::holds_alternative<int>(val_right) ? static_cast<float>(std::get<int>(val_right)) : std::get<float>(val_right);
                    if (std::abs(f_right) < 1e-9) runtimeError("Division by zero."); // ��������� float � �����
                    stack.push(RuntimeStackItem(f_left / f_right));
                }
                else if (std::holds_alternative<int>(val_left) && std::holds_alternative<int>(val_right)) {
                    int i_right = std::get<int>(val_right);
                    if (i_right == 0) runtimeError("Division by zero.");
                    // ������������� �������
                    stack.push(RuntimeStackItem(std::get<int>(val_left) / i_right));
                }
                else {
                    runtimeError("Type error during DIV: Incompatible or uninitialized operands.");
                }
                break;
            }

                               // --- �������� ��������� ---
                               // ��������� ��������� - ������ int (0 ��� false, 1 ��� true)
            case RPNOpCode::CMP_EQ:
            case RPNOpCode::CMP_NE:
            case RPNOpCode::CMP_GT:
            case RPNOpCode::CMP_LT: {
                StoredValue val_right = getValueFromStackItem(popStack());
                StoredValue val_left = getValueFromStackItem(popStack());
                bool result = false;

                if (std::holds_alternative<float>(val_left) || std::holds_alternative<float>(val_right)) {
                    float f_left = std::holds_alternative<int>(val_left) ? static_cast<float>(std::get<int>(val_left)) : std::get<float>(val_left);
                    float f_right = std::holds_alternative<int>(val_right) ? static_cast<float>(std::get<int>(val_right)) : std::get<float>(val_right);
                    // ��� float ��������� �� ������ ���������/����������� ����� ���� ��������������
                    // �� ��� �������� ������� ������ ��������� ���������.
                    if (currentOp.opCode == RPNOpCode::CMP_EQ) result = (std::abs(f_left - f_right) < 1e-9); // ��������� float � epsilon
                    else if (currentOp.opCode == RPNOpCode::CMP_NE) result = (std::abs(f_left - f_right) >= 1e-9);
                    else if (currentOp.opCode == RPNOpCode::CMP_GT) result = (f_left > f_right);
                    else if (currentOp.opCode == RPNOpCode::CMP_LT) result = (f_left < f_right);
                }
                else if (std::holds_alternative<int>(val_left) && std::holds_alternative<int>(val_right)) {
                    int i_left = std::get<int>(val_left);
                    int i_right = std::get<int>(val_right);
                    if (currentOp.opCode == RPNOpCode::CMP_EQ) result = (i_left == i_right);
                    else if (currentOp.opCode == RPNOpCode::CMP_NE) result = (i_left != i_right);
                    else if (currentOp.opCode == RPNOpCode::CMP_GT) result = (i_left > i_right);
                    else if (currentOp.opCode == RPNOpCode::CMP_LT) result = (i_left < i_right);
                }
                else {
                    runtimeError("Type error during comparison: Incompatible or uninitialized operands.");
                }
                stack.push(RuntimeStackItem(result ? 1 : 0));
                break;
            }

                                  // --- �������� ������������ ---
            case RPNOpCode::ASSIGN: {
                // �� �����: ... Address ValueToAssign
                // ������ ������ ��� ����������, ��� ValueToAssign ��� ������� ���� (��� �������������)
                StoredValue valueToAssign = getValueFromStackItem(popStack()); // ��������
                RuntimeStackItem addressItem = popStack(); // ����� (���������� ��� ������� �������)
                setValueAtStackItemAddress(addressItem, valueToAssign);
                break;
            }

                                  // --- �������� ���������� ������� ---
            case RPNOpCode::INDEX: {
                // �� �����: ... ArrayBaseAddress(VarAddress) IndexValue(int)
                int elementRuntimeIndex = popInt(); // ������ ��������
                RuntimeStackItem arrayBaseAddrItem = popStack(); // ������� ����� ������� (��� VarAddress)

                if (arrayBaseAddrItem.type != RuntimeStackItem::ItemType::VAR_ADDRESS) {
                    runtimeError("Internal: Expected array base address (as VarAddress) for INDEX operation.");
                }
                size_t arrayTableIndex = std::get<VarAddress>(arrayBaseAddrItem.value).table_index;

                // �������� �� ������������� ������ (�������� ��� �����)
                if (elementRuntimeIndex < 0) {
                    runtimeError("Array index cannot be negative: " +
                        symbolTable.getSymbolName(arrayTableIndex) + "[" + std::to_string(elementRuntimeIndex) + "].");
                }
                // �������� �� ����� �� ������� ������� �������� � symbolTable.get/setArrayElementValue

                stack.push(RuntimeStackItem(ArrayElementAddress{ arrayTableIndex, elementRuntimeIndex }));
                break;
            }

                                 // --- ����/����� ---
            case RPNOpCode::READ_INT: {
                RuntimeStackItem addressItem = popStack(); // �����, ���� ������
                int valueRead;
                std::cout << "? int > ";
                std::cin >> valueRead;
                if (std::cin.fail()) {
                    std::cin.clear(); // ����� ������ ������
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // ������� ������
                    runtimeError("Invalid input. Integer expected for READ_INT.");
                }
                else {
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // ������� ������� ������
                }
                setValueAtStackItemAddress(addressItem, StoredValue(valueRead));
                break;
            }
            case RPNOpCode::READ_FLOAT: {
                RuntimeStackItem addressItem = popStack();
                float valueRead;
                std::cout << "? float > ";
                std::cin >> valueRead;
                if (std::cin.fail()) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    runtimeError("Invalid input. Float expected for READ_FLOAT.");
                }
                else {
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                }
                setValueAtStackItemAddress(addressItem, StoredValue(valueRead));
                break;
            }
            case RPNOpCode::WRITE_INT: {
                int valueToWrite = popInt();
                std::cout << valueToWrite << std::endl;
                break;
            }
            case RPNOpCode::WRITE_FLOAT: {
                float valueToWrite = popFloat();
                // ����� float � ��������� ���������
                std::cout << std::fixed << std::setw(6) << valueToWrite << std::endl;
                std::cout.unsetf(std::ios_base::floatfield); // ����� ����� fixed ��� ����������� �������
                break;
            }

                                       // --- �������� ---
            case RPNOpCode::JUMP:
                if (!currentOp.jumpTarget.has_value() || currentOp.jumpTarget.value() < 0) {
                    runtimeError("Internal: JUMP target address not set or invalid.");
                }
                instructionPointer = currentOp.jumpTarget.value();
                break;

            case RPNOpCode::JUMP_FALSE: {
                int condition = popInt(); // ��������� ������� (0 ��� 1)
                if (!currentOp.jumpTarget.has_value() || currentOp.jumpTarget.value() < 0) {
                    runtimeError("Internal: JUMP_FALSE target address not set or invalid.");
                }
                if (condition == 0) { // ���� ������� �����
                    instructionPointer = currentOp.jumpTarget.value();
                }
                // ���� �������, IP ��� ��������������� � ������� �� �����������
                break;
            }

                                      // --- �������������� ����� ---
            case RPNOpCode::CONVERT_TO_FLOAT: {
                int intVal = popInt(); // ��������� ��� int (���� ��� ��� float, �� ��������)
                // ����������: ������� StoredValue, ��������� ���, ����� ��������������
// RuntimeStackItem item = popStack();
// if (item.type == RuntimeStackItem::ItemType::INT_VALUE) {
//    stack.push(RuntimeStackItem(static_cast<float>(std::get<int>(item.value))));
// } else if (item.type == RuntimeStackItem::ItemType::FLOAT_VALUE) {
//    stack.push(item); // ��� float, ������ �� ������
// } else { runtimeError("CONVERT_TO_FLOAT expects a numeric value on stack."); }
                stack.push(RuntimeStackItem(static_cast<float>(intVal)));
                break;
            }
            case RPNOpCode::CONVERT_TO_INT: {
                float floatVal = popFloat(); // ��������� ��� float (���� ��� ��� int, �� ���������)
                // RuntimeStackItem item = popStack();
                // if (item.type == RuntimeStackItem::ItemType::FLOAT_VALUE) {
                //    stack.push(RuntimeStackItem(static_cast<int>(std::floor(std::get<float>(item.value)))));
                // } else if (item.type == RuntimeStackItem::ItemType::INT_VALUE) {
                //    stack.push(item); // ��� int
                // } else { runtimeError("CONVERT_TO_INT expects a numeric value on stack."); }
                stack.push(RuntimeStackItem(static_cast<int>(std::floor(floatVal)))); // ��������
                break;
            }

            default:
                runtimeError("Unknown RPN operation code encountered: " + std::to_string(static_cast<int>(currentOp.opCode)));
                break;
            }
        } // ����� try
    }
    catch (const std::runtime_error& e) {
        // ������, ������������� �� runtimeError ��� ������� �����, ��� ������ ���� ������������.
        // ����� �� ������ ���������� ����������.
        // ���� ������ �� ���� ������������ ErrorHandler'�� (��������, std::bad_alloc), �������� �����.
        if (!errorHandler.hasErrors()) { // ���� ��� �����, ���������������� ����������
            errorHandler.logRuntimeError("Unhandled std::runtime_error: " + std::string(e.what()));
        }
        // ���������� ������������ ��-�� ���������� ����������.
        return;
    }
    catch (const std::exception& e) { // ������ ����������� ����������
        errorHandler.logRuntimeError("Unhandled std::exception: " + std::string(e.what()));
        return;
    }
    catch (...) { // ��� ���������
        errorHandler.logRuntimeError("Unknown unhandled exception during execution.");
        return;
    }

    // �������� �� "��������" ���� � ����� (�����������, ����� ��������� �� ���������� ������ � ���)
    // if (!stack.isEmpty() && !errorHandler.hasErrors()) {
    //     errorHandler.logRuntimeError("Warning: Stack is not empty at the end of execution. Size: " + std::to_string(stack.size()));
    // }
}