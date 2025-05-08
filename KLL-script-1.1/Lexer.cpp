#include "Lexer.h"
#include <iostream>
#include <cstring>
#include <cctype>    // Для isprint, isdigit, isalpha
#include <cstdlib>   // Для std::strtod
#include <sstream>   // Для createErrorToken

// --- Обновленные статические таблицы ---

// Категории: 0:a-z(excl 'e'), 1:0-9, 2:'+', 3:'-', 4:'=', 5:'*', 6:'/', 7:space/cr/tab,
//            8:'(', 9:')', 10:'[', 11:']', 12:'~', 13:'>', 14:'<', 15:'!', 16:';',
//            17:'\n', 18:'.', 19:'e'/'E', 20:'&', 21:'|', 22:other/$
const int Lexer::asciiCategoryTable[128] = {
    //  0..31 Control characters (mostly 22, \t=7, \n=17, \r=7)
       22,22,22,22,22,22,22,22,22, 7,17,22,22, 7,22,22,
       22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,
       // 32..47 ' ' ! " # $ % & ' ( ) * + , - . /
           7,15,22,22,22/*$*/,22,20/*&*/,22, 8, 9, 5, 2,22, 3,18, 6,
           // 48..63 0..9 : ; < = > ?
               1, 1, 1, 1, 1, 1, 1, 1, 1, 1,22,16,14, 4,13,22,
               // 64..79 @ A..D E F..O
                  22, 0, 0, 0, 0,19/*E*/,0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                  // 80..95 P..Z [ \ ] ^ _
                      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,10,22,11,22, 0,
                      // 96..111 ` a..d e f..o
                         22, 0, 0, 0, 0,19/*e*/,0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         //112..127 p..z { | } ~ DEL
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,22,21/*|*/,22,12,22
};


// Таблица переходов КА
// Столбцы: ... 19:e/E, 20:'&', 21:'|', 22:other/$
// Состояния: START(0)..EXP_DIGIT(7), AND1(8), OR1(9)
// Добавляем состояния, но таблица остается 10x23 (столбцы 0-22)
// Ошибки: 99 - Error state
const int Lexer::stateTransitionTable[10][23] = { // Увеличили размер до 23 столбцов
    //    a-z 0-9  +  -  =  *  / sp  (  )  [  ]  ~  >  <  !  ; \n  . e/E  &  | oth/$
    //     0   1   2  3  4  5  6  7  8  9 10 11 12 13 14 15 16  17 18  19 20 21   22
    /*S 0*/{ 1,  2,100 + '+',100 + '-',100 + '=',100 + '*',100 + '/', 8,100 + '(',100 + ')',100 + '[',100 + ']',100 + '~',100 + '>',100 + '<',100 + '!',100 + ';', 18,  3,  1, 30, 31, 99},// 30:увидели &, 31:увидели |
    /*ID 1*/{ 1,  1,122,122,122,122,122,122,122,122,122,122,122,122,122,122,122,123,122,  1,122,122,122},// '.' завершает ID
    /*INT 2*/{99,  2,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,129,  3,  4,128,128,128},
    /*DOT 3*/{99,  4, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99},
    /*FRA 4*/{99,  4,130,130,130,130,130,130,130,130,130,130,130,130,130,130,130,131, 99,  5,130,130,130},
    /*EXP 5*/{99,  7, 99, 6, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99},
    /*SGN 6*/{99,  7, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99},
    /*DIG 7*/{99,  7,130,130,130,130,130,130,130,130,130,130,130,130,130,130,130,131, 99, 99,130,130,130},
    /*AND1 8*/{99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 32, 99, 99},// 32: увидели второй '&'
    /*OR1 9*/{99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 33, 99}, // 33: увидели второй '|'
};
// Новые семантические программы:
// 30: Увидели первый '&', переход в состояние AND1(8)
// 31: Увидели первый '|', переход в состояние OR1(9)
// 32: Увидели второй '&', конечное состояние -> вернуть T_OP_AND
// 33: Увидели второй '|', конечное состояние -> вернуть T_OP_OR

// Семантические программы (действия):
// <100: внутренние действия
// 1: Начать идентификатор
// 2: Начать целое число
// 3: Увидели точку (возможно float)
// 4: Цифра после точки (состояние FRAC)
// 5: Увидели 'e'/'E' (возможно экспонента)
// 6: Увидели знак +/- после 'e'/'E'
// 7: Цифра после 'e'/'E' или после знака
// 8: Пропуск пробела/табуляции/CR
// 18: Пропуск перевода строки LF
// 21: Продолжить идентификатор
// 27: Продолжить целое число
// >=100: Конечное состояние, вернуть токен (код = действие - 100)
// 122: Конец идентификатора (не буква/цифра/e)
// 123: Конец идентификатора (перед \n)
// 128: Конец целого числа (не цифра, не '.', не 'e')
// 129: Конец целого числа (перед \n)
// 130: Конец float (после дробной части или экспоненты)
// 131: Конец float (перед \n)
// 99: Ошибка

