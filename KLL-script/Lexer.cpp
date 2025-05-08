#include "Lexer.h"
#include <iostream> // Для отладки или временных ошибок
#include <cstring> // Для strcmp, если понадобится (хотя лучше использовать std::string)
#include <cctype> // Для isprint
#include <sstream> // Для форматирования строки с кодом символа

// --- Инициализация статических таблиц ---

// Таблица семантических программ (матрица где строки это состояния, а столбцы это тип считываемого символа)
// Индексы состояний: 0 - START, 1 - IDENTIFIER, 2 - NUMBER
// Индексы категорий символов: см. asciiCategoryTable и комментарии к SemTable в оригинальном коде
const int Lexer::stateTransitionTable[3][20] = {
    //		     a-z 0-9  +   -   =   *   /  sp   (   )   [   ]   ~   >   <   !   ;  \n err   $
    //		     0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19
    /*START*/ {  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20},
    /*IDENT*/ { 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 24, 22}, // Было 21 для цифр, но идентификатор может содержать цифры
    /*NUMBER*/{ 24, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 19, 28}  // Нельзя букву после цифры (ошибка 24). Скобки/пробелы и т.д. завершают число (28)
};

// Таблица преобразования ASCII кодов в категории символов (соответствует AscTable из оригинала)
// 0:'a'-'z', 1:'0'-'9', 2:'+', 3:'-', 4:'=', 5:'*', 6:'/', 7:' ', 8:'(', 9:')', 10:'[', 11:']',
// 12:'~', 13:'>', 14:'<', 15:'!', 16:';', 17:'\n', 18:other, 19:'$' (EOF)
const int Lexer::asciiCategoryTable[128] = {
    18, 18, 18, 18, 18, 18, 18, 18, 18,  7, 17, 18, 18,  7, 18, 18, // 0-15
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, // 16-31
     7, 15, 18, 18, 19, 18, 18, 18,  8,  9,  5,  2, 18,  3, 18,  6, // 32-47 ' ' ! ... /
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 18, 16, 14,  4, 13, 18, // 48-63 '0'-'9' : ; < = > ?
    18,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 64-79 '@' 'A'-'O'
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 18, 11, 18, 18, // 80-95 'P'-'Z' [ \ ] ^ _
    18,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 96-111 '`' 'a'-'o'
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 18, 18, 18, 12, 18  // 112-127 'p'-'z' { | } ~ DEL
};

// --- Конструктор ---
Lexer::Lexer(const std::string& source, SymbolTable& symTable /*, ErrorHandler& errHandler*/)
    : sourceCode(source), currentPos(0), currentLine(1), lineStartPos(0),
    symbolTable(symTable) /*, errorHandler(errHandler)*/ {
}

// --- Вспомогательные методы ---
char Lexer::peekNextChar() const {
    if (currentPos < sourceCode.length()) {
        return sourceCode[currentPos];
    }
    return '$'; // Специальный символ конца файла (или '\0')
}

char Lexer::consumeChar() {
    if (currentPos >= sourceCode.length()) {
        return '$'; // Уже в конце
    }
    char consumed = sourceCode[currentPos];
    currentPos++;

    if (consumed == '\n') {
        currentLine++;
        lineStartPos = currentPos; // Запоминаем начало новой строки
    }
    return consumed;
}

int Lexer::getCurrentColumn() const {
    // +1, так как столбцы обычно нумеруются с 1
    return static_cast<int>(currentPos - lineStartPos) + 1;
}

// --- Создание токенов ---
Token Lexer::createToken(TokenType type) {
    return Token(type, currentLine, getCurrentColumn() - 1); // -1 т.к. символ уже считан
}
Token Lexer::createToken(TokenType type, const std::string& text) {
    return Token(type, text, currentLine, getCurrentColumn() - text.length());
}
Token Lexer::createToken(TokenType type, int value) {
    // Длина числа неизвестна, позиция будет примерной
    return Token(type, value, currentLine, getCurrentColumn());
}
Token Lexer::createErrorToken(const std::string& message) {
    // TODO: Использовать ErrorHandler
    char badChar = '?'; // Символ по умолчанию
    if (currentPos < sourceCode.length()) { // Убедимся, что не вышли за границу
        badChar = sourceCode[currentPos]; // Берем символ из текущей позиции (до consumeChar в case)
    }
    else if (currentPos > 0 && currentPos == sourceCode.length()) {
        badChar = sourceCode[currentPos - 1]; // Если ошибка на самом конце, покажем последний символ
    }


    std::stringstream ss;
    ss << message << " (char: '"
        << (isprint(static_cast<unsigned char>(badChar)) ? std::string(1, badChar) : "\\?") << "', "
        << "ASCII: " << static_cast<int>(static_cast<unsigned char>(badChar)) << ")";

    std::cerr << "Lexical Error (Line " << currentLine << ", Col " << getCurrentColumn() << "): "
        << ss.str() << std::endl;

    // Возвращаем токен ошибки, но позиция уже сдвинута в вызывающем коде (case 19, 24)
    return Token(TokenType::T_ERROR, currentLine, getCurrentColumn());
}

// --- Обработка семантических действий ---
Token Lexer::handleIdentifierOrKeyword(std::string& lexeme) {
    // Проверяем, не ключевое ли это слово
    if (auto kwType = symbolTable.getKeywordType(lexeme); kwType.has_value()) {
        return createToken(kwType.value(), lexeme);
    }
    else {
        // Это идентификатор
        return createToken(TokenType::T_IDENTIFIER, lexeme);
    }
}

