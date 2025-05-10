// definitions.h
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <string> 

// --- ��������� ������������ ����������� ---
// ���������� enum class ��� ������� ���������������� � ��������� ���������� ����
enum class LexerState {
    START,                  // 0: ��������� ���������
    IDENTIFIER,             // 1: ������ ��������������/��������� �����
    NUMBER_INT,             // 2: ������ ����� ����� �����
    NUMBER_FLOAT_DECIMAL,   // 3: ������ ������� ����� ����� (����� �����)
    NUMBER_FLOAT_EXP        // 4: ������ ���������������� ����� (���� �� ������������)
    // ������ � ��������� ��������� �� ����� ��� ����� ��������� ��,
    // ��� ������������ ������� ������ getNextToken �� ������ ������������� ��������
};


// --- ���� ������ (�������) ---
// ... (��������� ��� definitions.h ��� ���������) ...
enum class TokenType {
    // �������� � ��������������
    T_IDENTIFIER,   // �������������
    T_NUMBER_INT,   // ������������� ���������
    T_NUMBER_FLOAT, // ������������ ���������

    // �������� �����
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

    // ��������� � �����������
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

    // ��������� ���������
    T_EQUAL = '~',
    T_GREATER = '>',
    T_LESS = '<',
    T_NOT_EQUAL = '!',

    // ����������� ������
    T_EOF,
    T_ERROR,
    T_UNKNOWN
};

// --- ���� �������� ��� (RPN - Reverse Polish Notation) ---
// ... (��������� ��� definitions.h ��� ���������) ...
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


// --- ���� �������� � ������� �������� ---
// ... (��������� ��� definitions.h ��� ���������) ...
enum class SymbolType {
    VARIABLE_INT,
    VARIABLE_FLOAT,
    ARRAY_INT,
    ARRAY_FLOAT
};

#endif // DEFINITIONS_H