// --- Конструктор ---
Lexer::Lexer(const std::string& source, SymbolTable& symTable)
    : sourceCode(source), currentPos(0), currentLine(1), lineStartPos(0),
    symbolTable(symTable) {
}

// --- Вспомогательные методы --- (без изменений)
char Lexer::peekNextChar() const { /* ... */ }
char Lexer::consumeChar() { /* ... */ }
int Lexer::getCurrentColumn() const { /* ... */ }

// --- Создание токенов --- (добавлены float, bool)
Token Lexer::createToken(TokenType type) { /* ... */ }
Token Lexer::createToken(TokenType type, const std::string& text) { /* ... */ }
Token Lexer::createToken(TokenType type, int value) { /* ... */ }
Token Lexer::createToken(TokenType type, double value) { // Новый
    return Token(type, value, currentLine, getCurrentColumn()); // Позиция примерная
}
Token Lexer::createToken(TokenType type, bool value) { // Новый
    return Token(type, value, currentLine, getCurrentColumn()); // Позиция примерная
}

Token Lexer::createErrorToken(const std::string& message) {
    char badChar = '?';
    if (currentPos < sourceCode.length()) {
        badChar = sourceCode[currentPos];
    }
    else if (currentPos > 0 && currentPos == sourceCode.length()) {
        badChar = sourceCode[currentPos - 1];
    }
    std::stringstream ss;
    ss << message << " (char: '"
        << (isprint(static_cast<unsigned char>(badChar)) ? std::string(1, badChar) : "\\?") << "', "
        << "ASCII: " << static_cast<int>(static_cast<unsigned char>(badChar)) << ")";
    std::cerr << "Lexical Error (Line " << currentLine << ", Col " << getCurrentColumn() << "): "
        << ss.str() << std::endl;
    return Token(TokenType::T_ERROR, currentLine, getCurrentColumn());
}


// --- Обработка идентификаторов/ключевых слов ---
Token Lexer::handleIdentifierOrKeyword(std::string& lexeme) {
    // Проверяем сначала на bool литералы
    if (lexeme == "true") {
        return createToken(TokenType::T_KW_TRUE, true);
    }
    if (lexeme == "false") {
        return createToken(TokenType::T_KW_FALSE, false);
    }
    // Затем на другие ключевые слова
    if (auto kwType = symbolTable.getKeywordType(lexeme); kwType.has_value()) {
        return createToken(kwType.value(), lexeme);
    }
    else {
        return createToken(TokenType::T_IDENTIFIER, lexeme);
    }
}

