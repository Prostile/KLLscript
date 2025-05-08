#include "Lexer.h"
#include <iostream> // ��� ������� ��� ��������� ������
#include <cstring> // ��� strcmp, ���� ����������� (���� ����� ������������ std::string)
#include <cctype> // ��� isprint
#include <sstream> // ��� �������������� ������ � ����� �������

// --- ������������� ����������� ������ ---

// ������� ������������� �������� (������� ��� ������ ��� ���������, � ������� ��� ��� ������������ �������)
// ������� ���������: 0 - START, 1 - IDENTIFIER, 2 - NUMBER
// ������� ��������� ��������: ��. asciiCategoryTable � ����������� � SemTable � ������������ ����
const int Lexer::stateTransitionTable[3][20] = {
    //		     a-z 0-9  +   -   =   *   /  sp   (   )   [   ]   ~   >   <   !   ;  \n err   $
    //		     0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19
    /*START*/ {  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20},
    /*IDENT*/ { 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 24, 22}, // ���� 21 ��� ����, �� ������������� ����� ��������� �����
    /*NUMBER*/{ 24, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 19, 28}  // ������ ����� ����� ����� (������ 24). ������/������� � �.�. ��������� ����� (28)
};

// ������� �������������� ASCII ����� � ��������� �������� (������������� AscTable �� ���������)
// 0:'a'-'z', 1:'0'-'9', 2:'+', 3:'-', 4:'=', 5:'*', 6:'/', 7:' ', 8:'(', 9:')', 10:'[', 11:']',
// 12:'~', 13:'>', 14:'<', 15:'!', 16:';', 17:'\n', 18:other, 19:'$' (EOF)
const int Lexer::asciiCategoryTable[128] = {
    18, 18, 18, 18, 18, 18, 18, 18, 18,  7, 17, 18, 18,  7, 18, 18, // 0-15
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, // 16-31
     7, 15, 18, 18, 19, 18, 18, 18,  8,  9,  5,  2, 18,  3, 18,  6, // 32-47 ' ' ! ... /
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 18, 16, 14,  4, 13, 18, // 48-63 '0'-'9' : ; < = > ?
    18,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 64-79 '@' 'A'-'O'
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 18, 11, 18, 18, // 80-95 'P'-'Z' [ \ ] ^ _
    18,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 96-111 '`' 'a'-'o'
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 18, 18, 18, 12, 18  // 112-127 'p'-'z' { | } ~ DEL
};

// --- ����������� ---
Lexer::Lexer(const std::string& source, SymbolTable& symTable /*, ErrorHandler& errHandler*/)
    : sourceCode(source), currentPos(0), currentLine(1), lineStartPos(0),
    symbolTable(symTable) /*, errorHandler(errHandler)*/ {
}

// --- ��������������� ������ ---
char Lexer::peekNextChar() const {
    if (currentPos < sourceCode.length()) {
        return sourceCode[currentPos];
    }
    return '$'; // ����������� ������ ����� ����� (��� '\0')
}

char Lexer::consumeChar() {
    if (currentPos >= sourceCode.length()) {
        return '$'; // ��� � �����
    }
    char consumed = sourceCode[currentPos];
    currentPos++;

    if (consumed == '\n') {
        currentLine++;
        lineStartPos = currentPos; // ���������� ������ ����� ������
    }
    return consumed;
}

int Lexer::getCurrentColumn() const {
    // +1, ��� ��� ������� ������ ���������� � 1
    return static_cast<int>(currentPos - lineStartPos) + 1;
}

// --- �������� ������� ---
Token Lexer::createToken(TokenType type) {
    return Token(type, currentLine, getCurrentColumn() - 1); // -1 �.�. ������ ��� ������
}
Token Lexer::createToken(TokenType type, const std::string& text) {
    return Token(type, text, currentLine, getCurrentColumn() - text.length());
}
Token Lexer::createToken(TokenType type, int value) {
    // ����� ����� ����������, ������� ����� ���������
    return Token(type, value, currentLine, getCurrentColumn());
}
Token Lexer::createErrorToken(const std::string& message) {
    // TODO: ������������ ErrorHandler
    char badChar = '?'; // ������ �� ���������
    if (currentPos < sourceCode.length()) { // ��������, ��� �� ����� �� �������
        badChar = sourceCode[currentPos]; // ����� ������ �� ������� ������� (�� consumeChar � case)
    }
    else if (currentPos > 0 && currentPos == sourceCode.length()) {
        badChar = sourceCode[currentPos - 1]; // ���� ������ �� ����� �����, ������� ��������� ������
    }


    std::stringstream ss;
    ss << message << " (char: '"
        << (isprint(static_cast<unsigned char>(badChar)) ? std::string(1, badChar) : "\\?") << "', "
        << "ASCII: " << static_cast<int>(static_cast<unsigned char>(badChar)) << ")";

    std::cerr << "Lexical Error (Line " << currentLine << ", Col " << getCurrentColumn() << "): "
        << ss.str() << std::endl;

    // ���������� ����� ������, �� ������� ��� �������� � ���������� ���� (case 19, 24)
    return Token(TokenType::T_ERROR, currentLine, getCurrentColumn());
}

// --- ��������� ������������� �������� ---
Token Lexer::handleIdentifierOrKeyword(std::string& lexeme) {
    // ���������, �� �������� �� ��� �����
    if (auto kwType = symbolTable.getKeywordType(lexeme); kwType.has_value()) {
        return createToken(kwType.value(), lexeme);
    }
    else {
        // ��� �������������
        return createToken(TokenType::T_IDENTIFIER, lexeme);
    }
}

