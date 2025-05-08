#include "SymbolTable.h"
#include <iostream>
#include <stdexcept> // ��� std::visit

// �����������
SymbolTable::SymbolTable() {
    keywordMap["int"] = TokenType::T_KW_INT;
    keywordMap["arr"] = TokenType::T_KW_ARR;
    keywordMap["float"] = TokenType::T_KW_FLOAT; // ���������
    keywordMap["bool"] = TokenType::T_KW_BOOL;   // ���������
    keywordMap["if"] = TokenType::T_KW_IF;
    keywordMap["else"] = TokenType::T_KW_ELSE;
    keywordMap["while"] = TokenType::T_KW_WHILE;
    keywordMap["begin"] = TokenType::T_KW_BEGIN;
    keywordMap["end"] = TokenType::T_KW_END;
    keywordMap["cin"] = TokenType::T_KW_CIN;
    keywordMap["cout"] = TokenType::T_KW_COUT;
    keywordMap["true"] = TokenType::T_KW_TRUE;   // ��������� (��� �����, �� ��������� �����)
    keywordMap["false"] = TokenType::T_KW_FALSE; // ��������� (��� �����, �� ��������� �����)
    keywordMap["not"] = TokenType::T_KW_NOT;     // ���������
}

// --- ��������������� ����������� ������� ---
bool SymbolTable::isVariableType(SymbolType type) {
    return type == SymbolType::VARIABLE_INT ||
        type == SymbolType::VARIABLE_FLOAT ||
        type == SymbolType::VARIABLE_BOOL;
}

bool SymbolTable::isArrayType(SymbolType type) {
    return type == SymbolType::ARRAY_INT ||
        type == SymbolType::ARRAY_FLOAT ||
        type == SymbolType::ARRAY_BOOL;
}

SymbolType SymbolTable::getArrayElementType(SymbolType arrayType) {
    if (arrayType == SymbolType::ARRAY_INT) return SymbolType::VARIABLE_INT;
    if (arrayType == SymbolType::ARRAY_FLOAT) return SymbolType::VARIABLE_FLOAT;
    if (arrayType == SymbolType::ARRAY_BOOL) return SymbolType::VARIABLE_BOOL;
    // ���� �� ������ �������, ���� arrayType ������������� ��� �������
    throw std::runtime_error("Internal Error: Invalid array type in getArrayElementType.");
}


