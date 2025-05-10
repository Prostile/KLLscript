// interpreter.cpp
#include "interpreter.h"
#include <iostream> // Для cin/cout и отладки
#include <iomanip>  
#include <limits>   // Для std::numeric_limits (очистка cin)
#include <cmath>    // Для std::floor (при конвертации float в int)

// --- Реализация RuntimeStack ---
void RuntimeStack::push(const RuntimeStackItem& item) {
    if (stck.size() >= MAX_STACK_SIZE) {
        // Эту ошибку должен поймать и обработать Interpreter через runtimeError
        throw std::runtime_error("Runtime Stack overflow.");
    }
    stck.push(item);
}

RuntimeStackItem RuntimeStack::pop() {
    //std::cout << "DEBUG: RuntimeStack::pop() called. Stack size before pop: " << stck.size() << std::endl;
    if (stck.empty()) {
        //std::cout << "ERROR DEBUG: Attempt to pop from empty stack!" << std::endl; // Отладка
        // Эту ошибку должен поймать и обработать Interpreter через runtimeError
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

// --- Реализация Interpreter ---

Interpreter::Interpreter(const std::vector<RPNOperation>& code, SymbolTable& symTab, ErrorHandler& errHandler)
    : rpnCode(code), symbolTable(symTab), errorHandler(errHandler), instructionPointer(0) {
}

void Interpreter::runtimeError(const std::string& message) {
    errorHandler.logRuntimeError("RPN[" + std::to_string(instructionPointer - 1) + "]: " + message); // -1 т.к. IP уже инкрементирован
    // Для фатальных ошибок можно бросать исключение, чтобы прервать execute()
    throw std::runtime_error("Fatal runtime error occurred.");
}

// --- Вспомогательные методы для работы со стеком и значениями ---

RuntimeStackItem Interpreter::popStack() {
    try {
        return stack.pop();
    }
    catch (const std::runtime_error& e) { // Ловим stack underflow из RuntimeStack
        runtimeError(e.what()); // Передаем ошибку в наш ErrorHandler
        // Возвращаем фиктивное значение, чтобы компилятор был доволен, но throw уже был
        return RuntimeStackItem(0);
    }
}

int Interpreter::popInt() {
    RuntimeStackItem item = popStack();
    if (item.type == RuntimeStackItem::ItemType::INT_VALUE) {
        return std::get<int>(item.value);
    }
    else if (item.type == RuntimeStackItem::ItemType::FLOAT_VALUE) {
        // Неявное усечение float до int при извлечении как int
        // Можно добавить предупреждение, если это нежелательное поведение
        // errorHandler.logRuntimeError("Warning: Implicit conversion from float to int during popInt. Value truncated.");
        return static_cast<int>(std::floor(std::get<float>(item.value))); // Усечение к меньшему (как в C)
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
    return 0; // Недостижимо из-за runtimeError
}

float Interpreter::popFloat() {
    RuntimeStackItem item = popStack();
    if (item.type == RuntimeStackItem::ItemType::FLOAT_VALUE) {
        return std::get<float>(item.value);
    }
    else if (item.type == RuntimeStackItem::ItemType::INT_VALUE) {
        // Неявное преобразование int во float
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
    return 0.0f; // Недостижимо
}

VarAddress Interpreter::popVarAddress() {
    RuntimeStackItem item = popStack();
    if (item.type == RuntimeStackItem::ItemType::VAR_ADDRESS) {
        return std::get<VarAddress>(item.value);
    }
    runtimeError("Type mismatch on stack: Expected variable address.");
    return VarAddress{ 0 }; // Недостижимо
}

ArrayElementAddress Interpreter::popArrayElementAddress() {
    RuntimeStackItem item = popStack();
    if (item.type == RuntimeStackItem::ItemType::ARRAY_ELEMENT_ADDRESS) {
        return std::get<ArrayElementAddress>(item.value);
    }
    runtimeError("Type mismatch on stack: Expected array element address.");
    return ArrayElementAddress{ 0,0 }; // Недостижимо
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
        if (!optVal) { // symbolTable.getVariableValue должен был бы сам вызвать errorHandler при ошибке
            runtimeError("Failed to retrieve value for variable (index: " + std::to_string(varIndex) + ").");
            return std::monostate{};
        }
        if (std::holds_alternative<std::monostate>(optVal.value())) {
            // Не фатальная ошибка здесь, пусть вызывающий код решает (например, арифметическая операция)
            // errorHandler.logRuntimeError("Variable '" + symbolTable.getSymbolName(varIndex) + "' used before initialization.");
        }
        return optVal.value();
    }
    else if (item.type == RuntimeStackItem::ItemType::ARRAY_ELEMENT_ADDRESS) {
        const auto& addr = std::get<ArrayElementAddress>(item.value);
        auto optVal = symbolTable.getArrayElementValue(addr.array_table_index, static_cast<size_t>(addr.element_runtime_index));
        if (!optVal) { // getArrayElementValue сам логирует ошибку выхода за границы
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
    return std::monostate{}; // Недостижимо
}

void Interpreter::setValueAtStackItemAddress(const RuntimeStackItem& addressItem, const StoredValue& valueToSet) {
    if (addressItem.type == RuntimeStackItem::ItemType::VAR_ADDRESS) {
        size_t varIndex = std::get<VarAddress>(addressItem.value).table_index;
        if (!symbolTable.setVariableValue(varIndex, valueToSet)) {
            // symbolTable.setVariableValue должен был вызвать errorHandler
            runtimeError("Failed to set value for variable (index: " + std::to_string(varIndex) + ").");
        }
    }
    else if (addressItem.type == RuntimeStackItem::ItemType::ARRAY_ELEMENT_ADDRESS) {
        const auto& addr = std::get<ArrayElementAddress>(addressItem.value);
        if (!symbolTable.setArrayElementValue(addr.array_table_index, static_cast<size_t>(addr.element_runtime_index), valueToSet)) {
            // symbolTable.setArrayElementValue должен был вызвать errorHandler
            runtimeError("Failed to set value for array element '" +
                symbolTable.getSymbolName(addr.array_table_index) +
                "[" + std::to_string(addr.element_runtime_index) + "]'.");
        }
    }
    else {
        runtimeError("Invalid address type on stack for setValue operation.");
    }
}

// Метод execute() будет в следующей части
// interpreter.cpp (Продолжение)
// ... (код RuntimeStack, конструктор Interpreter, вспомогательные методы из Части 1) ...

void Interpreter::execute() {
    instructionPointer = 0;
    stack.clear(); // Очищаем стек перед новым запуском

    // Защита от слишком длинного выполнения (бесконечного цикла в ОПС)
    const int MAX_EXECUTED_INSTRUCTIONS = 10000000; // 10 миллионов операций
    int executedCounter = 0;

    try { // Основной блок try-catch для перехвата runtimeError и других исключений
        while (instructionPointer >= 0 && static_cast<size_t>(instructionPointer) < rpnCode.size()) {
            if (executedCounter++ > MAX_EXECUTED_INSTRUCTIONS) {
                runtimeError("Maximum instruction execution limit reached. Possible infinite loop.");
                // runtimeError бросит исключение, которое прервет цикл
            }

            const RPNOperation& currentOp = rpnCode[instructionPointer];
            instructionPointer++; // Инкрементируем до выполнения, чтобы переходы работали корректно

            switch (currentOp.opCode) {
                // --- Операнды ---
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
                // На стек кладется адрес *базы* массива (индекс в таблице символов).
                // Для работы с ним как с ArrayElementAddress ему нужен еще и runtime_index,
                // который будет вычислен операцией INDEX.
                // Поэтому здесь мы также используем VarAddress, т.к. это просто индекс в symbolTable.
                // Операция INDEX затем возьмет этот базовый адрес и индекс элемента.
                stack.push(RuntimeStackItem(VarAddress{ currentOp.symbolIndex.value() }));
                break;

                // --- Арифметические операции ---
            case RPNOpCode::ADD: {
                // Операнды извлекаются как float, т.к. float "старше" int
                // float right = popFloat();
                // float left = popFloat();
                // Если оба исходных были int, результат должен быть int.
                // Если хотя бы один float, результат float.
                // popFloat уже сделал преобразование int->float, если нужно.
                // Теперь нужно определить, был ли результат изначально int.
                // Это можно сделать, проверив типы исходных элементов на стеке,
                // но popFloat() их уже потребил.
                // Проще: если результат сложения float-чисел является целым, то храним как int.
                // Но это не всегда корректно (1.0 + 2.0 = 3.0, должно быть float 3.0, если хоть один был float)
                // Правило: если хотя бы один операнд был float, результат float.
                // popFloat() вернул float. Если он был из int, он стал float.
                // Значит, если хоть один из них изначально был float, результат float.
                // Мы не можем это узнать напрямую здесь без анализа типов до popFloat.
                // Решение: интерпретатор выполняет операции во float, если возможно,
                // а CONVERT_TO_INT используется для явного приведения.
                // Или, если язык требует сохранения целочисленной арифметики:
                // RuntimeStackItem item_right = stack.pop(); // Нужно было бы заглянуть, а не извлечь
                // RuntimeStackItem item_left = stack.pop();   // (Сложно без peek)
                // Откатимся к простому варианту: операции выполняются над наиболее общим типом (float).
                // Если оба операнда на стеке были int (до popFloat), то и результат должен быть int.

                // Пересмотренный подход: извлекаем как StoredValue, затем работаем с типами
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

                // Деление всегда производим во float, чтобы избежать потери точности при int/int,
                // если язык этого не требует строго (например, 5/2 = 2.5, а не 2).
                // Если строго целочисленное, то нужна другая логика.
                // Предположим, что / всегда дает float, если хотя бы один операнд float, или если int/int дает дробное.
                // Для простоты: если оба int, делим как int. Если хоть один float, делим как float.
                if (std::holds_alternative<float>(val_left) || std::holds_alternative<float>(val_right)) {
                    float f_left = std::holds_alternative<int>(val_left) ? static_cast<float>(std::get<int>(val_left)) : std::get<float>(val_left);
                    float f_right = std::holds_alternative<int>(val_right) ? static_cast<float>(std::get<int>(val_right)) : std::get<float>(val_right);
                    if (std::abs(f_right) < 1e-9) runtimeError("Division by zero."); // Сравнение float с нулем
                    stack.push(RuntimeStackItem(f_left / f_right));
                }
                else if (std::holds_alternative<int>(val_left) && std::holds_alternative<int>(val_right)) {
                    int i_right = std::get<int>(val_right);
                    if (i_right == 0) runtimeError("Division by zero.");
                    // Целочисленное деление
                    stack.push(RuntimeStackItem(std::get<int>(val_left) / i_right));
                }
                else {
                    runtimeError("Type error during DIV: Incompatible or uninitialized operands.");
                }
                break;
            }

                               // --- Операции сравнения ---
                               // Результат сравнения - всегда int (0 для false, 1 для true)
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
                    // Для float сравнение на точное равенство/неравенство может быть проблематичным
                    // но для учебного проекта прямое сравнение допустимо.
                    if (currentOp.opCode == RPNOpCode::CMP_EQ) result = (std::abs(f_left - f_right) < 1e-9); // Сравнение float с epsilon
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

                                  // --- Операция присваивания ---
            case RPNOpCode::ASSIGN: {
                // На стеке: ... Address ValueToAssign
                // Парсер должен был обеспечить, что ValueToAssign уже нужного типа (или конвертирован)
                StoredValue valueToAssign = getValueFromStackItem(popStack()); // Значение
                RuntimeStackItem addressItem = popStack(); // Адрес (переменная или элемент массива)
                setValueAtStackItemAddress(addressItem, valueToAssign);
                break;
            }

                                  // --- Операция индексации массива ---
            case RPNOpCode::INDEX: {
                // На стеке: ... ArrayBaseAddress(VarAddress) IndexValue(int)
                int elementRuntimeIndex = popInt(); // Индекс элемента
                RuntimeStackItem arrayBaseAddrItem = popStack(); // Базовый адрес массива (как VarAddress)

                if (arrayBaseAddrItem.type != RuntimeStackItem::ItemType::VAR_ADDRESS) {
                    runtimeError("Internal: Expected array base address (as VarAddress) for INDEX operation.");
                }
                size_t arrayTableIndex = std::get<VarAddress>(arrayBaseAddrItem.value).table_index;

                // Проверка на отрицательный индекс (парсером или здесь)
                if (elementRuntimeIndex < 0) {
                    runtimeError("Array index cannot be negative: " +
                        symbolTable.getSymbolName(arrayTableIndex) + "[" + std::to_string(elementRuntimeIndex) + "].");
                }
                // Проверка на выход за верхнюю границу делается в symbolTable.get/setArrayElementValue

                stack.push(RuntimeStackItem(ArrayElementAddress{ arrayTableIndex, elementRuntimeIndex }));
                break;
            }

                                 // --- Ввод/Вывод ---
            case RPNOpCode::READ_INT: {
                RuntimeStackItem addressItem = popStack(); // Адрес, куда читать
                int valueRead;
                std::cout << "? int > ";
                std::cin >> valueRead;
                if (std::cin.fail()) {
                    std::cin.clear(); // Сброс флагов ошибок
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Очистка буфера
                    runtimeError("Invalid input. Integer expected for READ_INT.");
                }
                else {
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Очистка остатка буфера
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
                // Вывод float с некоторой точностью
                std::cout << std::fixed << std::setw(6) << valueToWrite << std::endl;
                std::cout.unsetf(std::ios_base::floatfield); // Сброс флага fixed для последующих выводов
                break;
            }

                                       // --- Переходы ---
            case RPNOpCode::JUMP:
                if (!currentOp.jumpTarget.has_value() || currentOp.jumpTarget.value() < 0) {
                    runtimeError("Internal: JUMP target address not set or invalid.");
                }
                instructionPointer = currentOp.jumpTarget.value();
                break;

            case RPNOpCode::JUMP_FALSE: {
                int condition = popInt(); // Результат условия (0 или 1)
                if (!currentOp.jumpTarget.has_value() || currentOp.jumpTarget.value() < 0) {
                    runtimeError("Internal: JUMP_FALSE target address not set or invalid.");
                }
                if (condition == 0) { // Если условие ложно
                    instructionPointer = currentOp.jumpTarget.value();
                }
                // Если истинно, IP уже инкрементирован и переход не выполняется
                break;
            }

                                      // --- Преобразование типов ---
            case RPNOpCode::CONVERT_TO_FLOAT: {
                int intVal = popInt(); // Извлекаем как int (если там был float, он усечется)
                // Правильнее: извлечь StoredValue, проверить тип, потом конвертировать
// RuntimeStackItem item = popStack();
// if (item.type == RuntimeStackItem::ItemType::INT_VALUE) {
//    stack.push(RuntimeStackItem(static_cast<float>(std::get<int>(item.value))));
// } else if (item.type == RuntimeStackItem::ItemType::FLOAT_VALUE) {
//    stack.push(item); // Уже float, ничего не делаем
// } else { runtimeError("CONVERT_TO_FLOAT expects a numeric value on stack."); }
                stack.push(RuntimeStackItem(static_cast<float>(intVal)));
                break;
            }
            case RPNOpCode::CONVERT_TO_INT: {
                float floatVal = popFloat(); // Извлекаем как float (если там был int, он повысится)
                // RuntimeStackItem item = popStack();
                // if (item.type == RuntimeStackItem::ItemType::FLOAT_VALUE) {
                //    stack.push(RuntimeStackItem(static_cast<int>(std::floor(std::get<float>(item.value)))));
                // } else if (item.type == RuntimeStackItem::ItemType::INT_VALUE) {
                //    stack.push(item); // Уже int
                // } else { runtimeError("CONVERT_TO_INT expects a numeric value on stack."); }
                stack.push(RuntimeStackItem(static_cast<int>(std::floor(floatVal)))); // Усечение
                break;
            }

            default:
                runtimeError("Unknown RPN operation code encountered: " + std::to_string(static_cast<int>(currentOp.opCode)));
                break;
            }
        } // Конец try
    }
    catch (const std::runtime_error& e) {
        // Ошибки, перехваченные из runtimeError или методов стека, уже должны быть залогированы.
        // Здесь мы просто прекращаем выполнение.
        // Если ошибка не была залогирована ErrorHandler'ом (например, std::bad_alloc), логируем здесь.
        if (!errorHandler.hasErrors()) { // Если это новое, незалогированное исключение
            errorHandler.logRuntimeError("Unhandled std::runtime_error: " + std::string(e.what()));
        }
        // Выполнение прекращается из-за брошенного исключения.
        return;
    }
    catch (const std::exception& e) { // Другие стандартные исключения
        errorHandler.logRuntimeError("Unhandled std::exception: " + std::string(e.what()));
        return;
    }
    catch (...) { // Все остальное
        errorHandler.logRuntimeError("Unknown unhandled exception during execution.");
        return;
    }

    // Проверка на "непустой" стек в конце (опционально, может указывать на логические ошибки в ОПС)
    // if (!stack.isEmpty() && !errorHandler.hasErrors()) {
    //     errorHandler.logRuntimeError("Warning: Stack is not empty at the end of execution. Size: " + std::to_string(stack.size()));
    // }
}