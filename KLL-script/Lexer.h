#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <optional>

#include "definitions.h"
#include "DataStructures.h" // Нужно определение Token
// #include "ErrorHandler.h" // Будет нужно позже
#include "SymbolTable.h"  // Для проверки ключевых слов

// --- Класс Лексического Анализатора ---
class Lexer {
private:
    const std::string& sourceCode; // Ссылка на исходный код
    size_t currentPos;             // Текущая позиция в sourceCode
    int currentLine;               // Текущий номер строки
    int lineStartPos;              // Позиция начала текущей строки (для вычисления столбца)

    SymbolTable& symbolTable;      // Ссылка на таблицу символов (для ключевых слов)
    // ErrorHandler& errorHandler; // Ссылка на обработчик ошибок

    // Таблица переходов КА (State, InputCategory -> SemanticAction)
    // Размерности: [Количество состояний][Количество категорий символов]
    // Значения: Номер семантической программы
    static const int stateTransitionTable[3][20];

    // Таблица преобразования ASCII кодов в категории символов
    static const int asciiCategoryTable[128];

    // Внутренние методы - семантические программы
    Token processAction(int semanticAction, char currentChar);
    Token handleIdentifierOrKeyword(std::string& lexeme);
    Token handleNumber(int& value, char digit);
    Token createToken(TokenType type); // Создает токен с текущей позицией
    Token createToken(TokenType type, const std::string& text);
    Token createToken(TokenType type, int value);
    Token createErrorToken(const std::string& message);

    // Вспомогательные методы
    char peekNextChar() const; // Посмотреть следующий символ, не сдвигая позицию
    char consumeChar();        // Прочитать текущий символ и сдвинуть позицию
    int getCurrentColumn() const; // Вычислить текущий столбец

public:
    // Конструктор
    Lexer(const std::string& source, SymbolTable& symTable /*, ErrorHandler& errHandler*/);

    // Основной метод: получить следующий токен из потока
    Token getNextToken();
};

#endif // LEXER_H