// ��������, �������� �� ��� �������� ������
std::optional<TokenType> SymbolTable::getKeywordType(const std::string& name) const {
    auto it = keywordMap.find(name);
    if (it != keywordMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

// ����� ������� �� �����
std::optional<size_t> SymbolTable::findSymbol(const std::string& name) const {
    auto it = nameToIndexMap.find(name);
    if (it != nameToIndexMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

// ���������� ������� (���������� ��� �������)
std::optional<size_t> SymbolTable::addSymbol(const std::string& name, SymbolType type, int declarationLine) {
    if (findSymbol(name).has_value() || getKeywordType(name).has_value()) {
        // TODO: ������������� � ErrorHandler
        // std::cerr << "Error line " << declarationLine << ": Symbol '" << name << "' already defined or is a keyword." << std::endl;
        return std::nullopt;
    }

    size_t newIndex = symbols.size();
    // ���������� ����������� SymbolInfo, ������� ��������� �������� �� ���������
    symbols.emplace_back(name, type, declarationLine);
    nameToIndexMap[name] = newIndex;
    return newIndex;
}

// // ������ ������ (����� �����������������, ���� �����)
// std::optional<size_t> SymbolTable::addVariable(const std::string& name, SymbolType type, int declarationLine) {
//      if (!isVariableType(type)) {
//           // TODO: Error
//           return std::nullopt;
//      }
//      return addSymbol(name, type, declarationLine);
// }
// std::optional<size_t> SymbolTable::addArray(const std::string& name, SymbolType type, int declarationLine) {
//      if (!isArrayType(type)) {
//           // TODO: Error
//           return std::nullopt;
//      }
//      return addSymbol(name, type, declarationLine);
// }


// ��������� SymbolInfo �� �������
const SymbolInfo& SymbolTable::getSymbolInfo(size_t index) const {
    if (index >= symbols.size()) {
        std::cerr << "Internal Error: Symbol index " << index << " out of bounds." << std::endl;
        exit(EXIT_FAILURE);
    }
    return symbols[index];
}
SymbolInfo& SymbolTable::getSymbolInfo(size_t index) {
    if (index >= symbols.size()) {
        std::cerr << "Internal Error: Symbol index " << index << " out of bounds." << std::endl;
        exit(EXIT_FAILURE);
    }
    return symbols[index];
}

// ��������� ���� ������� �� �������
SymbolType SymbolTable::getSymbolType(size_t index) const {
    return getSymbolInfo(index).type;
}

// ��������� ����� ������� �� �������
const std::string& SymbolTable::getSymbolName(size_t index) const {
    return getSymbolInfo(index).name;
}

// ��������, �������� �� ������ ��������
bool SymbolTable::isArray(size_t index) const {
    return isArrayType(getSymbolType(index));
}

// ��������, �������� �� ������ ���������� (�� ��������)
bool SymbolTable::isVariable(size_t index) const {
    return isVariableType(getSymbolType(index));
}

// ��������� �������� ����������
bool SymbolTable::setVariableValue(size_t index, const SymbolValue& value) {
    SymbolInfo& info = getSymbolInfo(index);
    if (!isVariable(index)) {
        // TODO: ������ - ������� ��������� �������� �������
        std::cerr << "Runtime Error: Cannot assign value directly to array '" << info.name << "'." << std::endl;
        return false;
    }
    if (!checkAssignmentTypeCompatibility(info.type, value)) {
        // TODO: ������ - ��������������� �����
        // ��������� �� ������ ����� ���� ����� ��������� � checkAssignmentTypeCompatibility
        std::cerr << "Runtime Error: Type mismatch assigning to variable '" << info.name << "'." << std::endl;
        return false;
    }

    // ��������� ������������ � ��������� ������� ����������� int � float
    if (info.type == SymbolType::VARIABLE_FLOAT && std::holds_alternative<int>(value)) {
        info.value = static_cast<double>(std::get<int>(value)); // �������� int � double
    }
    else {
        info.value = value; // ���� ��������� ��� value ��� float/bool
    }
    return true;
}

// ��������� �������� ����������
std::optional<SymbolValue> SymbolTable::getVariableValue(size_t index) const {
    const SymbolInfo& info = getSymbolInfo(index);
    if (!isVariable(index)) {
        // TODO: ������ - ������� �������� �������� �������
        std::cerr << "Runtime Error: Cannot get single value from array '" << info.name << "'." << std::endl;
        return std::nullopt;
    }
    return info.value;
}

// ������������� (��������� ������) ��� �������
bool SymbolTable::resizeArray(size_t index, size_t size) {
    SymbolInfo& info = getSymbolInfo(index);
    if (!isArray(index)) {
        // TODO: ������
        std::cerr << "Runtime Error: Cannot resize non-array symbol '" << info.name << "'." << std::endl;
        return false;
    }
    if (info.arraySize > 0 && info.arrayData.size() == info.arraySize) {
        // �������������� ��� ������: ��������� ����������� �������?
        // ���� ��������, ������ ������ ��������
        std::cerr << "Warning: Resizing already allocated array '" << info.name << "'." << std::endl;
    }
    info.arraySize = size;
    // �������� �������� �� ��������� ��� ���� ��������
    SymbolValue defaultValue = getDefaultValueForType(getArrayElementType(info.type));
    info.arrayData.assign(size, defaultValue); // ��������� ���������� �� ���������
    return true;
}

// ��������� ������� �������
std::optional<size_t> SymbolTable::getArraySize(size_t index) const {
    const SymbolInfo& info = getSymbolInfo(index);
    if (!isArray(index)) {
        return std::nullopt;
    }
    return info.arraySize;
}

// ��������� �������� �������� �������
bool SymbolTable::setArrayElementValue(size_t arrayIndex, size_t elementIndex, const SymbolValue& value) {
    SymbolInfo& info = getSymbolInfo(arrayIndex);
    if (!isArray(arrayIndex)) {
        std::cerr << "Runtime Error: Symbol '" << info.name << "' is not an array." << std::endl;
        return false;
    }
    if (elementIndex >= info.arraySize) {
        std::cerr << "Runtime Error: Array index out of bounds for '" << info.name
            << "' (index: " << elementIndex << ", size: " << info.arraySize << ")." << std::endl;
        return false;
    }

    SymbolType elementType = getArrayElementType(info.type);
    if (!checkAssignmentTypeCompatibility(elementType, value)) {
        std::cerr << "Runtime Error: Type mismatch assigning to element " << elementIndex << " of array '" << info.name << "'." << std::endl;
        return false;
    }

    // ��������� ������������ � ��������� ������� ����������� int � float
    if (elementType == SymbolType::VARIABLE_FLOAT && std::holds_alternative<int>(value)) {
        info.arrayData[elementIndex] = static_cast<double>(std::get<int>(value));
    }
    else {
        info.arrayData[elementIndex] = value;
    }
    return true;
}

// ��������� �������� �������� �������
std::optional<SymbolValue> SymbolTable::getArrayElementValue(size_t arrayIndex, size_t elementIndex) const {
    const SymbolInfo& info = getSymbolInfo(arrayIndex);
    if (!isArray(arrayIndex)) {
        std::cerr << "Runtime Error: Symbol '" << info.name << "' is not an array." << std::endl;
        return std::nullopt;
    }
    if (elementIndex >= info.arraySize) {
        std::cerr << "Runtime Error: Array index out of bounds for '" << info.name
            << "' (index: " << elementIndex << ", size: " << info.arraySize << ")." << std::endl;
        return std::nullopt;
    }
    return info.arrayData[elementIndex];
}

// �������� ������������� ����� ��� ������������ (target = value)
bool SymbolTable::checkAssignmentTypeCompatibility(SymbolType targetType, const SymbolValue& value) const {
    // ���������� std::visit ��� �������� ���� � variant ��� ���������� (�����)
    bool compatible = false;
    std::visit([targetType, &compatible](auto&& arg) {
        using T = std::decay_t<decltype(arg)>; // ���������� ��� ������ variant (int, double, bool)
        switch (targetType) {
        case SymbolType::VARIABLE_INT:
        case SymbolType::ARRAY_INT:
            compatible = std::is_same_v<T, int>; // int = int
            break;
        case SymbolType::VARIABLE_FLOAT:
        case SymbolType::ARRAY_FLOAT:
            compatible = std::is_same_v<T, int> || std::is_same_v<T, double>; // float = int || float = float
            break;
        case SymbolType::VARIABLE_BOOL:
        case SymbolType::ARRAY_BOOL:
            compatible = std::is_same_v<T, bool>; // bool = bool
            break;
        }
        }, value);
    return compatible;
}
// ���������� ��� �������� ������������� ���� SymbolType
bool SymbolTable::checkAssignmentTypeCompatibility(SymbolType targetType, SymbolType valueType) const {
    if (targetType == valueType) return true; // ���� ���������

    // ������� ���������� int � float ��� ������������
    if ((targetType == SymbolType::VARIABLE_FLOAT || targetType == SymbolType::ARRAY_FLOAT) &&
        (valueType == SymbolType::VARIABLE_INT || valueType == SymbolType::ARRAY_INT)) {
        return true;
    }
    // ��� ��������� ������������ ����� ���������
    return false;
}


// ��������� �������� ���������� �������� � �������
size_t SymbolTable::getTableSize() const {
    return symbols.size();
}   