Token Lexer::handleNumber(int& value, char digit) {
    value = value * 10 + (digit - '0');
    // Возвращаем nullptr, так как токен еще не готов
    // Токен будет создан, когда встретится не-цифра
    return Token(); // Возвращаем пустой токен для продолжения
}


// --- Основной метод: получить следующий токен ---
Token Lexer::getNextToken() {
    LexerState currentState = LexerState::START;
    std::string currentLexeme = "";
    int currentNumberValue = 0;
    char currentChar = 0;

    // --- Отладка-- -
    char firstChar = peekNextChar();
    std::cout << "[DEBUG Lexer] Starting token parse at Line " << currentLine
        << ", Col " << getCurrentColumn()
        << ", Char: '" << (isprint(static_cast<unsigned char>(firstChar)) ? std::string(1, firstChar) : "\\?")
        << "' (ASCII: " << static_cast<int>(static_cast<unsigned char>(firstChar)) << ")" << std::endl;
    // --- Конец Отладки ---*/

    while (currentState != LexerState::FINAL && currentState != LexerState::ERROR) {
        currentChar = peekNextChar(); // Смотрим следующий символ

        // Определяем категорию символа
        int category = 18; // 'other' по умолчанию
        if (currentChar >= 0 && currentChar < 128) {
            category = asciiCategoryTable[static_cast<unsigned char>(currentChar)];
            // Особая обработка конца файла
            if (currentPos >= sourceCode.length()) {
                category = 19; // '$' категория EOF
            }
        }
        else if (currentPos >= sourceCode.length()) {
            category = 19; // '$' категория EOF
        }


        // Получаем номер семантического действия из таблицы переходов
        int semanticAction = stateTransitionTable[static_cast<int>(currentState)][category];

        // Выполняем действие
        switch (semanticAction) {
        case 1: // Начало идентификатора
            currentState = LexerState::IDENTIFIER;
            currentLexeme += consumeChar();
            break;
        case 2: // Начало числа
            currentState = LexerState::NUMBER;
            currentNumberValue = (consumeChar() - '0');
            break;
        case 3: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_PLUS);
        case 4: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_MINUS);
        case 5: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_ASSIGN);
        case 6: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_MULTIPLY);
        case 7: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_DIVIDE);
        case 8: // Пробел - игнорируем и остаемся в START
            consumeChar();
            currentState = LexerState::START; // Остаемся в начальном состоянии
            break;
        case 9: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_LPAREN);
        case 10: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_RPAREN);
        case 11: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_LBRACKET);
        case 12: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_RBRACKET);
        case 13: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_EQUAL);
        case 14: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_GREATER);
        case 15: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_LESS);
        case 16: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_NOT_EQUAL);
        case 17: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_SEMICOLON);
        case 18: // Перевод строки - игнорируем и остаемся в START
            consumeChar(); // consumeChar() сам обработает currentLine++
            currentState = LexerState::START;
            break;
        case 19: // Ошибка (недопустимый символ в данном состоянии)
        {
            std::string base_msg = "Unexpected character";
            consumeChar(); // Потребляем ошибочный символ
            return createErrorToken(base_msg);
        }

        case 20: // Конец файла ($)
            currentState = LexerState::FINAL;
            // consumeChar(); // Не нужно, мы уже в конце
            return createToken(TokenType::T_EOF);

        case 21: // Продолжение идентификатора (буква или цифра)
            currentState = LexerState::IDENTIFIER; // Остаемся в состоянии идентификатора
            currentLexeme += consumeChar();
            break;

        case 22: // Конец идентификатора (встретили не букву/цифру)
            currentState = LexerState::FINAL;
            // Символ НЕ потребляем, он начнет следующий токен
            return handleIdentifierOrKeyword(currentLexeme);

        case 23: // Конец идентификатора и перевод строки
            currentState = LexerState::FINAL;
            // Символ НЕ потребляем, перевод строки обработается на след. итерации
            return handleIdentifierOrKeyword(currentLexeme);

        case 24: // Ошибка в идентификаторе или числе (например, буква после числа)
        {
            std::string base_msg = "Invalid character in identifier or number";
            consumeChar(); // Потребляем ошибочный символ
            return createErrorToken(base_msg);
        }

        case 27: // Продолжение числа
            currentState = LexerState::NUMBER; // Остаемся в состоянии числа
            handleNumber(currentNumberValue, consumeChar()); // Добавляем цифру
            break;

        case 28: // Конец числа (встретили не цифру)
            currentState = LexerState::FINAL;
            // Символ НЕ потребляем
            return createToken(TokenType::T_NUMBER_INT, currentNumberValue);

        case 29: // Конец числа и перевод строки
            currentState = LexerState::FINAL;
            // Символ НЕ потребляем
            return createToken(TokenType::T_NUMBER_INT, currentNumberValue);

        default: // Неизвестное семантическое действие - внутренняя ошибка
            currentState = LexerState::ERROR;
            return createErrorToken("Internal Lexer Error - Unknown semantic action: " + std::to_string(semanticAction));
        }
    }

    // Если цикл завершился не через return (маловероятно)
    return createErrorToken("Internal Lexer Error - Unexpected loop exit");
}