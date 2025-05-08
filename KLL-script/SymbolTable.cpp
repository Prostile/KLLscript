#include "SymbolTable.h"
#include <iostream> // ��� ��������� ��������� �� �������

// �����������: �������������� ������� � ��������� �����
SymbolTable::SymbolTable() {
    // ������������� ����� �������� ����
    keywordMap["int"] = TokenType::T_KW_INT;
    keywordMap["arr"] = TokenType::T_KW_ARR;
    keywordMap["if"] = TokenType::T_KW_IF;
    keywordMap["else"] = TokenType::T_KW_ELSE;
    keywordMap["while"] = TokenType::T_KW_WHILE;
    keywordMap["begin"] = TokenType::T_KW_BEGIN;
    keywordMap["end"] = TokenType::T_KW_END;
    keywordMap["cin"] = TokenType::T_KW_CIN;
    keywordMap["cout"] = TokenType::T_KW_COUT;
    // �������� float, bool ��� ����������
}

// ��������, �������� �� ��� �������� ������
std::optional<TokenType> SymbolTable::getKeywordType(const std::string& name) const {
    auto it = keywordMap.find(name);
    if (it != keywordMap.end()) {
        return it->second; // ������� �������� �����
    }
    return std::nullopt; // �� �������� �����
}

// ����� ������� �� �����
std::optional<size_t> SymbolTable::findSymbol(const std::string& name) const {
    auto it = nameToIndexMap.find(name);
    if (it != nameToIndexMap.end()) {
        return it->second; // ���������� ������ ���������� �������
    }
    return std::nullopt; // ������ �� ������
}

// ���������� ����������
std::optional<size_t> SymbolTable::addVariable(const std::string& name, SymbolType type, int declarationLine) {
    if (findSymbol(name).has_value() || getKeywordType(name).has_value()) {
        // ������: ��������������� ������� ��� ������������� ����� ��������� �����
        // TODO: ������������� � ErrorHandler
        // std::cerr << "Error: Symbol '" << name << "' already defined." << std::endl;
        return std::nullopt;
    }

    // ��������� ������������ ���� ��� ����������
    if (type != SymbolType::VARIABLE_INT /* && type != SymbolType::VARIABLE_FLOAT && type != SymbolType::VARIABLE_BOOL */) {
        // TODO: ������������� � ErrorHandler
        // std::cerr << "Internal Error: Attempting to add variable '" << name << "' with non-variable type." << std::endl;
        return std::nullopt;
    }


    size_t newIndex = symbols.size();
    symbols.emplace_back(name, type, declarationLine); // ������� ����� SymbolInfo
    nameToIndexMap[name] = newIndex;                   // ��������� � ����� ��� �������� ������
    return newIndex;
}

// ���������� �������
std::optional<size_t> SymbolTable::addArray(const std::string& name, SymbolType type, int declarationLine) {
    if (findSymbol(name).has_value() || getKeywordType(name).has_value()) {
        // ������: ��������������� ������� ��� ������������� ����� ��������� �����
        // TODO: ������������� � ErrorHandler
        // std::cerr << "Error: Symbol '" << name << "' already defined." << std::endl;
        return std::nullopt;
    }

    // ��������� ������������ ���� ��� �������
    if (type != SymbolType::ARRAY_INT /* && type != SymbolType::ARRAY_FLOAT && type != SymbolType::ARRAY_BOOL */) {
        // TODO: ������������� � ErrorHandler
        // std::cerr << "Internal Error: Attempting to add array '" << name << "' with non-array type." << std::endl;
        return std::nullopt;
    }

    size_t newIndex = symbols.size();
    symbols.emplace_back(name, type, declarationLine); // ������� ����� SymbolInfo
    nameToIndexMap[name] = newIndex;                   // ��������� � ����� ��� �������� ������
    // ������ ������� ����� ���������� ����� ����� resizeArray
    return newIndex;
}