// --- Основной метод: получить следующий токен ---
Token Lexer::getNextToken() {
    LexerState currentState = LexerState::START;
    std::string currentLexeme = "";

    while (true) {
        char currentChar = peekNextChar();
        int category = 22; // other по умолчанию

        if (currentPos >= sourceCode.length()) {
            category = 22; // Используем категорию 'other' для EOF
        }
        else if (currentChar >= 0 && currentChar < 128) {
            category = asciiCategoryTable[static_cast<unsigned char>(currentChar)];
        }

        int action = 99; // Ошибка по умолчанию
        if (static_cast<int>(currentState) < 10) {
            // Проверяем границы категории
            if (category < 0 || category >= 23) { // Используем 23 как размер
                category = 22; // Если категория некорректна, считаем 'other'
            }
            action = stateTransitionTable[static_cast<int>(currentState)][category];
        }

        // --- Обработка EOF в состоянии START ---
        if (currentState == LexerState::START && currentPos >= sourceCode.length()) {
            return createToken(TokenType::T_EOF); // Явно возвращаем EOF
        }
        // --- Конец обработки EOF ---


        if (action == 99) {
            // Если мы в состоянии AND1 или OR1 и встретили не то, это ошибка
            if (currentState == LexerState::OP_AND_1 || currentState == LexerState::OP_OR_1) {
                consumeChar(); // Потребляем символ, вызвавший ошибку
                return createErrorToken("Incomplete operator: expected '&' or '|'");
            }
            // Иначе - общая ошибка
            consumeChar();
            return createErrorToken("Invalid character or state");
        }

        // Обработка конечных состояний (>= 100)
        if (action >= 100) {
            TokenType finalType = static_cast<TokenType>(action - 100);
            // ... (обработка T_IDENTIFIER, T_NUMBER_INT, T_NUMBER_FLOAT как раньше) ...
            // ... (используем std::stoi и std::strtod с try-catch) ...
            if (finalType == TokenType::T_IDENTIFIER) { // Конец идентификатора (122)
                return handleIdentifierOrKeyword(currentLexeme);
            }
            else if (finalType == TokenType::T_NUMBER_INT) { // Конец целого (128)
                try {
                    size_t processedChars = 0;
                    int val = std::stoi(currentLexeme, &processedChars);
                    // Дополнительная проверка, что вся строка была числом (на случай ошибок в КА)
                    if (processedChars != currentLexeme.length()) {
                        return createErrorToken("Invalid integer literal format (stray characters)");
                    }
                    return createToken(TokenType::T_NUMBER_INT, val);
                }
                catch (const std::invalid_argument&) {
                    return createErrorToken("Invalid integer literal format");
                }
                catch (const std::out_of_range&) {
                    return createErrorToken("Integer literal out of range");
                }
            }
            else if (finalType == TokenType::T_NUMBER_FLOAT) { // Конец float (130)
                try {
                    char* endPtr;
                    double val = std::strtod(currentLexeme.c_str(), &endPtr);
                    // Проверяем, что вся строка была разобрана
                    if (endPtr != currentLexeme.c_str() + currentLexeme.length()) {
                        return createErrorToken("Invalid float literal format (stray characters)");
                    }
                    // Проверка на INF / NAN (strtod их возвращает, но нам они не нужны)
                    if (!std::isfinite(val)) { // Нужно #include <cmath>
                        return createErrorToken("Floating point literal results in infinity or NaN");
                    }
                    return createToken(TokenType::T_NUMBER_FLOAT, val);
                }
                catch (...) { // Ловим другие возможные проблемы
                    return createErrorToken("Invalid float literal format");
                }
            }
            else { // Односимвольные токены
                consumeChar();
                return createToken(finalType);
            }
        }

        // Обработка семантических программ (< 100)
        switch (action) {
            // ... (case 1-7, 8, 18, 21, 27 как раньше) ...
        case 1: // Начать идентификатор
            currentState = LexerState::IDENTIFIER;
            currentLexeme += consumeChar();
            break;
        case 2: // Начать целое число
            currentState = LexerState::NUMBER_INT;
            currentLexeme += consumeChar();
            break;
        case 3: // Увидели точку
            currentState = LexerState::NUMBER_FLOAT_DOT;
            currentLexeme += consumeChar();
            break;
        case 4: // Цифра после точки -> FRAC
            currentState = LexerState::NUMBER_FLOAT_FRAC;
            currentLexeme += consumeChar();
            break;
        case 5: // Увидели 'e'/'E' -> EXP
            currentState = LexerState::NUMBER_FLOAT_EXP;
            currentLexeme += consumeChar();
            break;
        case 6: // Знак +/- после 'e'/'E' -> EXP_SIGN
            currentState = LexerState::NUMBER_FLOAT_EXP_SIGN;
            currentLexeme += consumeChar();
            break;
        case 7: // Цифра после 'e', знака или другая цифра в экспоненте -> EXP_DIGIT
            currentState = LexerState::NUMBER_FLOAT_EXP_DIGIT;
            currentLexeme += consumeChar();
            break;
        case 8: // Пропуск пробела/табуляции/CR
            consumeChar();
            // currentState = LexerState::START; // Не меняем состояние! Остаемся в START
            break;
        case 18: // Пропуск перевода строки LF
            consumeChar();
            // currentState = LexerState::START; // Не меняем состояние! Остаемся в START
            break;
        case 21: // Продолжить идентификатор
            currentLexeme += consumeChar();
            break;
        case 27: // Продолжить целое число
            currentLexeme += consumeChar();
            break;

            // --- Новые действия для && и || ---
        case 30: // Увидели первый '&'
            currentState = LexerState::OP_AND_1;
            consumeChar(); // Потребляем первый '&'
            // Не добавляем в lexeme, т.к. нам нужен сам токен &&
            break;
        case 31: // Увидели первый '|'
            currentState = LexerState::OP_OR_1;
            consumeChar(); // Потребляем первый '|'
            break;
        case 32: // Увидели второй '&'
            consumeChar(); // Потребляем второй '&'
            return createToken(TokenType::T_OP_AND); // Возвращаем токен &&
        case 33: // Увидели второй '|'
            consumeChar(); // Потребляем второй '|'
            return createToken(TokenType::T_OP_OR); // Возвращаем токен ||

            // Обработка завершения на \n (действия 123, 129, 131)
        case 123: // Конец идентификатора перед \n
            return handleIdentifierOrKeyword(currentLexeme);
        case 129: // Конец целого перед \n
            try { /* ... stoi ... */ }
            catch (...) { /* ... */ }
        case 131: // Конец float перед \n
            try { /* ... strtod ... */ }
            catch (...) { /* ... */ }

        default:
            consumeChar();
            return createErrorToken("Internal Lexer Error - Unexpected action: " + std::to_string(action));
        }
    }
    // Сюда не должны дойти
    return createErrorToken("Internal Lexer Error - Lexer loop exited unexpectedly");
}