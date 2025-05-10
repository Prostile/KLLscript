// symbol_table.cpp
#include "symbol_table.h"

SymbolTable::SymbolTable(ErrorHandler& errHandler) : errorHandler(errHandler) {
    // ������������� ����� �������� ����
    keywordMap["int"] = TokenType::T_KW_INT;
    keywordMap["float"] = TokenType::T_KW_FLOAT;
    keywordMap["arr"] = TokenType::T_KW_ARR;
    keywordMap["if"] = TokenType::T_KW_IF;
    keywordMap["else"] = TokenType::T_KW_ELSE;
    keywordMap["while"] = TokenType::T_KW_WHILE;
    keywordMap["begin"] = TokenType::T_KW_BEGIN;
    keywordMap["end"] = TokenType::T_KW_END;
    keywordMap["cin"] = TokenType::T_KW_CIN;
    keywordMap["cout"] = TokenType::T_KW_COUT;
}

std::optional<TokenType> SymbolTable::getKeywordType(const std::string& name) const {
    auto it = keywordMap.find(name);
    if (it != keywordMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<size_t> SymbolTable::findSymbol(const std::string& name) const {
    auto it = nameToIndexMap.find(name);
    if (it != nameToIndexMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<size_t> SymbolTable::addVariable(const std::string& name, SymbolType type, int declarationLine) {
    if (getKeywordType(name).has_value()) {
        errorHandler.logSemanticError("Identifier '" + name + "' is a reserved keyword.", declarationLine);
        return std::nullopt;
    }
    if (findSymbol(name).has_value()) {
        errorHandler.logSemanticError("Variable '" + name + "' already declared.", declarationLine);
        return std::nullopt;
    }

    if (type != SymbolType::VARIABLE_INT && type != SymbolType::VARIABLE_FLOAT) {
        // ��� �������� ������ ��� ���������� ���������������, ������ ������ ���������� ���������� ���
        errorHandler.logSemanticError("Internal: Invalid type for variable '" + name + "'.", declarationLine);
        return std::nullopt;
    }

    size_t newIndex = symbols.size();
    symbols.emplace_back(name, type, declarationLine);
    nameToIndexMap[name] = newIndex;
    return newIndex;
}

std::optional<size_t> SymbolTable::addArray(const std::string& name, SymbolType type, int declarationLine, size_t size) {
    if (getKeywordType(name).has_value()) {
        errorHandler.logSemanticError("Identifier '" + name + "' is a reserved keyword.", declarationLine);
        return std::nullopt;
    }
    if (findSymbol(name).has_value()) {
        errorHandler.logSemanticError("Array '" + name + "' already declared.", declarationLine);
        return std::nullopt;
    }

    if (type != SymbolType::ARRAY_INT && type != SymbolType::ARRAY_FLOAT) {
        errorHandler.logSemanticError("Internal: Invalid type for array '" + name + "'.", declarationLine);
        return std::nullopt;
    }
    if (size == 0) {
        errorHandler.logSemanticError("Array '" + name + "' cannot have zero size.", declarationLine);
        return std::nullopt;
    }

    size_t newIndex = symbols.size();
    symbols.emplace_back(name, type, declarationLine, size); // ���������� ����������� SymbolInfo ��� ��������
    nameToIndexMap[name] = newIndex;
    return newIndex;
}

const SymbolInfo* SymbolTable::getSymbolInfo(size_t index) const {
    if (index >= symbols.size()) {
        // ��� ������ �� ������ ��������� ��� ���������� ������ �������/��������������
        // �� ������� ��� �������
        // errorHandler.logRuntimeError("Internal: Symbol index " + std::to_string(index) + " out of bounds.");
        return nullptr;
    }
    return &symbols[index];
}

SymbolInfo* SymbolTable::getSymbolInfo(size_t index) {
    if (index >= symbols.size()) {
        // errorHandler.logRuntimeError("Internal: Symbol index " + std::to_string(index) + " out of bounds.");
        return nullptr;
    }
    return &symbols[index];
}

SymbolType SymbolTable::getSymbolType(size_t index) const {
    const SymbolInfo* info = getSymbolInfo(index);
    if (info) {
        return info->type;
    }
    // ���������� ����� "����������" ��� ��� ������/������������� ������ ���������, ��� ������ �������
    // ��� ��������, ������������, ��� ���������� ��� �������� findSymbol
    // � ������, ����� �������� �� ������ �����������, ���� ������ ������� �� findSymbol ��� addVariable/addArray.
    return SymbolType::VARIABLE_INT; // ��������, ����� ���������� �� �������. ����� �� ��� ���� optional.
}

const std::string& SymbolTable::getSymbolName(size_t index) const {
    const SymbolInfo* info = getSymbolInfo(index);
    // ������������, ��� ������ �������, ��� � � getSymbolType
    // ���� info ����� nullptr, ��� �������� � ��������������� ���������.
    // ��� �������� ������� ��� ����� ���� ���������, ���� �� ����������� ������������ �������.
    static const std::string empty_string = "";
    if (info) return info->name;
    return empty_string;
}

int SymbolTable::getSymbolDeclarationLine(size_t index) const {
    const SymbolInfo* info = getSymbolInfo(index);
    if (info) return info->declarationLine;
    return 0;
}


bool SymbolTable::setVariableValue(size_t index, const StoredValue& valueToSet) {
    SymbolInfo* info = getSymbolInfo(index);
    if (!info) return false;

    if (info->type != SymbolType::VARIABLE_INT && info->type != SymbolType::VARIABLE_FLOAT) {
        errorHandler.logRuntimeError("Attempt to set value for non-variable symbol '" + info->name + "'.");
        return false;
    }

    // �������� � ��������� �������������� ����� ��� ������������
    if (info->type == SymbolType::VARIABLE_INT) {
        if (std::holds_alternative<int>(valueToSet)) {
            info->value = valueToSet;
        }
        else if (std::holds_alternative<float>(valueToSet)) {
            errorHandler.logRuntimeError("Warning: Implicit conversion from float to int for variable '" + info->name + "'. Value truncated.");
            info->value = static_cast<int>(std::get<float>(valueToSet)); // ��������
        }
        else {
            errorHandler.logRuntimeError("Invalid value type for int variable '" + info->name + "'.");
            return false;
        }
    }
    else if (info->type == SymbolType::VARIABLE_FLOAT) {
        if (std::holds_alternative<float>(valueToSet)) {
            info->value = valueToSet;
        }
        else if (std::holds_alternative<int>(valueToSet)) {
            info->value = static_cast<float>(std::get<int>(valueToSet)); // �������������� int � float
        }
        else {
            errorHandler.logRuntimeError("Invalid value type for float variable '" + info->name + "'.");
            return false;
        }
    }
    return true;
}

std::optional<StoredValue> SymbolTable::getVariableValue(size_t index) const {
    const SymbolInfo* info = getSymbolInfo(index);
    if (!info) return std::nullopt;

    if (info->type != SymbolType::VARIABLE_INT && info->type != SymbolType::VARIABLE_FLOAT) {
        // errorHandler.logRuntimeError("Attempt to get value from non-variable symbol '" + info->name + "'.");
        // ��� ������ ������ ������������� �� ����� �������� ��������, ��� �������.
        // ������������� ������ �������� ��� �������� ������.
        return std::nullopt;
    }
    if (std::holds_alternative<std::monostate>(info->value)) {
        errorHandler.logRuntimeError("Variable '" + info->name + "' used before initialization.");
        // � ����������� �� ��������� �����, ����� ���������� 0/0.0f ��� ��� ��������� ������.
        // ��� �������� ������ monostate, � ������������� �����.
    }
    return info->value;
}

std::optional<size_t> SymbolTable::getArrayDeclaredSize(size_t index) const {
    const SymbolInfo* info = getSymbolInfo(index);
    if (!info) return std::nullopt;

    if (info->type != SymbolType::ARRAY_INT && info->type != SymbolType::ARRAY_FLOAT) {
        return std::nullopt;
    }
    return info->arrayDeclaredSize;
}

bool SymbolTable::setArrayElementValue(size_t arrayIndex, size_t elementIndex, const StoredValue& valueToSet) {
    SymbolInfo* info = getSymbolInfo(arrayIndex);
    if (!info) return false;

    if (info->type != SymbolType::ARRAY_INT && info->type != SymbolType::ARRAY_FLOAT) {
        errorHandler.logRuntimeError("Attempt to set element for non-array symbol '" + info->name + "'.");
        return false;
    }
    if (elementIndex >= info->arrayDeclaredSize) {
        errorHandler.logRuntimeError("Array index " + std::to_string(elementIndex) +
            " out of bounds for array '" + info->name +
            "' (size: " + std::to_string(info->arrayDeclaredSize) + ").");
        return false;
    }

    if (info->type == SymbolType::ARRAY_INT) {
        if (std::holds_alternative<int>(valueToSet)) {
            info->arrayData[elementIndex] = valueToSet;
        }
        else if (std::holds_alternative<float>(valueToSet)) {
            errorHandler.logRuntimeError("Warning: Implicit conversion from float to int for array element '" +
                info->name + "[" + std::to_string(elementIndex) + "]'. Value truncated.");
            info->arrayData[elementIndex] = static_cast<int>(std::get<float>(valueToSet));
        }
        else {
            errorHandler.logRuntimeError("Invalid value type for int array element '" + info->name + "[" + std::to_string(elementIndex) + "]'.");
            return false;
        }
    }
    else if (info->type == SymbolType::ARRAY_FLOAT) {
        if (std::holds_alternative<float>(valueToSet)) {
            info->arrayData[elementIndex] = valueToSet;
        }
        else if (std::holds_alternative<int>(valueToSet)) {
            info->arrayData[elementIndex] = static_cast<float>(std::get<int>(valueToSet));
        }
        else {
            errorHandler.logRuntimeError("Invalid value type for float array element '" + info->name + "[" + std::to_string(elementIndex) + "]'.");
            return false;
        }
    }
    return true;
}

std::optional<StoredValue> SymbolTable::getArrayElementValue(size_t arrayIndex, size_t elementIndex) const {
    const SymbolInfo* info = getSymbolInfo(arrayIndex);
    if (!info) return std::nullopt;

    if (info->type != SymbolType::ARRAY_INT && info->type != SymbolType::ARRAY_FLOAT) {
        // errorHandler.logRuntimeError("Attempt to get element from non-array symbol '" + info->name + "'.");
        return std::nullopt;
    }
    if (elementIndex >= info->arrayDeclaredSize) {
        errorHandler.logRuntimeError("Array index " + std::to_string(elementIndex) +
            " out of bounds for array '" + info->name +
            "' (size: " + std::to_string(info->arrayDeclaredSize) + ").");
        return std::nullopt; // ����� ������� nullopt, ����� ������������� ��� ��� ����������
    }
    // �������� �� �������������������� ������� (���� ��� ����� ��� ��������� �����)
    // if (std::holds_alternative<std::monostate>(info->arrayData[elementIndex])) {
    //     errorHandler.logRuntimeError("Array element '" + info->name + "[" + std::to_string(elementIndex) + "]' used before initialization.");
    // }
    return info->arrayData[elementIndex];
}

size_t SymbolTable::getTableSize() const {
    return symbols.size();
}

/*
// ��� �������, ���� �����������
void SymbolTable::print() const {
    std::cout << "--- Symbol Table ---" << std::endl;
    if (symbols.empty()) {
        std::cout << "(empty)" << std::endl;
    }
    for (size_t i = 0; i < symbols.size(); ++i) {
        const auto& sym = symbols[i];
        std::cout << "[" << i << "] " << sym.name << " (Line: " << sym.declarationLine << ") - Type: ";
        switch (sym.type) {
            case SymbolType::VARIABLE_INT:   std::cout << "VARIABLE_INT"; break;
            case SymbolType::VARIABLE_FLOAT: std::cout << "VARIABLE_FLOAT"; break;
            case SymbolType::ARRAY_INT:      std::cout << "ARRAY_INT (Size: " << sym.arrayDeclaredSize << ")"; break;
            case SymbolType::ARRAY_FLOAT:    std::cout << "ARRAY_FLOAT (Size: " << sym.arrayDeclaredSize << ")"; break;
        }
        // ����� �������� ����� ��������, ���� ��� ����
        if (sym.type == SymbolType::VARIABLE_INT && std::holds_alternative<int>(sym.value)) {
            std::cout << ", Value: " << std::get<int>(sym.value);
        } else if (sym.type == SymbolType::VARIABLE_FLOAT && std::holds_alternative<float>(sym.value)) {
            std::cout << ", Value: " << std::get<float>(sym.value);
        }
        std::cout << std::endl;
    }
    std::cout << "--------------------" << std::endl;
}
*/