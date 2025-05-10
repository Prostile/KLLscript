// symbol_table.h
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <vector>
#include <string>
#include <optional>
#include <variant>
#include <unordered_map>
#include <utility> // ��� std::move

#include "definitions.h" // ����� SymbolType, TokenType
#include "error_handler.h" // ��� ��������� �� �������

// ��������, �������� ��� ������� (���������� ��� �������� �������)
// ���������� std::monostate ��� �������������������� �������� ��� ����� �������� ��� �� ���������
using StoredValue = std::variant<std::monostate, int, float>;

// ���������� � ������� � �������
struct SymbolInfo {
    std::string name;
    SymbolType type;
    int declarationLine; // ������, ��� ������ ��� ��������

    // ��� ����������
    StoredValue value;

    // ��� ��������
    std::vector<StoredValue> arrayData;
    size_t arrayDeclaredSize; // ������, ��������� ��� ���������� (�����������)

    // ����������� ��� ����������
    SymbolInfo(std::string n, SymbolType t, int line)
        : name(std::move(n)), type(t), declarationLine(line),
        value(std::monostate{}), arrayDeclaredSize(0) {
    }

    // ����������� ��� �������� (������ �������� ��������)
    SymbolInfo(std::string n, SymbolType t, int line, size_t declaredSize)
        : name(std::move(n)), type(t), declarationLine(line),
        value(std::monostate{}), arrayDeclaredSize(declaredSize) {
        // ������������� ��������� ������� ���������� �� ���������
        if (type == SymbolType::ARRAY_INT) {
            arrayData.resize(declaredSize, StoredValue(0)); // ������� int ���������������� ������
        }
        else if (type == SymbolType::ARRAY_FLOAT) {
            arrayData.resize(declaredSize, StoredValue(0.0f)); // ������� float ���������������� 0.0f
        }
    }
};

// ����� ������� ��������
class SymbolTable {
private:
    std::vector<SymbolInfo> symbols;
    std::unordered_map<std::string, size_t> nameToIndexMap;
    std::unordered_map<std::string, TokenType> keywordMap;
    ErrorHandler& errorHandler; // ������ �� ���������� ������

public:
    SymbolTable(ErrorHandler& errHandler);

    // --- ������ � ��������� ������� ---
    std::optional<TokenType> getKeywordType(const std::string& name) const;

    // --- ����� � ���������� �������� ---
    std::optional<size_t> findSymbol(const std::string& name) const;

    // ���������� ���������� (int ��� float)
    // ���������� ������ ��� std::nullopt ��� ������
    std::optional<size_t> addVariable(const std::string& name, SymbolType type, int declarationLine);

    // ���������� ������� (int ��� float)
    // size - ������ �������, ��������� ��� ����������
    // ���������� ������ ��� std::nullopt ��� ������
    std::optional<size_t> addArray(const std::string& name, SymbolType type, int declarationLine, size_t size);

    // --- ������ � ���������� � �������� ---
    const SymbolInfo* getSymbolInfo(size_t index) const; // ���������� ���������, ����� ����� ���� ������� nullptr
    SymbolInfo* getSymbolInfo(size_t index);             // ������������� ������

    SymbolType getSymbolType(size_t index) const;
    const std::string& getSymbolName(size_t index) const;
    int getSymbolDeclarationLine(size_t index) const;

    // --- ������ �� ���������� ���������� ---
    bool setVariableValue(size_t index, const StoredValue& valueToSet);
    std::optional<StoredValue> getVariableValue(size_t index) const;

    // --- ������ � ��������� ---
    // ������ ������� ������ ��������������� ��� ���������� (addArray)
    std::optional<size_t> getArrayDeclaredSize(size_t index) const;
    bool setArrayElementValue(size_t arrayIndex, size_t elementIndex, const StoredValue& valueToSet);
    std::optional<StoredValue> getArrayElementValue(size_t arrayIndex, size_t elementIndex) const;

    // --- ��������������� ---
    size_t getTableSize() const;
    // void print() const; // ��� �������
};

#endif // SYMBOL_TABLE_H