// token.h
#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <variant>
#include <utility> // ��� std::move

#include "definitions.h"

// ��������� ��� ������������� ������ (�������)
struct Token {
    TokenType type;
    std::string text; // ��������� ������������� ������ (��� ���������������, ������, � �.�.)

    // �������� ������, ���� ��� �����.
    // ���������� std::variant ��� �������� ���� int, ���� float.
    // ����� ���� ������ (std::monostate), ���� ����� �� ��������.
    std::variant<std::monostate, int, float> value;

    int line;     // ����� ������, ��� ����� ����������
    int column;   // ����� �������, ��� ����� ����������

    // ����������� �� ��������� (��� �������� "������" ��� �������������������� �������)
    Token() : type(TokenType::T_UNKNOWN), text(""), value(std::monostate{}), line(0), column(0) {}

    // ����������� ��� ������� ��� �������������� ��������, �� � ������� (��������, ���������, �������� �����)
    Token(TokenType t, std::string txt, int l, int c)
        : type(t), text(std::move(txt)), value(std::monostate{}), line(l), column(c) {
    }

    // ����������� ��� ������� ��� ������ � �������� (��������, T_EOF, T_SEMICOLON �� ASCII)
    Token(TokenType t, int l, int c)
        : type(t), text(""), value(std::monostate{}), line(l), column(c) {
    }

    // ����������� ��� ������������� �������
    Token(TokenType t, int val, int l, int c, std::string txt = "")
        : type(t), text(std::move(txt)), value(val), line(l), column(c) {
        if (text.empty()) { // ���� ����� �� �������, ��������� ��� �� �����
            this->text = std::to_string(val);
        }
    }

    // ����������� ��� ������������ �������
    Token(TokenType t, float val, int l, int c, std::string txt = "")
        : type(t), text(std::move(txt)), value(val), line(l), column(c) {
        if (text.empty()) { // ���� ����� �� �������, ��������� ��� �� �����
            // ���������� to_string, �� ����� ��������� �������� ��� �������������
            this->text = std::to_string(val);
        }
    }

    // ��������������� ������ ��� ��������� �������� (� ��������� ����)
    int getIntValue() const {
        if (std::holds_alternative<int>(value)) {
            return std::get<int>(value);
        }
        // ����� ������� ���������� ��� ������� �������� �� ���������/������
        // ��� �������� �������� �������, ����� ���������� �� ���������� �������� ������
        // ��� �������� ����� � cerr � exit.
        // std::cerr << "Error: Attempted to get int value from a non-int token." << std::endl;
        return 0; // ���������� 0 ��� ������ �������� �� ���������
    }

    float getFloatValue() const {
        if (std::holds_alternative<float>(value)) {
            return std::get<float>(value);
        }
        // std::cerr << "Error: Attempted to get float value from a non-float token." << std::endl;
        return 0.0f; // ���������� 0.0f ��� ������ �������� �� ���������
    }

    bool isNumber() const {
        return type == TokenType::T_NUMBER_INT || type == TokenType::T_NUMBER_FLOAT;
    }
};

#endif // TOKEN_H