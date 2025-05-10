// lexer.cpp
#include "lexer.h"
#include <cctype>   // ��� isalpha, isdigit, isprint
#include <sstream>  // ��� �������������� ��������� �� �������

// --- ������������� ����������� ������ ---

// ���������:
// 0: START
// 1: IDENTIFIER
// 2: NUMBER_INT (����� ����� �����)
// 3: NUMBER_FLOAT_DECIMAL (����� �����, ������� �����)
// 4: NUMBER_FLOAT_EXP (����� 'e' ��� 'E', ������� ���� ��� ����� - �� ��������� ��� ��������� �� �������)

// ��������� �������� (������� ��� stateTransitionTable):
// 0: ����� (a-z, A-Z)
// 1: ����� (0-9)
// 2: +
// 3: -
// 4: =
// 5: *
// 6: /
// 7: ������, \t, \r
// 8: (
// 9: )
// 10: [
// 11: ]
// 12: ~ (��� ==)
// 13: >
// 14: <
// 15: ! (��� !=)
// 16: ;
// 17: \n (������� ������)
// 18: ������ �������� ������� (�� �������� � ������ ���������, ��������, �������)
// 19: $ (EOF)
// 20: . (����� ��� float)

const int Lexer::stateTransitionTable[5][21] = {
    // ���������:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20
    // ������:    ��� ��� +   -   =   *   /  ���� (   )   [   ]   ~   >   <   !   ;  \n  �� EOF  .
    /* 0 START */ { 1,  2, 30, 31, 32, 33, 34,  0, 35, 36, 37, 38, 39, 40, 41, 42, 43,  0, 45, 46, 47}, // �������� 30-43,46 - �������� ������� �������; 0-�����; 45-������; 47-������ float � �����
    /* 1 IDENT */ { 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11}, // 10-���������� �������������; 11-��������� �������������
    /* 2 NUM_INT */ { 21, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,  3}, // 20-���������� �����; 21-��������� �����; 3-������� � ��������� float_decimal
    /* 3 NUM_FLT_D*/{ 23, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23}, // 22-���������� ������� �����; 23-��������� float
    /* 4 NUM_FLT_E */{99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99}  // ��������� ��� ���������� (���� �� ������������, 99 - ������/�����������)
};
// �������� (������������� ���������):
// 0: ������������ ������, �������� � START
// 1: ������ ��������������, ������� � IDENTIFIER, �������� ������
// 2: ������ ������ �����, ������� � NUMBER_INT, �������� �����
// 3: ��������� ����� ����� ����, ������� � NUMBER_FLOAT_DECIMAL, �������� �����
// 10: ���������� �������������, �������� � IDENTIFIER, �������� ������
// 11: ��������� �������������/�������� �����, ������� �����, ������ �� ����������
// 20: ���������� ����� �����, �������� � NUMBER_INT, �������� �����
// 21: ��������� ����� �����, ������� �����, ������ �� ����������
// 22: ���������� ������� ����� float, �������� � NUMBER_FLOAT_DECIMAL, �������� �����
// 23: ��������� float, ������� �����, ������ �� ����������
// 30-43: ��������� �������������� ����� ��� ������� �����������, ������� �����, ������ ���������
// 45: ������ - ����������� ������
// 46: EOF - ������� T_EOF
// 47: ������ float � ����� (��������, ".5"), ������� � NUMBER_FLOAT_DECIMAL, �������� ����� (� "0" ����� ���)


const int Lexer::asciiCategoryTable[128] = {
    // ����������� ������� (0-31), ����������� - "������" ��� \n, \t, \r
    18, 18, 18, 18, 18, 18, 18, 18, 18,  7, 17, 18, 18,  7, 18, 18, // 0-15   (\t=7, \n=17, \r=7)
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, // 16-31
    // �������� �������
     7, 15, 18, 18, 19, 18, 18, 18,  8,  9,  5,  2, 18,  3, 20,  6, // 32-47  (' '=7, '!'=15, '$'=19 (EOF), '('=8, ')'=9, '*'=5, '+'=2, ','=18, '-'=3, '.'=20, '/'=6)
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 18, 16, 14,  4, 13, 18, // 48-63  ('0'-'9'=1, ':'=18, ';'=16, '<'=14, '='=4, '>'=13, '?'=18)
    18,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 64-79  ('@'=18, 'A'-'O'=0)
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 18, 11, 18, 18, // 80-95  ('P'-'Z'=0, '['=10, '\'=18, ']'=11, '^'=18, '_'=18)
    18,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 96-111 ('`'=18, 'a'-'o'=0)
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 18, 18, 18, 12, 18  // 112-127('p'-'z'=0, '{'=18, '|'=18, '}'=18, '~'=12, DEL=18)
};


Lexer::Lexer(const std::string& source, SymbolTable& symTable, ErrorHandler& errHandler)
    : sourceCode(source), currentPos(0), currentLine(1), lineStartPos(0),
    symbolTable(symTable), errorHandler(errHandler) {
}

