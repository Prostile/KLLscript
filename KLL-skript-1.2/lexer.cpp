// lexer.cpp
#include "lexer.h"
#include <cctype>   // Для isalpha, isdigit, isprint
#include <sstream>  // Для форматирования сообщений об ошибках

// --- Инициализация статических таблиц ---

// Состояния:
// 0: START
// 1: IDENTIFIER
// 2: NUMBER_INT (целая часть числа)
// 3: NUMBER_FLOAT_DECIMAL (после точки, ожидаем цифры)
// 4: NUMBER_FLOAT_EXP (после 'e' или 'E', ожидаем знак или цифры - не реализуем для упрощения по заданию)

// Категории символов (индексы для stateTransitionTable):
// 0: буква (a-z, A-Z)
// 1: цифра (0-9)
// 2: +
// 3: -
// 4: =
// 5: *
// 6: /
// 7: пробел, \t, \r
// 8: (
// 9: )
// 10: [
// 11: ]
// 12: ~ (для ==)
// 13: >
// 14: <
// 15: ! (для !=)
// 16: ;
// 17: \n (перевод строки)
// 18: другие печатные символы (не вошедшие в другие категории, например, запятая)
// 19: $ (EOF)
// 20: . (точка для float)

const int Lexer::stateTransitionTable[5][21] = {
    // Категория:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20
    // Символ:    бук циф +   -   =   *   /  проб (   )   [   ]   ~   >   <   !   ;  \n  др EOF  .
    /* 0 START */ { 1,  2, 30, 31, 32, 33, 34,  0, 35, 36, 37, 38, 39, 40, 41, 42, 43,  0, 45, 46, 47}, // Действия 30-43,46 - создание простых токенов; 0-игнор; 45-ошибка; 47-начало float с точки
    /* 1 IDENT */ { 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11}, // 10-продолжить идентификатор; 11-завершить идентификатор
    /* 2 NUM_INT */ { 21, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,  3}, // 20-продолжить целое; 21-завершить целое; 3-переход в состояние float_decimal
    /* 3 NUM_FLT_D*/{ 23, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23}, // 22-продолжить дробную часть; 23-завершить float
    /* 4 NUM_FLT_E */{99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99}  // Состояние для экспоненты (пока не используется, 99 - ошибка/недостижимо)
};
// Действия (семантические программы):
// 0: Игнорировать символ, остаться в START
// 1: Начало идентификатора, переход в IDENTIFIER, добавить символ
// 2: Начало целого числа, переход в NUMBER_INT, добавить цифру
// 3: Встретили точку после цифр, переход в NUMBER_FLOAT_DECIMAL, добавить точку
// 10: Продолжить идентификатор, остаться в IDENTIFIER, добавить символ
// 11: Завершить идентификатор/ключевое слово, вернуть токен, символ НЕ потреблять
// 20: Продолжить целое число, остаться в NUMBER_INT, добавить цифру
// 21: Завершить целое число, вернуть токен, символ НЕ потреблять
// 22: Продолжить дробную часть float, остаться в NUMBER_FLOAT_DECIMAL, добавить цифру
// 23: Завершить float, вернуть токен, символ НЕ потреблять
// 30-43: Распознан односимвольный токен или простой разделитель, вернуть токен, символ ПОТРЕБИТЬ
// 45: Ошибка - неизвестный символ
// 46: EOF - вернуть T_EOF
// 47: Начало float с точки (например, ".5"), переход в NUMBER_FLOAT_DECIMAL, добавить точку (и "0" перед ней)


const int Lexer::asciiCategoryTable[128] = {
    // Управляющие символы (0-31), большинство - "другие" или \n, \t, \r
    18, 18, 18, 18, 18, 18, 18, 18, 18,  7, 17, 18, 18,  7, 18, 18, // 0-15   (\t=7, \n=17, \r=7)
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, // 16-31
    // Печатные символы
     7, 15, 18, 18, 19, 18, 18, 18,  8,  9,  5,  2, 18,  3, 20,  6, // 32-47  (' '=7, '!'=15, '$'=19 (EOF), '('=8, ')'=9, '*'=5, '+'=2, ','=18, '-'=3, '.'=20, '/'=6)
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 18, 16, 14,  4, 13, 18, // 48-63  ('0'-'9'=1, ':'=18, ';'=16, '<'=14, '='=4, '>'=13, '?'=18)
    18,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 64-79  ('@'=18, 'A'-'O'=0)
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 18, 11, 18, 18, // 80-95  ('P'-'Z'=0, '['=10, '\'=18, ']'=11, '^'=18, '_'=18)
    18,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 96-111 ('`'=18, 'a'-'o'=0)
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 18, 18, 18, 12, 18  // 112-127('p'-'z'=0, '{'=18, '|'=18, '}'=18, '~'=12, DEL=18)
};