Token Lexer::handleNumber(int& value, char digit) {
    value = value * 10 + (digit - '0');
    // ���������� nullptr, ��� ��� ����� ��� �� �����
    // ����� ����� ������, ����� ���������� ��-�����
    return Token(); // ���������� ������ ����� ��� �����������
}


// --- �������� �����: �������� ��������� ����� ---
Token Lexer::getNextToken() {
    LexerState currentState = LexerState::START;
    std::string currentLexeme = "";
    int currentNumberValue = 0;
    char currentChar = 0;

    // --- �������-- -
    char firstChar = peekNextChar();
    std::cout << "[DEBUG Lexer] Starting token parse at Line " << currentLine
        << ", Col " << getCurrentColumn()
        << ", Char: '" << (isprint(static_cast<unsigned char>(firstChar)) ? std::string(1, firstChar) : "\\?")
        << "' (ASCII: " << static_cast<int>(static_cast<unsigned char>(firstChar)) << ")" << std::endl;
    // --- ����� ������� ---*/

    while (currentState != LexerState::FINAL && currentState != LexerState::ERROR) {
        currentChar = peekNextChar(); // ������� ��������� ������

        // ���������� ��������� �������
        int category = 18; // 'other' �� ���������
        if (currentChar >= 0 && currentChar < 128) {
            category = asciiCategoryTable[static_cast<unsigned char>(currentChar)];
            // ������ ��������� ����� �����
            if (currentPos >= sourceCode.length()) {
                category = 19; // '$' ��������� EOF
            }
        }
        else if (currentPos >= sourceCode.length()) {
            category = 19; // '$' ��������� EOF
        }


        // �������� ����� �������������� �������� �� ������� ���������
        int semanticAction = stateTransitionTable[static_cast<int>(currentState)][category];

        // ��������� ��������
        switch (semanticAction) {
        case 1: // ������ ��������������
            currentState = LexerState::IDENTIFIER;
            currentLexeme += consumeChar();
            break;
        case 2: // ������ �����
            currentState = LexerState::NUMBER;
            currentNumberValue = (consumeChar() - '0');
            break;
        case 3: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_PLUS);
        case 4: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_MINUS);
        case 5: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_ASSIGN);
        case 6: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_MULTIPLY);
        case 7: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_DIVIDE);
        case 8: // ������ - ���������� � �������� � START
            consumeChar();
            currentState = LexerState::START; // �������� � ��������� ���������
            break;
        case 9: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_LPAREN);
        case 10: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_RPAREN);
        case 11: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_LBRACKET);
        case 12: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_RBRACKET);
        case 13: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_EQUAL);
        case 14: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_GREATER);
        case 15: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_LESS);
        case 16: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_NOT_EQUAL);
        case 17: currentState = LexerState::FINAL; consumeChar(); return createToken(TokenType::T_SEMICOLON);
        case 18: // ������� ������ - ���������� � �������� � START
            consumeChar(); // consumeChar() ��� ���������� currentLine++
            currentState = LexerState::START;
            break;
        case 19: // ������ (������������ ������ � ������ ���������)
        {
            std::string base_msg = "Unexpected character";
            consumeChar(); // ���������� ��������� ������
            return createErrorToken(base_msg);
        }

        case 20: // ����� ����� ($)
            currentState = LexerState::FINAL;
            // consumeChar(); // �� �����, �� ��� � �����
            return createToken(TokenType::T_EOF);

        case 21: // ����������� �������������� (����� ��� �����)
            currentState = LexerState::IDENTIFIER; // �������� � ��������� ��������������
            currentLexeme += consumeChar();
            break;

        case 22: // ����� �������������� (��������� �� �����/�����)
            currentState = LexerState::FINAL;
            // ������ �� ����������, �� ������ ��������� �����
            return handleIdentifierOrKeyword(currentLexeme);

        case 23: // ����� �������������� � ������� ������
            currentState = LexerState::FINAL;
            // ������ �� ����������, ������� ������ ������������ �� ����. ��������
            return handleIdentifierOrKeyword(currentLexeme);

        case 24: // ������ � �������������� ��� ����� (��������, ����� ����� �����)
        {
            std::string base_msg = "Invalid character in identifier or number";
            consumeChar(); // ���������� ��������� ������
            return createErrorToken(base_msg);
        }

        case 27: // ����������� �����
            currentState = LexerState::NUMBER; // �������� � ��������� �����
            handleNumber(currentNumberValue, consumeChar()); // ��������� �����
            break;

        case 28: // ����� ����� (��������� �� �����)
            currentState = LexerState::FINAL;
            // ������ �� ����������
            return createToken(TokenType::T_NUMBER_INT, currentNumberValue);

        case 29: // ����� ����� � ������� ������
            currentState = LexerState::FINAL;
            // ������ �� ����������
            return createToken(TokenType::T_NUMBER_INT, currentNumberValue);

        default: // ����������� ������������� �������� - ���������� ������
            currentState = LexerState::ERROR;
            return createErrorToken("Internal Lexer Error - Unknown semantic action: " + std::to_string(semanticAction));
        }
    }

    // ���� ���� ���������� �� ����� return (������������)
    return createErrorToken("Internal Lexer Error - Unexpected loop exit");
}