char Lexer::peekChar(size_t offset) const {
    if (currentPos + offset < sourceCode.length()) {
        return sourceCode[currentPos + offset];
    }
    return '$'; // ����������� ������ ����� �����, ���� ����� �� �������
}

char Lexer::consumeChar() {
    if (currentPos >= sourceCode.length()) {
        return '$'; // ��� � �����
    }
    char consumed = sourceCode[currentPos];
    currentPos++;

    if (consumed == '\n') {
        currentLine++;
        lineStartPos = currentPos;
    }
    return consumed;
}

int Lexer::getCurrentColumn() const {
    return static_cast<int>(currentPos - lineStartPos) + 1;
}

Token Lexer::createFinalToken(TokenType type, const std::string& text) {
    // ��� �������������� ������� � EOF, ����� ����� ���� ������ ��� ������������� �� ����
    std::string tokenText = text;
    if (text.empty()) {
        if (type >= TokenType::T_ASSIGN && type <= TokenType::T_SEMICOLON) { // �������������� ���������
            tokenText = std::string(1, static_cast<char>(type));
        }
        else if (type == TokenType::T_EOF) {
            tokenText = "EOF";
        }
        // ��� �������� ���� ����� ����� ������� ���� ��� �������� ������
    }
    return Token(type, tokenText, currentLine, getCurrentColumn() - static_cast<int>(tokenText.length()));
}

Token Lexer::createIntToken(int value, const std::string& text) {
    return Token(TokenType::T_NUMBER_INT, value, currentLine, getCurrentColumn() - static_cast<int>(text.length()), text);
}

Token Lexer::createFloatToken(float value, const std::string& text) {
    return Token(TokenType::T_NUMBER_FLOAT, value, currentLine, getCurrentColumn() - static_cast<int>(text.length()), text);
}