Lexer::Lexer(const std::string& source, SymbolTable& symTable, ErrorHandler& errHandler)
    : sourceCode(source), currentPos(0), currentLine(1), lineStartPos(0),
    symbolTable(symTable), errorHandler(errHandler) {
}

char Lexer::peekChar(size_t offset) const {
    if (currentPos + offset < sourceCode.length()) {
        return sourceCode[currentPos + offset];
    }
    return '$'; // Специальный символ конца файла, если вышли за пределы
}

char Lexer::consumeChar() {
    if (currentPos >= sourceCode.length()) {
        return '$'; // Уже в конце
    }
    char consumed = sourceCode[currentPos];
    currentPos++;

    if (consumed == '\n') {
        currentLine++;
        lineStartPos = currentPos;
    }
    return consumed;
}

int Lexer::getCurrentColumn() const {
    return static_cast<int>(currentPos - lineStartPos) + 1;
}

Token Lexer::createFinalToken(TokenType type, const std::string& text) {
    // Для односимвольных токенов и EOF, текст может быть пустым или формироваться из типа
    std::string tokenText = text;
    if (text.empty()) {
        if (type >= TokenType::T_ASSIGN && type <= TokenType::T_SEMICOLON) { // Односимвольные операторы
            tokenText = std::string(1, static_cast<char>(type));
        }
        else if (type == TokenType::T_EOF) {
            tokenText = "EOF";
        }
        // для ключевых слов текст будет передан явно при создании токена
    }
    return Token(type, tokenText, currentLine, getCurrentColumn() - static_cast<int>(tokenText.length()));
}

Token Lexer::createIntToken(int value, const std::string& text) {
    return Token(TokenType::T_NUMBER_INT, value, currentLine, getCurrentColumn() - static_cast<int>(text.length()), text);
}

Token Lexer::createFloatToken(float value, const std::string& text) {
    return Token(TokenType::T_NUMBER_FLOAT, value, currentLine, getCurrentColumn() - static_cast<int>(text.length()), text);
}


