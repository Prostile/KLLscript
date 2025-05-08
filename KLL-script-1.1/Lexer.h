#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <optional>

#include "definitions.h"
#include "DataStructures.h"
#include "SymbolTable.h" // ��� �������� �������� ���� 'true', 'false', 'not'

class Lexer {
private:
    const std::string& sourceCode;
    size_t currentPos;
    int currentLine;
    int lineStartPos;

    SymbolTable& symbolTable; // ��� �������� ���� �������� ����

    // ����������� ������� ��������� ��
    // �����������: [���������� ���������][���������� ��������� ��������]
    // ���������: START(0), IDENTIFIER(1), NUMBER_INT(2), FLOAT_DOT(3), FLOAT_FRAC(4),
    //            FLOAT_EXP(5), FLOAT_EXP_SIGN(6), FLOAT_EXP_DIGIT(7), OP_AND_1(8), OP_OR_1(9)
    static const int stateTransitionTable[10][23]; // +1 ��������� ��� '.'

    // ����������� ������� ��������� ��������
    // ���������: 0:a-z(excl 'e'), 1:0-9, 2:'+', 3:'-', 4:'=', 5:'*', 6:'/', 7:space/cr/tab,
    //            8:'(', 9:')', 10:'[', 11:']', 12:'~', 13:'>', 14:'<', 15:'!', 16:';',
    //            17:'\n', 18:'.', 19:'e'/'E', 20:other/&/|/$
    static const int asciiCategoryTable[128];

    // ���������� ������ - ������������� ���������
    Token handleIdentifierOrKeyword(std::string& lexeme);
    // ��� ���������� handleNumber, ������ �������� � switch
    Token createToken(TokenType type);
    Token createToken(TokenType type, const std::string& text);
    Token createToken(TokenType type, int value);
    Token createToken(TokenType type, double value); // ��� float
    Token createToken(TokenType type, bool value);   // ��� bool
    Token createErrorToken(const std::string& message);

    // ��������������� ������
    char peekNextChar() const;
    char consumeChar();
    int getCurrentColumn() const;

public:
    Lexer(const std::string& source, SymbolTable& symTable);
    Token getNextToken();
};

#endif // LEXER_H