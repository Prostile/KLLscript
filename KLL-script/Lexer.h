#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <optional>

#include "definitions.h"
#include "DataStructures.h" // ����� ����������� Token
// #include "ErrorHandler.h" // ����� ����� �����
#include "SymbolTable.h"  // ��� �������� �������� ����

// --- ����� ������������ ����������� ---
class Lexer {
private:
    const std::string& sourceCode; // ������ �� �������� ���
    size_t currentPos;             // ������� ������� � sourceCode
    int currentLine;               // ������� ����� ������
    int lineStartPos;              // ������� ������ ������� ������ (��� ���������� �������)

    SymbolTable& symbolTable;      // ������ �� ������� �������� (��� �������� ����)
    // ErrorHandler& errorHandler; // ������ �� ���������� ������

    // ������� ��������� �� (State, InputCategory -> SemanticAction)
    // �����������: [���������� ���������][���������� ��������� ��������]
    // ��������: ����� ������������� ���������
    static const int stateTransitionTable[3][20];

    // ������� �������������� ASCII ����� � ��������� ��������
    static const int asciiCategoryTable[128];

    // ���������� ������ - ������������� ���������
    Token processAction(int semanticAction, char currentChar);
    Token handleIdentifierOrKeyword(std::string& lexeme);
    Token handleNumber(int& value, char digit);
    Token createToken(TokenType type); // ������� ����� � ������� ��������
    Token createToken(TokenType type, const std::string& text);
    Token createToken(TokenType type, int value);
    Token createErrorToken(const std::string& message);

    // ��������������� ������
    char peekNextChar() const; // ���������� ��������� ������, �� ������� �������
    char consumeChar();        // ��������� ������� ������ � �������� �������
    int getCurrentColumn() const; // ��������� ������� �������

public:
    // �����������
    Lexer(const std::string& source, SymbolTable& symTable /*, ErrorHandler& errHandler*/);

    // �������� �����: �������� ��������� ����� �� ������
    Token getNextToken();
};

#endif // LEXER_H