Token Lexer::getNextToken() {
    LexerState currentState = LexerState::START; // Начальное состояние КА (используем 0 для таблицы)
    std::string currentLexemeText = "";
    // int currentNumberIntValue = 0; // Не нужно, парсим из currentLexemeText в конце
    // float currentNumberFloatValue = 0.0f;

    while (true) { // Цикл конечного автомата
        char currentChar = peekChar();
        int charCategory = 18; // 'other_printable' по умолчанию

        if (currentChar == '$' && currentPos >= sourceCode.length()) { // Явная проверка конца строки
            charCategory = 19; // '$' (EOF)
        }
        else if (currentChar >= 0 && static_cast<unsigned char>(currentChar) < 128) {
            charCategory = asciiCategoryTable[static_cast<unsigned char>(currentChar)];
        }
        else {
            // Символ вне диапазона ASCII 0-127 (например, UTF-8). Считаем его "другим"
            // или можно выдать ошибку, если язык поддерживает только ASCII.
            // Для простоты, пока считаем "другим".
            charCategory = 18;
        }

        int semanticAction = stateTransitionTable[static_cast<int>(currentState)][charCategory];

        switch (semanticAction) {
        case 0: // Игнорировать символ (пробел, \t, \r), остаться в START
            consumeChar();
            currentState = LexerState::START;
            currentLexemeText = ""; // Сброс лексемы
            break;
        case 1: // Начало идентификатора
            currentLexemeText += consumeChar();
            currentState = LexerState::IDENTIFIER; // Переход в состояние 1 (IDENTIFIER)
            break;
        case 2: // Начало целого числа
            currentLexemeText += consumeChar();
            currentState = LexerState::NUMBER_INT; // Переход в состояние 2 (NUMBER_INT)
            break;
        case 3: // Точка после цифр (переход в NUMBER_FLOAT_DECIMAL)
            currentLexemeText += consumeChar(); // '.'
            currentState = static_cast<LexerState>(3); // NUMBER_FLOAT_DECIMAL
            break;
        case 10: // Продолжить идентификатор
            currentLexemeText += consumeChar();
            // currentState остается LexerState::IDENTIFIER
            break;
        case 11: // Завершить идентификатор/ключевое слово
            // Символ НЕ потребляем, он начнет следующий токен
            if (auto kwType = symbolTable.getKeywordType(currentLexemeText); kwType.has_value()) {
                return createFinalToken(kwType.value(), currentLexemeText);
            }
            else {
                return createFinalToken(TokenType::T_IDENTIFIER, currentLexemeText);
            }
            // break; // не нужен, т.к. return
        case 20: // Продолжить целое число
            currentLexemeText += consumeChar();
            // currentState остается LexerState::NUMBER_INT
            break;
        case 21: // Завершить целое число
            // Символ НЕ потребляем
            try {
                return createIntToken(std::stoi(currentLexemeText), currentLexemeText);
            }
            catch (const std::out_of_range& oor) {
                errorHandler.logLexicalError("Integer literal '" + currentLexemeText + "' is out of range.", currentLine, getCurrentColumn() - static_cast<int>(currentLexemeText.length()));
                return Token(TokenType::T_ERROR, currentLine, getCurrentColumn() - static_cast<int>(currentLexemeText.length()));
            }
            // break; // не нужен
        case 22: // Продолжить дробную часть float
            currentLexemeText += consumeChar();
            currentState = static_cast<LexerState>(3); // NUMBER_FLOAT_DECIMAL
            break;
        case 23: // Завершить float
            // Символ НЕ потребляем
            // Проверка, если после точки не было цифр (e.g. "12.")
            if (currentLexemeText.back() == '.') {
                errorHandler.logLexicalError("Malformed float literal '" + currentLexemeText + "'. Digit expected after decimal point.", currentLine, getCurrentColumn() - static_cast<int>(currentLexemeText.length()));
                return Token(TokenType::T_ERROR, currentLine, getCurrentColumn() - static_cast<int>(currentLexemeText.length()));
            }
            try {
                return createFloatToken(std::stof(currentLexemeText), currentLexemeText);
            }
            catch (const std::out_of_range& oor) {
                errorHandler.logLexicalError("Float literal '" + currentLexemeText + "' is out of range.", currentLine, getCurrentColumn() - static_cast<int>(currentLexemeText.length()));
                return Token(TokenType::T_ERROR, currentLine, getCurrentColumn() - static_cast<int>(currentLexemeText.length()));
            }
            // break; // не нужен

        // Односимвольные токены (30-43)
        case 30: consumeChar(); return createFinalToken(TokenType::T_PLUS, "+");
        case 31: consumeChar(); return createFinalToken(TokenType::T_MINUS, "-");
        case 32: consumeChar(); return createFinalToken(TokenType::T_ASSIGN, "=");
        case 33: consumeChar(); return createFinalToken(TokenType::T_MULTIPLY, "*");
        case 34: consumeChar(); return createFinalToken(TokenType::T_DIVIDE, "/");
        case 35: consumeChar(); return createFinalToken(TokenType::T_LPAREN, "(");
        case 36: consumeChar(); return createFinalToken(TokenType::T_RPAREN, ")");
        case 37: consumeChar(); return createFinalToken(TokenType::T_LBRACKET, "[");
        case 38: consumeChar(); return createFinalToken(TokenType::T_RBRACKET, "]");
        case 39: consumeChar(); return createFinalToken(TokenType::T_EQUAL, "~");
        case 40: consumeChar(); return createFinalToken(TokenType::T_GREATER, ">");
        case 41: consumeChar(); return createFinalToken(TokenType::T_LESS, "<");
        case 42: consumeChar(); return createFinalToken(TokenType::T_NOT_EQUAL, "!");
        case 43: consumeChar(); return createFinalToken(TokenType::T_SEMICOLON, ";");
            // case 18 из START (категория \n) была обработана consumeChar() и currentState = LexerState::START

        case 45: // Ошибка - неизвестный символ в текущем состоянии
        {
            char errChar = consumeChar();
            std::string msg = "Unexpected character '";
            if (isprint(static_cast<unsigned char>(errChar))) msg += errChar;
            else msg += "\\" + std::to_string(static_cast<unsigned char>(errChar));
            msg += "'.";
            errorHandler.logLexicalError(msg, currentLine, getCurrentColumn() - 1);
            // Возвращаем токен ошибки, чтобы парсер мог его обработать или пропустить
            // Позиция в токене ошибки уже будет корректной.
            return Token(TokenType::T_ERROR, currentLine, getCurrentColumn() - 1); // -1 потому что символ уже съеден
        }
        case 46: // EOF
            return createFinalToken(TokenType::T_EOF);

        case 47: // Начало float с точки (например, ".5")
            currentLexemeText = "0"; // Добавляем "0" для корректного stof(".5") -> stof("0.5")
            currentLexemeText += consumeChar(); // Добавляем саму точку '.'
            currentState = static_cast<LexerState>(3); // Переход в NUMBER_FLOAT_DECIMAL
            // Если сразу после точки не цифра, это будет обработано в состоянии NUMBER_FLOAT_DECIMAL (действие 23)
            break;

        case 99: // Недостижимое состояние/ошибка в таблице
            errorHandler.logLexicalError("Internal Lexer Error: Unreachable state or invalid action 99.", currentLine, getCurrentColumn());
            consumeChar(); // Попытка пропустить символ
            return Token(TokenType::T_ERROR, currentLine, getCurrentColumn() - 1);


        default: // Неизвестное семантическое действие - внутренняя ошибка
            errorHandler.logLexicalError("Internal Lexer Error: Unknown semantic action " + std::to_string(semanticAction) + ".", currentLine, getCurrentColumn());
            consumeChar(); // Попытка пропустить символ, чтобы избежать бесконечного цикла
            return Token(TokenType::T_ERROR, currentLine, getCurrentColumn() - 1);
        }
    }
}