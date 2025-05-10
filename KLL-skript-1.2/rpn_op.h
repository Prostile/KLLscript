// rpn_op.h
#ifndef RPN_OP_H
#define RPN_OP_H

#include <variant>
#include <optional>   // Для опциональных полей
#include <string>     // Для отладочного вывода или хранения имен меток, если понадобится

#include "definitions.h" // Нужен RPNOpCode

// Структура для элемента ОПС (RPN - Reverse Polish Notation)
struct RPNOperation {
    RPNOpCode opCode; // Код операции

    // Операнд, если операция его требует (например, константа)
    // Используем std::variant для хранения int или float.
    // std::monostate означает отсутствие явного значения операнда (например, для ADD, SUB).
    std::variant<std::monostate, int, float> operandValue;

    // Индекс символа в таблице символов (для PUSH_VAR_ADDR, PUSH_ARRAY_ADDR)
    std::optional<size_t> symbolIndex;

    // Цель перехода (индекс в векторе ОПС) для операций JUMP, JUMP_FALSE
    // Инициализируется -1 или отсутствующим значением, чтобы показать, что адрес еще не установлен (placeholder)
    std::optional<int> jumpTarget;

    // --- Конструкторы ---

    // Конструктор для операций без явного операнда-значения и без индекса символа
    // (например, ADD, SUB, MUL, DIV, INDEX, CONVERT_TO_INT, CONVERT_TO_FLOAT)
    explicit RPNOperation(RPNOpCode code)
        : opCode(code), operandValue(std::monostate{}), symbolIndex(std::nullopt), jumpTarget(std::nullopt) {
    }

    // Конструктор для операций с целочисленным операндом-значением (PUSH_CONST_INT)
    RPNOperation(RPNOpCode code, int val)
        : opCode(code), operandValue(val), symbolIndex(std::nullopt), jumpTarget(std::nullopt) {
        // Убедимся, что конструктор используется для правильного opCode
        // if (code != RPNOpCode::PUSH_CONST_INT) { /* Логика ошибки или предупреждения */ }
    }

    // Конструктор для операций с вещественным операндом-значением (PUSH_CONST_FLOAT)
    RPNOperation(RPNOpCode code, float val)
        : opCode(code), operandValue(val), symbolIndex(std::nullopt), jumpTarget(std::nullopt) {
        // if (code != RPNOpCode::PUSH_CONST_FLOAT) { /* Логика ошибки или предупреждения */ }
    }

    // Конструктор для операций, использующих индекс символа в таблице символов
    // (PUSH_VAR_ADDR, PUSH_ARRAY_ADDR)
    RPNOperation(RPNOpCode code, size_t symIdx)
        : opCode(code), operandValue(std::monostate{}), symbolIndex(symIdx), jumpTarget(std::nullopt) {
        // if (code != RPNOpCode::PUSH_VAR_ADDR && code != RPNOpCode::PUSH_ARRAY_ADDR) { /* ... */ }
    }

    // Конструктор для операций ввода/вывода (они могут не иметь других операндов в этой структуре)
    // Используем 'bool' как фиктивный параметр, чтобы отличить от первого конструктора
    RPNOperation(RPNOpCode code, bool isIoOperationPlaceholder)
        : opCode(code), operandValue(std::monostate{}), symbolIndex(std::nullopt), jumpTarget(std::nullopt) {
        // Эта перегрузка нужна, чтобы избежать неоднозначности с RPNOperation(RPNOpCode code)
        // Можно добавить проверку, что code это READ_INT, READ_FLOAT, WRITE_INT, WRITE_FLOAT
        // if (code != RPNOpCode::READ_INT && code != RPNOpCode::READ_FLOAT &&
        //     code != RPNOpCode::WRITE_INT && code != RPNOpCode::WRITE_FLOAT) {
        //     // Логика ошибки, если этот конструктор вызван не для операций ввода/вывода
        // }
    }


    // Конструктор для операций перехода (JUMP, JUMP_FALSE) - создает "заглушку"
    // Используем 'int' как фиктивный параметр placeholderTarget, чтобы отличить от других
    // или можно было бы использовать другой bool.
    // Инициализируем jumpTarget значением по умолчанию (например, -1 или std::nullopt).
    RPNOperation(RPNOpCode code, int placeholderTarget, bool isJumpOperation)
        : opCode(code), operandValue(std::monostate{}), symbolIndex(std::nullopt) {
        if (code == RPNOpCode::JUMP || code == RPNOpCode::JUMP_FALSE) {
            jumpTarget = placeholderTarget; // Обычно -1 или специальное значение для незаполненного адреса
            // Парсер будет использовать std::nullopt или -1 для начального значения
        }
        else {
            jumpTarget = std::nullopt; // Не переход, цели нет
            // Можно добавить сообщение об ошибке, если этот конструктор вызван не для JUMP/JUMP_FALSE
        }
    }
};

#endif // RPN_OP_H