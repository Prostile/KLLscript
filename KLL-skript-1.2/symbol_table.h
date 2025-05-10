// symbol_table.h
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <vector>
#include <string>
#include <optional>
#include <variant>
#include <unordered_map>
#include <utility> // Для std::move

#include "definitions.h" // Нужны SymbolType, TokenType
#include "error_handler.h" // Для сообщений об ошибках

// Значение, хранимое для символа (переменной или элемента массива)
// Используем std::monostate для неинициализированных значений или когда значение еще не присвоено
using StoredValue = std::variant<std::monostate, int, float>;

// Информация о символе в таблице
struct SymbolInfo {
    std::string name;
    SymbolType type;
    int declarationLine; // Строка, где символ был объявлен

    // Для переменных
    StoredValue value;

    // Для массивов
    std::vector<StoredValue> arrayData;
    size_t arrayDeclaredSize; // Размер, указанный при объявлении (статический)

    // Конструктор для переменных
    SymbolInfo(std::string n, SymbolType t, int line)
        : name(std::move(n)), type(t), declarationLine(line),
        value(std::monostate{}), arrayDeclaredSize(0) {
    }

    // Конструктор для массивов (размер задается отдельно)
    SymbolInfo(std::string n, SymbolType t, int line, size_t declaredSize)
        : name(std::move(n)), type(t), declarationLine(line),
        value(std::monostate{}), arrayDeclaredSize(declaredSize) {
        // Инициализация элементов массива значениями по умолчанию
        if (type == SymbolType::ARRAY_INT) {
            arrayData.resize(declaredSize, StoredValue(0)); // Массивы int инициализируются нулями
        }
        else if (type == SymbolType::ARRAY_FLOAT) {
            arrayData.resize(declaredSize, StoredValue(0.0f)); // Массивы float инициализируются 0.0f
        }
    }
};

// Класс Таблицы Символов
class SymbolTable {
private:
    std::vector<SymbolInfo> symbols;
    std::unordered_map<std::string, size_t> nameToIndexMap;
    std::unordered_map<std::string, TokenType> keywordMap;
    ErrorHandler& errorHandler; // Ссылка на обработчик ошибок

public:
    SymbolTable(ErrorHandler& errHandler);

    // --- Работа с ключевыми словами ---
    std::optional<TokenType> getKeywordType(const std::string& name) const;

    // --- Поиск и добавление символов ---
    std::optional<size_t> findSymbol(const std::string& name) const;

    // Добавление переменной (int или float)
    // Возвращает индекс или std::nullopt при ошибке
    std::optional<size_t> addVariable(const std::string& name, SymbolType type, int declarationLine);

    // Добавление массива (int или float)
    // size - размер массива, указанный при объявлении
    // Возвращает индекс или std::nullopt при ошибке
    std::optional<size_t> addArray(const std::string& name, SymbolType type, int declarationLine, size_t size);

    // --- Доступ к информации о символах ---
    const SymbolInfo* getSymbolInfo(size_t index) const; // Возвращает указатель, чтобы можно было вернуть nullptr
    SymbolInfo* getSymbolInfo(size_t index);             // Неконстантная версия

    SymbolType getSymbolType(size_t index) const;
    const std::string& getSymbolName(size_t index) const;
    int getSymbolDeclarationLine(size_t index) const;

    // --- Работа со значениями переменных ---
    bool setVariableValue(size_t index, const StoredValue& valueToSet);
    std::optional<StoredValue> getVariableValue(size_t index) const;

    // --- Работа с массивами ---
    // Размер массива теперь устанавливается при добавлении (addArray)
    std::optional<size_t> getArrayDeclaredSize(size_t index) const;
    bool setArrayElementValue(size_t arrayIndex, size_t elementIndex, const StoredValue& valueToSet);
    std::optional<StoredValue> getArrayElementValue(size_t arrayIndex, size_t elementIndex) const;

    // --- Вспомогательное ---
    size_t getTableSize() const;
    // void print() const; // Для отладки
};

#endif // SYMBOL_TABLE_H