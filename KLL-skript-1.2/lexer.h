// lexer.h
#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

#include "definitions.h"    // TokenType, LexerState
#include "token.h"          // ��������� Token
#include "symbol_table.h"   // ��� �������� �������� ����
#include "error_handler.h"  // ��� ����������� ������

class Lexer {
private:
    const std::string& sourceCode; // ������ �� �������� ���
    size_t currentPos;             // ������� ������� � sourceCode
    int currentLine;               // ������� ����� ������
    int lineStartPos;              // ������� ������ ������� ������ (��� ���������� �������)

    SymbolTable& symbolTable;      // ������ �� ������� ��������
    ErrorHandler& errorHandler;    // ������ �� ���������� ������

    // ������� ��� ��������� ��������
    // [���������][��������� �������] -> ����� �������������� ��������
    // ���������: 0-START, 1-IDENTIFIER, 2-NUMBER_INT, 3-NUMBER_FLOAT_DECIMAL, 4-NUMBER_FLOAT_EXP
    // ��������� ��������: ��. ����������� � asciiCategoryTable
    static const int stateTransitionTable[5][21]; // ��������� ���������� ��������� ��� float

    // ������� �������������� ASCII ����� � ��������� ��������
    // ���������: 0:'a'-'z','A'-'Z', 1:'0'-'9', 2:'+', 3:'-', 4:'=', 5:'*', 6:'/',
    //            7:' ', 8:'(', 9:')', 10:'[', 11:']', 12:'~', 13:'>', 14:'<',
    //            15:'!', 16:';', 17:'\n', 18:other_printable, 19:'$', 20:'.' (����� ��� float)
    static const int asciiCategoryTable[128];

    // ��������������� ������
    char peekChar(size_t offset = 0) const; // ���������� ������ �� ���������, �� ������� �������
    char consumeChar();                     // ��������� ������� ������ � �������� �������
    int getCurrentColumn() const;           // ��������� ������� �������

    // ������ ��� �������� �������, ��������� errorHandler ��� ���������
    Token createFinalToken(TokenType type, const std::string& text = "");
    Token createIntToken(int value, const std::string& text);
    Token createFloatToken(float value, const std::string& text);
    // createErrorToken ������ �� ����� �����, �.�. ������ ���������� ����� errorHandler

public:
    Lexer(const std::string& source, SymbolTable& symTable, ErrorHandler& errHandler);

    // �������� ������� ������������ ����������� (���������� ��)
    Token getNextToken();
};

#endif // LEXER_H