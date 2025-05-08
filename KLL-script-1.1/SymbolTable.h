#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <type_traits> // Для type checks

#include "definitions.h"
#include "DataStructures.h"

class SymbolTable {
private:
    std::vector<SymbolInfo> symbols;
    std::unordered_map<std::string, size_t> nameToIndexMap;
    std::unordered_map<std::string, TokenType> keywordMap;

public:
    SymbolTable();

    // --- Работа с ключевыми словами ---
    std::optional<TokenType> getKeywordType(const std::string& name) const;

    // --- Поиск и добавление символов ---
    std::optional<size_t> findSymbol(const std::string& name) const;
    // Добавление символа (обобщенное)
    std::optional<size_t> addSymbol(const std::string& name, SymbolType type, int declarationLine);
    // // Старые методы (можно оставить для обратной совместимости или удалить)
    // std::optional<size_t> addVariable(const std::string& name, SymbolType type, int declarationLine);
    // std::optional<size_t> addArray(const std::string& name, SymbolType type, int declarationLine);


    // --- Доступ к информации о символах ---
    const SymbolInfo& getSymbolInfo(size_t index) const;
    SymbolInfo& getSymbolInfo(size_t index);
    SymbolType getSymbolType(size_t index) const;
    const std::string& getSymbolName(size_t index) const;
    // Проверка, является ли символ массивом
    bool isArray(size_t index) const;
    // Проверка, является ли символ переменной (не массивом)
    bool isVariable(size_t index) const;


    // --- Работа со значениями переменных ---
    bool setVariableValue(size_t index, const SymbolValue& value);
    std::optional<SymbolValue> getVariableValue(size_t index) const;

    // --- Работа с массивами ---
    bool resizeArray(size_t index, size_t size);
    std::optional<size_t> getArraySize(size_t index) const;
    bool setArrayElementValue(size_t arrayIndex, size_t elementIndex, const SymbolValue& value);
    std::optional<SymbolValue> getArrayElementValue(size_t arrayIndex, size_t elementIndex) const;

    // --- Проверка типов ---
    // Проверяет, можно ли присвоить значение типа valueType символу с типом targetType
    bool checkAssignmentTypeCompatibility(SymbolType targetType, const SymbolValue& value) const;
    bool checkAssignmentTypeCompatibility(SymbolType targetType, SymbolType valueType) const;

    // --- Вспомогательное ---
    size_t getTableSize() const;

private:
    // Вспомогательная функция для проверки, является ли тип типом переменной
    static bool isVariableType(SymbolType type);
    // Вспомогательная функция для проверки, является ли тип типом массива
    static bool isArrayType(SymbolType type);
    // Вспомогательная функция для получения типа элемента массива
    static SymbolType getArrayElementType(SymbolType arrayType);
};

#endif // SYMBOL_TABLE_H