Token Lexer::getNextToken() {
    LexerState currentState = LexerState::START; // ��������� ��������� �� (���������� 0 ��� �������)
    std::string currentLexemeText = "";
    // int currentNumberIntValue = 0; // �� �����, ������ �� currentLexemeText � �����
    // float currentNumberFloatValue = 0.0f;

    while (true) { // ���� ��������� ��������
        char currentChar = peekChar();
        int charCategory = 18; // 'other_printable' �� ���������

        if (currentChar == '$' && currentPos >= sourceCode.length()) { // ����� �������� ����� ������
            charCategory = 19; // '$' (EOF)
        }
        else if (currentChar >= 0 && static_cast<unsigned char>(currentChar) < 128) {
            charCategory = asciiCategoryTable[static_cast<unsigned char>(currentChar)];
        }
        else {
            // ������ ��� ��������� ASCII 0-127 (��������, UTF-8). ������� ��� "������"
            // ��� ����� ������ ������, ���� ���� ������������ ������ ASCII.
            // ��� ��������, ���� ������� "������".
            charCategory = 18;
        }

        int semanticAction = stateTransitionTable[static_cast<int>(currentState)][charCategory];

        switch (semanticAction) {
        case 0: // ������������ ������ (������, \t, \r), �������� � START
            consumeChar();
            currentState = LexerState::START;
            currentLexemeText = ""; // ����� �������
            break;
        case 1: // ������ ��������������
            currentLexemeText += consumeChar();
            currentState = LexerState::IDENTIFIER; // ������� � ��������� 1 (IDENTIFIER)
            break;
        case 2: // ������ ������ �����
            currentLexemeText += consumeChar();
            currentState = LexerState::NUMBER_INT; // ������� � ��������� 2 (NUMBER_INT)
            break;
        case 3: // ����� ����� ���� (������� � NUMBER_FLOAT_DECIMAL)
            currentLexemeText += consumeChar(); // '.'
            currentState = static_cast<LexerState>(3); // NUMBER_FLOAT_DECIMAL
            break;
        case 10: // ���������� �������������
            currentLexemeText += consumeChar();
            // currentState �������� LexerState::IDENTIFIER
            break;
        case 11: // ��������� �������������/�������� �����
            // ������ �� ����������, �� ������ ��������� �����
            if (auto kwType = symbolTable.getKeywordType(currentLexemeText); kwType.has_value()) {
                return createFinalToken(kwType.value(), currentLexemeText);
            }
            else {
                return createFinalToken(TokenType::T_IDENTIFIER, currentLexemeText);
            }
            // break; // �� �����, �.�. return
        case 20: // ���������� ����� �����
            currentLexemeText += consumeChar();
            // currentState �������� LexerState::NUMBER_INT
            break;
        case 21: // ��������� ����� �����
            // ������ �� ����������
            try {
                return createIntToken(std::stoi(currentLexemeText), currentLexemeText);
            }
            catch (const std::out_of_range& oor) {
                errorHandler.logLexicalError("Integer literal '" + currentLexemeText + "' is out of range.", currentLine, getCurrentColumn() - static_cast<int>(currentLexemeText.length()));
                return Token(TokenType::T_ERROR, currentLine, getCurrentColumn() - static_cast<int>(currentLexemeText.length()));
            }
            // break; // �� �����
        case 22: // ���������� ������� ����� float
            currentLexemeText += consumeChar();
            currentState = static_cast<LexerState>(3); // NUMBER_FLOAT_DECIMAL
            break;
        case 23: // ��������� float
            // ������ �� ����������
            // ��������, ���� ����� ����� �� ���� ���� (e.g. "12.")
            if (currentLexemeText.back() == '.') {
                errorHandler.logLexicalError("Malformed float literal '" + currentLexemeText + "'. Digit expected after decimal point.", currentLine, getCurrentColumn() - static_cast<int>(currentLexemeText.length()));
                return Token(TokenType::T_ERROR, currentLine, getCurrentColumn() - static_cast<int>(currentLexemeText.length()));
            }
            try {
                return createFloatToken(std::stof(currentLexemeText), currentLexemeText);
            }
            catch (const std::out_of_range& oor) {
                errorHandler.logLexicalError("Float literal '" + currentLexemeText + "' is out of range.", currentLine, getCurrentColumn() - static_cast<int>(currentLexemeText.length()));
                return Token(TokenType::T_ERROR, currentLine, getCurrentColumn() - static_cast<int>(currentLexemeText.length()));
            }
            // break; // �� �����

        // �������������� ������ (30-43)
        case 30: consumeChar(); return createFinalToken(TokenType::T_PLUS, "+");
        case 31: consumeChar(); return createFinalToken(TokenType::T_MINUS, "-");
        case 32: consumeChar(); return createFinalToken(TokenType::T_ASSIGN, "=");
        case 33: consumeChar(); return createFinalToken(TokenType::T_MULTIPLY, "*");
        case 34: consumeChar(); return createFinalToken(TokenType::T_DIVIDE, "/");
        case 35: consumeChar(); return createFinalToken(TokenType::T_LPAREN, "(");
        case 36: consumeChar(); return createFinalToken(TokenType::T_RPAREN, ")");
        case 37: consumeChar(); return createFinalToken(TokenType::T_LBRACKET, "[");
        case 38: consumeChar(); return createFinalToken(TokenType::T_RBRACKET, "]");
        case 39: consumeChar(); return createFinalToken(TokenType::T_EQUAL, "~");
        case 40: consumeChar(); return createFinalToken(TokenType::T_GREATER, ">");
        case 41: consumeChar(); return createFinalToken(TokenType::T_LESS, "<");
        case 42: consumeChar(); return createFinalToken(TokenType::T_NOT_EQUAL, "!");
        case 43: consumeChar(); return createFinalToken(TokenType::T_SEMICOLON, ";");
            // case 18 �� START (��������� \n) ���� ���������� consumeChar() � currentState = LexerState::START

        case 45: // ������ - ����������� ������ � ������� ���������
        {
            char errChar = consumeChar();
            std::string msg = "Unexpected character '";
            if (isprint(static_cast<unsigned char>(errChar))) msg += errChar;
            else msg += "\\" + std::to_string(static_cast<unsigned char>(errChar));
            msg += "'.";
            errorHandler.logLexicalError(msg, currentLine, getCurrentColumn() - 1);
            // ���������� ����� ������, ����� ������ ��� ��� ���������� ��� ����������
            // ������� � ������ ������ ��� ����� ����������.
            return Token(TokenType::T_ERROR, currentLine, getCurrentColumn() - 1); // -1 ������ ��� ������ ��� ������
        }
        case 46: // EOF
            return createFinalToken(TokenType::T_EOF);

        case 47: // ������ float � ����� (��������, ".5")
            currentLexemeText = "0"; // ��������� "0" ��� ����������� stof(".5") -> stof("0.5")
            currentLexemeText += consumeChar(); // ��������� ���� ����� '.'
            currentState = static_cast<LexerState>(3); // ������� � NUMBER_FLOAT_DECIMAL
            // ���� ����� ����� ����� �� �����, ��� ����� ���������� � ��������� NUMBER_FLOAT_DECIMAL (�������� 23)
            break;

        case 99: // ������������ ���������/������ � �������
            errorHandler.logLexicalError("Internal Lexer Error: Unreachable state or invalid action 99.", currentLine, getCurrentColumn());
            consumeChar(); // ������� ���������� ������
            return Token(TokenType::T_ERROR, currentLine, getCurrentColumn() - 1);


        default: // ����������� ������������� �������� - ���������� ������
            errorHandler.logLexicalError("Internal Lexer Error: Unknown semantic action " + std::to_string(semanticAction) + ".", currentLine, getCurrentColumn());
            consumeChar(); // ������� ���������� ������, ����� �������� ������������ �����
            return Token(TokenType::T_ERROR, currentLine, getCurrentColumn() - 1);
        }
    }
}