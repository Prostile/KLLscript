#include "SymbolTable.h"
#include <iostream> // Для возможных сообщений об ошибках

// Конструктор: инициализирует таблицу и служебные слова
SymbolTable::SymbolTable() {
    // Инициализация карты ключевых слов
    keywordMap["int"] = TokenType::T_KW_INT;
    keywordMap["arr"] = TokenType::T_KW_ARR;
    keywordMap["if"] = TokenType::T_KW_IF;
    keywordMap["else"] = TokenType::T_KW_ELSE;
    keywordMap["while"] = TokenType::T_KW_WHILE;
    keywordMap["begin"] = TokenType::T_KW_BEGIN;
    keywordMap["end"] = TokenType::T_KW_END;
    keywordMap["cin"] = TokenType::T_KW_CIN;
    keywordMap["cout"] = TokenType::T_KW_COUT;
    // Добавить float, bool при расширении
}

// Проверка, является ли имя ключевым словом
std::optional<TokenType> SymbolTable::getKeywordType(const std::string& name) const {
    auto it = keywordMap.find(name);
    if (it != keywordMap.end()) {
        return it->second; // Найдено ключевое слово
    }
    return std::nullopt; // Не ключевое слово
}

// Поиск символа по имени
std::optional<size_t> SymbolTable::findSymbol(const std::string& name) const {
    auto it = nameToIndexMap.find(name);
    if (it != nameToIndexMap.end()) {
        return it->second; // Возвращаем индекс найденного символа
    }
    return std::nullopt; // Символ не найден
}

// Добавление переменной
std::optional<size_t> SymbolTable::addVariable(const std::string& name, SymbolType type, int declarationLine) {
    if (findSymbol(name).has_value() || getKeywordType(name).has_value()) {
        // Ошибка: Переопределение символа или использование имени ключевого слова
        // TODO: Интегрировать с ErrorHandler
        // std::cerr << "Error: Symbol '" << name << "' already defined." << std::endl;
        return std::nullopt;
    }

    // Проверяем корректность типа для переменной
    if (type != SymbolType::VARIABLE_INT /* && type != SymbolType::VARIABLE_FLOAT && type != SymbolType::VARIABLE_BOOL */) {
        // TODO: Интегрировать с ErrorHandler
        // std::cerr << "Internal Error: Attempting to add variable '" << name << "' with non-variable type." << std::endl;
        return std::nullopt;
    }


    size_t newIndex = symbols.size();
    symbols.emplace_back(name, type, declarationLine); // Создаем новый SymbolInfo
    nameToIndexMap[name] = newIndex;                   // Добавляем в карту для быстрого поиска
    return newIndex;
}

// Добавление массива
std::optional<size_t> SymbolTable::addArray(const std::string& name, SymbolType type, int declarationLine) {
    if (findSymbol(name).has_value() || getKeywordType(name).has_value()) {
        // Ошибка: Переопределение символа или использование имени ключевого слова
        // TODO: Интегрировать с ErrorHandler
        // std::cerr << "Error: Symbol '" << name << "' already defined." << std::endl;
        return std::nullopt;
    }

    // Проверяем корректность типа для массива
    if (type != SymbolType::ARRAY_INT /* && type != SymbolType::ARRAY_FLOAT && type != SymbolType::ARRAY_BOOL */) {
        // TODO: Интегрировать с ErrorHandler
        // std::cerr << "Internal Error: Attempting to add array '" << name << "' with non-array type." << std::endl;
        return std::nullopt;
    }

    size_t newIndex = symbols.size();
    symbols.emplace_back(name, type, declarationLine); // Создаем новый SymbolInfo
    nameToIndexMap[name] = newIndex;                   // Добавляем в карту для быстрого поиска
    // Размер массива будет установлен позже через resizeArray
    return newIndex;
}

// Получение SymbolInfo по индексу
const SymbolInfo& SymbolTable::getSymbolInfo(size_t index) const {
    if (index >= symbols.size()) {
        // TODO: Обработка ошибки выхода за границы
        std::cerr << "Internal Error: Symbol index out of bounds." << std::endl;
        exit(EXIT_FAILURE);
    }
    return symbols[index];
}
SymbolInfo& SymbolTable::getSymbolInfo(size_t index) {
    if (index >= symbols.size()) {
        // TODO: Обработка ошибки выхода за границы
        std::cerr << "Internal Error: Symbol index out of bounds." << std::endl;
        exit(EXIT_FAILURE);
    }
    return symbols[index];
}


