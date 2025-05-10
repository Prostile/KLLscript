// token.h
#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <variant>
#include <utility> // Для std::move

#include "definitions.h"

// Структура для представления Токена (Лексемы)
struct Token {
    TokenType type;
    std::string text; // Текстовое представление токена (для идентификаторов, ошибок, и т.д.)

    // Значение токена, если это число.
    // Используем std::variant для хранения либо int, либо float.
    // Может быть пустым (std::monostate), если токен не числовой.
    std::variant<std::monostate, int, float> value;

    int line;     // Номер строки, где токен начинается
    int column;   // Номер столбца, где токен начинается

    // Конструктор по умолчанию (для создания "пустых" или неинициализированных токенов)
    Token() : type(TokenType::T_UNKNOWN), text(""), value(std::monostate{}), line(0), column(0) {}

    // Конструктор для токенов без специфического значения, но с текстом (например, операторы, ключевые слова)
    Token(TokenType t, std::string txt, int l, int c)
        : type(t), text(std::move(txt)), value(std::monostate{}), line(l), column(c) {
    }

    // Конструктор для токенов без текста и значения (например, T_EOF, T_SEMICOLON из ASCII)
    Token(TokenType t, int l, int c)
        : type(t), text(""), value(std::monostate{}), line(l), column(c) {
    }

    // Конструктор для целочисленных токенов
    Token(TokenType t, int val, int l, int c, std::string txt = "")
        : type(t), text(std::move(txt)), value(val), line(l), column(c) {
        if (text.empty()) { // Если текст не передан, формируем его из числа
            this->text = std::to_string(val);
        }
    }

    // Конструктор для вещественных токенов
    Token(TokenType t, float val, int l, int c, std::string txt = "")
        : type(t), text(std::move(txt)), value(val), line(l), column(c) {
        if (text.empty()) { // Если текст не передан, формируем его из числа
            // Используем to_string, но можно настроить точность при необходимости
            this->text = std::to_string(val);
        }
    }

    // Вспомогательные методы для получения значения (с проверкой типа)
    int getIntValue() const {
        if (std::holds_alternative<int>(value)) {
            return std::get<int>(value);
        }
        // Можно бросить исключение или вернуть значение по умолчанию/ошибку
        // Для простоты учебного проекта, можно положиться на корректное создание токена
        // или добавить вывод в cerr и exit.
        // std::cerr << "Error: Attempted to get int value from a non-int token." << std::endl;
        return 0; // Возвращаем 0 или другое значение по умолчанию
    }

    float getFloatValue() const {
        if (std::holds_alternative<float>(value)) {
            return std::get<float>(value);
        }
        // std::cerr << "Error: Attempted to get float value from a non-float token." << std::endl;
        return 0.0f; // Возвращаем 0.0f или другое значение по умолчанию
    }

    bool isNumber() const {
        return type == TokenType::T_NUMBER_INT || type == TokenType::T_NUMBER_FLOAT;
    }
};

#endif // TOKEN_H