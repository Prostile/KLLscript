// lexer.h
#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

#include "definitions.h"    // TokenType, LexerState
#include "token.h"          // Структура Token
#include "symbol_table.h"   // Для проверки ключевых слов
#include "error_handler.h"  // Для логирования ошибок

class Lexer {
private:
    const std::string& sourceCode; // Ссылка на исходный код
    size_t currentPos;             // Текущая позиция в sourceCode
    int currentLine;               // Текущий номер строки
    int lineStartPos;              // Позиция начала текущей строки (для вычисления столбца)

    SymbolTable& symbolTable;      // Ссылка на таблицу символов
    ErrorHandler& errorHandler;    // Ссылка на обработчик ошибок

    // Таблицы для конечного автомата
    // [Состояние][Категория символа] -> Номер семантического действия
    // Состояния: 0-START, 1-IDENTIFIER, 2-NUMBER_INT, 3-NUMBER_FLOAT_DECIMAL, 4-NUMBER_FLOAT_EXP
    // Категории символов: см. комментарии к asciiCategoryTable
    static const int stateTransitionTable[5][21]; // Увеличено количество состояний для float

    // Таблица преобразования ASCII кодов в категории символов
    // Категории: 0:'a'-'z','A'-'Z', 1:'0'-'9', 2:'+', 3:'-', 4:'=', 5:'*', 6:'/',
    //            7:' ', 8:'(', 9:')', 10:'[', 11:']', 12:'~', 13:'>', 14:'<',
    //            15:'!', 16:';', 17:'\n', 18:other_printable, 19:'$', 20:'.' (точка для float)
    static const int asciiCategoryTable[128];

    // Вспомогательные методы
    char peekChar(size_t offset = 0) const; // Посмотреть символ со смещением, не сдвигая позицию
    char consumeChar();                     // Прочитать текущий символ и сдвинуть позицию
    int getCurrentColumn() const;           // Вычислить текущий столбец

    // Методы для создания токенов, используя errorHandler для сообщений
    Token createFinalToken(TokenType type, const std::string& text = "");
    Token createIntToken(int value, const std::string& text);
    Token createFloatToken(float value, const std::string& text);
    // createErrorToken теперь не нужен здесь, т.к. ошибки логируются через errorHandler

public:
    Lexer(const std::string& source, SymbolTable& symTable, ErrorHandler& errHandler);

    // Основная функция лексического анализатора (реализация КА)
    Token getNextToken();
};

#endif // LEXER_H