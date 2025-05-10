// definitions.h
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <string> 

// --- Состояния Лексического Анализатора ---
// Используем enum class для большей типобезопасности и избежания конфликтов имен
enum class LexerState {
    START,                  // 0: Начальное состояние
    IDENTIFIER,             // 1: Чтение идентификатора/ключевого слова
    NUMBER_INT,             // 2: Чтение целой части числа
    NUMBER_FLOAT_DECIMAL,   // 3: Чтение дробной части числа (после точки)
    NUMBER_FLOAT_EXP        // 4: Чтение экспоненциальной части (пока не используется)
    // Ошибка и финальное состояние не нужны как явные состояния КА,
    // они определяются логикой внутри getNextToken на основе семантических действий
};


// --- Типы Лексем (Токенов) ---
// ... (остальной код definitions.h без изменений) ...
enum class TokenType {
    // Литералы и идентификаторы
    T_IDENTIFIER,   // Идентификатор
    T_NUMBER_INT,   // Целочисленная константа
    T_NUMBER_FLOAT, // Вещественная константа

    // Ключевые слова
    T_KW_INT,       // int
    T_KW_FLOAT,     // float
    T_KW_ARR,       // arr 
    T_KW_IF,        // if
    T_KW_ELSE,      // else
    T_KW_WHILE,     // while
    T_KW_BEGIN,     // begin
    T_KW_END,       // end
    T_KW_CIN,       // cin
    T_KW_COUT,      // cout

    // Операторы и разделители
    T_ASSIGN = '=',
    T_PLUS = '+',
    T_MINUS = '-',
    T_MULTIPLY = '*',
    T_DIVIDE = '/',
    T_LPAREN = '(',
    T_RPAREN = ')',
    T_LBRACKET = '[',
    T_RBRACKET = ']',
    T_SEMICOLON = ';',
    T_COMMA = ',',

    // Операторы сравнения
    T_EQUAL = '~',
    T_GREATER = '>',
    T_LESS = '<',
    T_NOT_EQUAL = '!',

    // Специальные токены
    T_EOF,
    T_ERROR,
    T_UNKNOWN
};

// --- Типы операций ОПС (RPN - Reverse Polish Notation) ---
// ... (остальной код definitions.h без изменений) ...
enum class RPNOpCode {
    PUSH_VAR_ADDR,
    PUSH_ARRAY_ADDR,
    PUSH_CONST_INT,
    PUSH_CONST_FLOAT,

    ADD,
    SUB,
    MUL,
    DIV,

    CMP_EQ,
    CMP_NE,
    CMP_GT,
    CMP_LT,

    ASSIGN,

    INDEX,

    READ_INT,
    READ_FLOAT,
    WRITE_INT,
    WRITE_FLOAT,

    JUMP,
    JUMP_FALSE,

    CONVERT_TO_FLOAT,
    CONVERT_TO_INT
};


// --- Типы символов в таблице символов ---
// ... (остальной код definitions.h без изменений) ...
enum class SymbolType {
    VARIABLE_INT,
    VARIABLE_FLOAT,
    ARRAY_INT,
    ARRAY_FLOAT
};

#endif // DEFINITIONS_H