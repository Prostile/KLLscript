#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <string>
#include <variant> // Используем variant для SymbolValue

// --- Состояния Лексического Анализатора ---
enum class LexerState {
    START,
    IDENTIFIER,
    NUMBER_INT,
    NUMBER_FLOAT_DOT,   // Состояние после точки (ожидаем цифру)
    NUMBER_FLOAT_FRAC,  // Состояние после цифры в дробной части
    NUMBER_FLOAT_EXP,   // Состояние после 'e' или 'E' (ожидаем знак или цифру)
    NUMBER_FLOAT_EXP_SIGN,// Состояние после знака в экспоненте
    NUMBER_FLOAT_EXP_DIGIT,// Состояние после цифры в экспоненте
    OP_AND_1,           // Состояние после первого '&'
    OP_OR_1,            // Состояние после первого '|'
    FINAL,
    ERROR
};

// --- Типы Лексем (Токенов) ---
enum TokenType {
    // Литералы и идентификаторы
    T_IDENTIFIER = 257,
    T_NUMBER_INT = 256,
    T_NUMBER_FLOAT = 255, // Новый тип лексемы для float
    T_KW_TRUE = 276,      // true (теперь как отдельный токен, не KW)
    T_KW_FALSE = 277,     // false (теперь как отдельный токен, не KW)

    // Ключевые слова
    T_KW_INT = 266,
    T_KW_ARR = 267,
    T_KW_FLOAT = 274,    // Новое ключевое слово
    T_KW_BOOL = 275,    // Новое ключевое слово
    T_KW_IF = 259,
    T_KW_ELSE = 261,
    T_KW_WHILE = 262,
    T_KW_BEGIN = 272,
    T_KW_END = 273,
    T_KW_CIN = 264,
    T_KW_COUT = 265,
    T_KW_NOT = 280,    // Ключевое слово 'not'

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
    T_DOT = '.',    // Точка, нужна для float литералов

    // Операторы сравнения
    T_EQUAL = '~',
    T_GREATER = '>',
    T_LESS = '<',
    T_NOT_EQUAL = '!',

    // Логические операторы
    T_OP_AND = 278,    // &&
    T_OP_OR = 279,    // ||
    // T_KW_NOT обрабатывается выше

    // Специальные токены
    T_EOF = 271,
    T_ERROR = -1,
    T_UNKNOWN = 0
};


// --- Типы операций ОПС (RPN - Reverse Polish Notation) ---
enum class RPNOpCode {
    // Операнды
    PUSH_VAR,
    PUSH_ARRAY,
    PUSH_CONST_INT,
    PUSH_CONST_FLOAT, // Добавлено
    PUSH_CONST_BOOL,  // Добавлено

    // Бинарные арифметические операции (для int)
    ADD_I, SUB_I, MUL_I, DIV_I,
    // Бинарные арифметические операции (для float)
    ADD_F, SUB_F, MUL_F, DIV_F, // Добавлено

    // Унарный минус
    NEG_I, // Для int
    NEG_F, // Для float (Добавлено)

    // Бинарные операции сравнения (результат bool)
    CMP_EQ_I, CMP_NE_I, CMP_GT_I, CMP_LT_I, // Для int
    CMP_EQ_F, CMP_NE_F, CMP_GT_F, CMP_LT_F, // Для float (Добавлено)
    CMP_EQ_B, CMP_NE_B,                     // Для bool (Добавлено)

    // Логические операции (для bool)
    AND, OR, NOT, // Добавлено

    // Операция присваивания (типы операндов определяются во время выполнения)
    ASSIGN,

    // Операция индексации (индекс всегда int)
    INDEX,

    // Унарные операции ввода/вывода
    READ_I, READ_F, READ_B,   // Добавлено разделение
    WRITE_I, WRITE_F, WRITE_B, // Добавлено разделение

    // Операции перехода
    JUMP,
    JUMP_FALSE,

    // Операции приведения типов
    CAST_I2F, // int to float (Добавлено)
    CAST_I2B, // int to bool (Добавлено)
    // CAST_F2I, // Не разрешено неявно
    // CAST_F2B, // Не разрешено
    // CAST_B2I, // Возможно, понадобится
    // CAST_B2F, // Возможно, понадобится

    // Метка (используется как операнд для переходов)
    LABEL
};

// --- Прочие константы ---
const int SYMBOL_TABLE_SIZE = 100;
const int RPN_STRING_BUFFER_SIZE = 20; // Увеличим для float

#endif // DEFINITIONS_H