// ��������� SymbolInfo �� �������
const SymbolInfo& SymbolTable::getSymbolInfo(size_t index) const {
    if (index >= symbols.size()) {
        // TODO: ��������� ������ ������ �� �������
        std::cerr << "Internal Error: Symbol index out of bounds." << std::endl;
        exit(EXIT_FAILURE);
    }
    return symbols[index];
}
SymbolInfo& SymbolTable::getSymbolInfo(size_t index) {
    if (index >= symbols.size()) {
        // TODO: ��������� ������ ������ �� �������
        std::cerr << "Internal Error: Symbol index out of bounds." << std::endl;
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


// ��������� �������� ����������
bool SymbolTable::setVariableValue(size_t index, const SymbolValue& val) {
    SymbolInfo& info = getSymbolInfo(index);
    // ��������, ��� ��� ������������� ����������, � �� ������
    if (info.type != SymbolType::VARIABLE_INT /* && info.type != SymbolType::VARIABLE_FLOAT && ... */) {
        // TODO: ������ - ������� ��������� �������� ������� ��� ����������
        return false;
    }
    // TODO: ����� �������� �������� ������������� ����� val � info.type
    info.value = val;
    return true;
}

// ��������� �������� ����������
std::optional<SymbolValue> SymbolTable::getVariableValue(size_t index) const {
    const SymbolInfo& info = getSymbolInfo(index);
    if (info.type != SymbolType::VARIABLE_INT /* && info.type != SymbolType::VARIABLE_FLOAT && ... */) {
        // TODO: ������ - ������� �������� �������� ������� ��� ����������
        return std::nullopt;
    }
    return info.value;
}

// ������������� (��������� ������) ��� �������
bool SymbolTable::resizeArray(size_t index, size_t size) {
    SymbolInfo& info = getSymbolInfo(index);
    // ��������, ��� ��� ������������� ������
    if (info.type != SymbolType::ARRAY_INT /* && info.type != SymbolType::ARRAY_FLOAT && ... */) {
        // TODO: ������ - ������� �������� ������ ���������� ��� �������
        return false;
    }
    if (info.arraySize > 0) {
        // TODO: ������ - ��������� ����������� ������� �������? ��� ���������?
    }
    info.arraySize = size;
    // �������� ������ � �������������� ������ (��� ���������� �� ��������� ��� ����)
    // ��� int ��� ����� ������ �����.
    info.arrayData.resize(size, SymbolValue(0));
    return true;
}

// ��������� ������� �������
std::optional<size_t> SymbolTable::getArraySize(size_t index) const {
    const SymbolInfo& info = getSymbolInfo(index);
    if (info.type != SymbolType::ARRAY_INT /* && info.type != SymbolType::ARRAY_FLOAT && ... */) {
        return std::nullopt;
    }
    return info.arraySize;
}


// ��������� �������� �������� �������
bool SymbolTable::setArrayElementValue(size_t arrayIndex, size_t elementIndex, const SymbolValue& val) {
    SymbolInfo& info = getSymbolInfo(arrayIndex);
    if (info.type != SymbolType::ARRAY_INT /* && info.type != SymbolType::ARRAY_FLOAT && ... */) {
        // TODO: ������ - �� ������
        return false;
    }
    if (elementIndex >= info.arraySize) {
        // TODO: ������ ������� ���������� - ����� �� ������� �������
        std::cerr << "Runtime Error: Array index out of bounds for '" << info.name
            << "' (index: " << elementIndex << ", size: " << info.arraySize << ")." << std::endl;
        // exit(EXIT_FAILURE); // ��� ������� false
        return false;
    }
    // TODO: �������� ������������� ����� val � ���� ��������� �������
    info.arrayData[elementIndex] = val;
    return true;
}

// ��������� �������� �������� �������
std::optional<SymbolValue> SymbolTable::getArrayElementValue(size_t arrayIndex, size_t elementIndex) const {
    const SymbolInfo& info = getSymbolInfo(arrayIndex);
    if (info.type != SymbolType::ARRAY_INT /* && info.type != SymbolType::ARRAY_FLOAT && ... */) {
        // TODO: ������ - �� ������
        return std::nullopt;
    }
    if (elementIndex >= info.arraySize) {
        // TODO: ������ ������� ���������� - ����� �� ������� �������
        std::cerr << "Runtime Error: Array index out of bounds for '" << info.name
            << "' (index: " << elementIndex << ", size: " << info.arraySize << ")." << std::endl;
        // exit(EXIT_FAILURE); // ��� ������� nullopt
        return std::nullopt;
    }
    return info.arrayData[elementIndex];
}

// ��������� �������� ���������� �������� � �������
size_t SymbolTable::getTableSize() const {
    return symbols.size();
}