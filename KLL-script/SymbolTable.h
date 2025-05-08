#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <vector>
#include <string>
#include <optional>     // Для возврата опциональных значений/индексов
#include <unordered_map> // Для быстрого поиска по имени

#include "definitions.h"
#include "DataStructures.h" // Нужны определения SymbolInfo, SymbolValue и т.д.

// --- Класс Таблицы Символов ---
// Инкапсулирует управление переменными и массивами
class SymbolTable {
private:
    // Основное хранилище информации о символах
    std::vector<SymbolInfo> symbols;

    // Карта для быстрого поиска индекса символа по его имени
    std::unordered_map<std::string, size_t> nameToIndexMap;

    // Таблица служебных слов (для быстрой проверки)
    std::unordered_map<std::string, TokenType> keywordMap;

    // Счетчик для генерации уникальных временных имен (если понадобится)
    // int tempCounter = 0;

public:
    // Конструктор: инициализирует таблицу и служебные слова
    SymbolTable();

    // --- Работа с ключевыми словами ---

    // Проверка, является ли имя ключевым словом
    std::optional<TokenType> getKeywordType(const std::string& name) const;

    // --- Поиск и добавление символов ---

    // Поиск символа по имени. Возвращает индекс или std::nullopt, если не найден.
    std::optional<size_t> findSymbol(const std::string& name) const;

    // Добавление переменной. Возвращает индекс добавленного символа или std::nullopt при ошибке (например, переопределение).
    std::optional<size_t> addVariable(const std::string& name, SymbolType type, int declarationLine);

    // Добавление массива. Возвращает индекс добавленного символа или std::nullopt при ошибке.
    // Размер пока не устанавливаем, это может делать интерпретатор при выполнении mem1/mem2.
    std::optional<size_t> addArray(const std::string& name, SymbolType type, int declarationLine);

    // --- Доступ к информации о символах ---

    // Получение SymbolInfo по индексу (для внутреннего использования или отладк)
    // Добавить проверку выхода за границы!
    const SymbolInfo& getSymbolInfo(size_t index) const;
    SymbolInfo& getSymbolInfo(size_t index); // Неконстантная версия

    // Получение типа символа по индексу
    SymbolType getSymbolType(size_t index) const;

    // Получение имени символа по индексу
    const std::string& getSymbolName(size_t index) const;

    // --- Работа со значениями переменных ---

    // Установка значения переменной
    bool setVariableValue(size_t index, const SymbolValue& value);

    // Получение значения переменной
    std::optional<SymbolValue> getVariableValue(size_t index) const;

    // --- Работа с массивами ---

    // Инициализация (выделение памяти) для массива
    bool resizeArray(size_t index, size_t size);

    // Получение размера массива
    std::optional<size_t> getArraySize(size_t index) const;

    // Установка значения элемента массива
    bool setArrayElementValue(size_t arrayIndex, size_t elementIndex, const SymbolValue& value);

    // Получение значения элемента массива
    std::optional<SymbolValue> getArrayElementValue(size_t arrayIndex, size_t elementIndex) const;

    // --- Вспомогательное ---
    size_t getTableSize() const;
};

#endif // SYMBOL_TABLE_H