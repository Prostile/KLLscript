#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <vector>
#include <string>
#include <optional>     // ��� �������� ������������ ��������/��������
#include <unordered_map> // ��� �������� ������ �� �����

#include "definitions.h"
#include "DataStructures.h" // ����� ����������� SymbolInfo, SymbolValue � �.�.

// --- ����� ������� �������� ---
// ������������� ���������� ����������� � ���������
class SymbolTable {
private:
    // �������� ��������� ���������� � ��������
    std::vector<SymbolInfo> symbols;

    // ����� ��� �������� ������ ������� ������� �� ��� �����
    std::unordered_map<std::string, size_t> nameToIndexMap;

    // ������� ��������� ���� (��� ������� ��������)
    std::unordered_map<std::string, TokenType> keywordMap;

    // ������� ��� ��������� ���������� ��������� ���� (���� �����������)
    // int tempCounter = 0;

public:
    // �����������: �������������� ������� � ��������� �����
    SymbolTable();

    // --- ������ � ��������� ������� ---

    // ��������, �������� �� ��� �������� ������
    std::optional<TokenType> getKeywordType(const std::string& name) const;

    // --- ����� � ���������� �������� ---

    // ����� ������� �� �����. ���������� ������ ��� std::nullopt, ���� �� ������.
    std::optional<size_t> findSymbol(const std::string& name) const;

    // ���������� ����������. ���������� ������ ������������ ������� ��� std::nullopt ��� ������ (��������, ���������������).
    std::optional<size_t> addVariable(const std::string& name, SymbolType type, int declarationLine);

    // ���������� �������. ���������� ������ ������������ ������� ��� std::nullopt ��� ������.
    // ������ ���� �� �������������, ��� ����� ������ ������������� ��� ���������� mem1/mem2.
    std::optional<size_t> addArray(const std::string& name, SymbolType type, int declarationLine);

    // --- ������ � ���������� � �������� ---

    // ��������� SymbolInfo �� ������� (��� ����������� ������������� ��� ������)
    // �������� �������� ������ �� �������!
    const SymbolInfo& getSymbolInfo(size_t index) const;
    SymbolInfo& getSymbolInfo(size_t index); // ������������� ������

    // ��������� ���� ������� �� �������
    SymbolType getSymbolType(size_t index) const;

    // ��������� ����� ������� �� �������
    const std::string& getSymbolName(size_t index) const;

    // --- ������ �� ���������� ���������� ---

    // ��������� �������� ����������
    bool setVariableValue(size_t index, const SymbolValue& value);

    // ��������� �������� ����������
    std::optional<SymbolValue> getVariableValue(size_t index) const;

    // --- ������ � ��������� ---

    // ������������� (��������� ������) ��� �������
    bool resizeArray(size_t index, size_t size);

    // ��������� ������� �������
    std::optional<size_t> getArraySize(size_t index) const;

    // ��������� �������� �������� �������
    bool setArrayElementValue(size_t arrayIndex, size_t elementIndex, const SymbolValue& value);

    // ��������� �������� �������� �������
    std::optional<SymbolValue> getArrayElementValue(size_t arrayIndex, size_t elementIndex) const;

    // --- ��������������� ---
    size_t getTableSize() const;
};

#endif // SYMBOL_TABLE_H