// Получение типа символа по индексу
SymbolType SymbolTable::getSymbolType(size_t index) const {
    return getSymbolInfo(index).type;
}

// Получение имени символа по индексу
const std::string& SymbolTable::getSymbolName(size_t index) const {
    return getSymbolInfo(index).name;
}


// Установка значения переменной
bool SymbolTable::setVariableValue(size_t index, const SymbolValue& val) {
    SymbolInfo& info = getSymbolInfo(index);
    // Проверка, что это действительно переменная, а не массив
    if (info.type != SymbolType::VARIABLE_INT /* && info.type != SymbolType::VARIABLE_FLOAT && ... */) {
        // TODO: Ошибка - попытка присвоить значение массиву как переменной
        return false;
    }
    // TODO: Можно добавить проверку совместимости типов val и info.type
    info.value = val;
    return true;
}

// Получение значения переменной
std::optional<SymbolValue> SymbolTable::getVariableValue(size_t index) const {
    const SymbolInfo& info = getSymbolInfo(index);
    if (info.type != SymbolType::VARIABLE_INT /* && info.type != SymbolType::VARIABLE_FLOAT && ... */) {
        // TODO: Ошибка - попытка получить значение массива как переменной
        return std::nullopt;
    }
    return info.value;
}

// Инициализация (выделение памяти) для массива
bool SymbolTable::resizeArray(size_t index, size_t size) {
    SymbolInfo& info = getSymbolInfo(index);
    // Проверка, что это действительно массив
    if (info.type != SymbolType::ARRAY_INT /* && info.type != SymbolType::ARRAY_FLOAT && ... */) {
        // TODO: Ошибка - попытка изменить размер переменной как массива
        return false;
    }
    if (info.arraySize > 0) {
        // TODO: Ошибка - повторное определение размера массива? Или разрешено?
    }
    info.arraySize = size;
    // Выделяем память и инициализируем нулями (или значениями по умолчанию для типа)
    // Для int это будет вектор нулей.
    info.arrayData.resize(size, SymbolValue(0));
    return true;
}

// Получение размера массива
std::optional<size_t> SymbolTable::getArraySize(size_t index) const {
    const SymbolInfo& info = getSymbolInfo(index);
    if (info.type != SymbolType::ARRAY_INT /* && info.type != SymbolType::ARRAY_FLOAT && ... */) {
        return std::nullopt;
    }
    return info.arraySize;
}


// Установка значения элемента массива
bool SymbolTable::setArrayElementValue(size_t arrayIndex, size_t elementIndex, const SymbolValue& val) {
    SymbolInfo& info = getSymbolInfo(arrayIndex);
    if (info.type != SymbolType::ARRAY_INT /* && info.type != SymbolType::ARRAY_FLOAT && ... */) {
        // TODO: Ошибка - не массив
        return false;
    }
    if (elementIndex >= info.arraySize) {
        // TODO: Ошибка времени выполнения - выход за границы массива
        std::cerr << "Runtime Error: Array index out of bounds for '" << info.name
            << "' (index: " << elementIndex << ", size: " << info.arraySize << ")." << std::endl;
        // exit(EXIT_FAILURE); // Или вернуть false
        return false;
    }
    // TODO: Проверка совместимости типов val и типа элементов массива
    info.arrayData[elementIndex] = val;
    return true;
}

// Получение значения элемента массива
std::optional<SymbolValue> SymbolTable::getArrayElementValue(size_t arrayIndex, size_t elementIndex) const {
    const SymbolInfo& info = getSymbolInfo(arrayIndex);
    if (info.type != SymbolType::ARRAY_INT /* && info.type != SymbolType::ARRAY_FLOAT && ... */) {
        // TODO: Ошибка - не массив
        return std::nullopt;
    }
    if (elementIndex >= info.arraySize) {
        // TODO: Ошибка времени выполнения - выход за границы массива
        std::cerr << "Runtime Error: Array index out of bounds for '" << info.name
            << "' (index: " << elementIndex << ", size: " << info.arraySize << ")." << std::endl;
        // exit(EXIT_FAILURE); // Или вернуть nullopt
        return std::nullopt;
    }
    return info.arrayData[elementIndex];
}

// Получение текущего количества символов в таблице
size_t SymbolTable::getTableSize() const {
    return symbols.size();
}