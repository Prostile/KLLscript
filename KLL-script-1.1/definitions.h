#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <string>
#include <variant> // ���������� variant ��� SymbolValue

// --- ��������� ������������ ����������� ---
enum class LexerState {
    START,
    IDENTIFIER,
    NUMBER_INT,
    NUMBER_FLOAT_DOT,   // ��������� ����� ����� (������� �����)
    NUMBER_FLOAT_FRAC,  // ��������� ����� ����� � ������� �����
    NUMBER_FLOAT_EXP,   // ��������� ����� 'e' ��� 'E' (������� ���� ��� �����)
    NUMBER_FLOAT_EXP_SIGN,// ��������� ����� ����� � ����������
    NUMBER_FLOAT_EXP_DIGIT,// ��������� ����� ����� � ����������
    OP_AND_1,           // ��������� ����� ������� '&'
    OP_OR_1,            // ��������� ����� ������� '|'
    FINAL,
    ERROR
};

// --- ���� ������ (�������) ---
enum TokenType {
    // �������� � ��������������
    T_IDENTIFIER = 257,
    T_NUMBER_INT = 256,
    T_NUMBER_FLOAT = 255, // ����� ��� ������� ��� float
    T_KW_TRUE = 276,      // true (������ ��� ��������� �����, �� KW)
    T_KW_FALSE = 277,     // false (������ ��� ��������� �����, �� KW)

    // �������� �����
    T_KW_INT = 266,
    T_KW_ARR = 267,
    T_KW_FLOAT = 274,    // ����� �������� �����
    T_KW_BOOL = 275,    // ����� �������� �����
    T_KW_IF = 259,
    T_KW_ELSE = 261,
    T_KW_WHILE = 262,
    T_KW_BEGIN = 272,
    T_KW_END = 273,
    T_KW_CIN = 264,
    T_KW_COUT = 265,
    T_KW_NOT = 280,    // �������� ����� 'not'

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
    T_DOT = '.',    // �����, ����� ��� float ���������

    // ��������� ���������
    T_EQUAL = '~',
    T_GREATER = '>',
    T_LESS = '<',
    T_NOT_EQUAL = '!',

    // ���������� ���������
    T_OP_AND = 278,    // &&
    T_OP_OR = 279,    // ||
    // T_KW_NOT �������������� ����

    // ����������� ������
    T_EOF = 271,
    T_ERROR = -1,
    T_UNKNOWN = 0
};


// --- ���� �������� ��� (RPN - Reverse Polish Notation) ---
enum class RPNOpCode {
    // ��������
    PUSH_VAR,
    PUSH_ARRAY,
    PUSH_CONST_INT,
    PUSH_CONST_FLOAT, // ���������
    PUSH_CONST_BOOL,  // ���������

    // �������� �������������� �������� (��� int)
    ADD_I, SUB_I, MUL_I, DIV_I,
    // �������� �������������� �������� (��� float)
    ADD_F, SUB_F, MUL_F, DIV_F, // ���������

    // ������� �����
    NEG_I, // ��� int
    NEG_F, // ��� float (���������)

    // �������� �������� ��������� (��������� bool)
    CMP_EQ_I, CMP_NE_I, CMP_GT_I, CMP_LT_I, // ��� int
    CMP_EQ_F, CMP_NE_F, CMP_GT_F, CMP_LT_F, // ��� float (���������)
    CMP_EQ_B, CMP_NE_B,                     // ��� bool (���������)

    // ���������� �������� (��� bool)
    AND, OR, NOT, // ���������

    // �������� ������������ (���� ��������� ������������ �� ����� ����������)
    ASSIGN,

    // �������� ���������� (������ ������ int)
    INDEX,

    // ������� �������� �����/������
    READ_I, READ_F, READ_B,   // ��������� ����������
    WRITE_I, WRITE_F, WRITE_B, // ��������� ����������

    // �������� ��������
    JUMP,
    JUMP_FALSE,

    // �������� ���������� �����
    CAST_I2F, // int to float (���������)
    CAST_I2B, // int to bool (���������)
    // CAST_F2I, // �� ��������� ������
    // CAST_F2B, // �� ���������
    // CAST_B2I, // ��������, �����������
    // CAST_B2F, // ��������, �����������

    // ����� (������������ ��� ������� ��� ���������)
    LABEL
};

// --- ������ ��������� ---
const int SYMBOL_TABLE_SIZE = 100;
const int RPN_STRING_BUFFER_SIZE = 20; // �������� ��� float

#endif // DEFINITIONS_H