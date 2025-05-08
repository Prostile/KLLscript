#include "Lexer.h"
#include <iostream>
#include <cstring>
#include <cctype>    // ��� isprint, isdigit, isalpha
#include <cstdlib>   // ��� std::strtod
#include <sstream>   // ��� createErrorToken

// --- ����������� ����������� ������� ---

// ���������: 0:a-z(excl 'e'), 1:0-9, 2:'+', 3:'-', 4:'=', 5:'*', 6:'/', 7:space/cr/tab,
//            8:'(', 9:')', 10:'[', 11:']', 12:'~', 13:'>', 14:'<', 15:'!', 16:';',
//            17:'\n', 18:'.', 19:'e'/'E', 20:'&', 21:'|', 22:other/$
const int Lexer::asciiCategoryTable[128] = {
    //  0..31 Control characters (mostly 22, \t=7, \n=17, \r=7)
       22,22,22,22,22,22,22,22,22, 7,17,22,22, 7,22,22,
       22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,
       // 32..47 ' ' ! " # $ % & ' ( ) * + , - . /
           7,15,22,22,22/*$*/,22,20/*&*/,22, 8, 9, 5, 2,22, 3,18, 6,
           // 48..63 0..9 : ; < = > ?
               1, 1, 1, 1, 1, 1, 1, 1, 1, 1,22,16,14, 4,13,22,
               // 64..79 @ A..D E F..O
                  22, 0, 0, 0, 0,19/*E*/,0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                  // 80..95 P..Z [ \ ] ^ _
                      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,10,22,11,22, 0,
                      // 96..111 ` a..d e f..o
                         22, 0, 0, 0, 0,19/*e*/,0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         //112..127 p..z { | } ~ DEL
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,22,21/*|*/,22,12,22
};


// ������� ��������� ��
// �������: ... 19:e/E, 20:'&', 21:'|', 22:other/$
// ���������: START(0)..EXP_DIGIT(7), AND1(8), OR1(9)
// ��������� ���������, �� ������� �������� 10x23 (������� 0-22)
// ������: 99 - Error state
const int Lexer::stateTransitionTable[10][23] = { // ��������� ������ �� 23 ��������
    //    a-z 0-9  +  -  =  *  / sp  (  )  [  ]  ~  >  <  !  ; \n  . e/E  &  | oth/$
    //     0   1   2  3  4  5  6  7  8  9 10 11 12 13 14 15 16  17 18  19 20 21   22
    /*S 0*/{ 1,  2,100 + '+',100 + '-',100 + '=',100 + '*',100 + '/', 8,100 + '(',100 + ')',100 + '[',100 + ']',100 + '~',100 + '>',100 + '<',100 + '!',100 + ';', 18,  3,  1, 30, 31, 99},// 30:������� &, 31:������� |
    /*ID 1*/{ 1,  1,122,122,122,122,122,122,122,122,122,122,122,122,122,122,122,123,122,  1,122,122,122},// '.' ��������� ID
    /*INT 2*/{99,  2,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,129,  3,  4,128,128,128},
    /*DOT 3*/{99,  4, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99},
    /*FRA 4*/{99,  4,130,130,130,130,130,130,130,130,130,130,130,130,130,130,130,131, 99,  5,130,130,130},
    /*EXP 5*/{99,  7, 99, 6, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99},
    /*SGN 6*/{99,  7, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99},
    /*DIG 7*/{99,  7,130,130,130,130,130,130,130,130,130,130,130,130,130,130,130,131, 99, 99,130,130,130},
    /*AND1 8*/{99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 32, 99, 99},// 32: ������� ������ '&'
    /*OR1 9*/{99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 33, 99}, // 33: ������� ������ '|'
};
// ����� ������������� ���������:
// 30: ������� ������ '&', ������� � ��������� AND1(8)
// 31: ������� ������ '|', ������� � ��������� OR1(9)
// 32: ������� ������ '&', �������� ��������� -> ������� T_OP_AND
// 33: ������� ������ '|', �������� ��������� -> ������� T_OP_OR

// ������������� ��������� (��������):
// <100: ���������� ��������
// 1: ������ �������������
// 2: ������ ����� �����
// 3: ������� ����� (�������� float)
// 4: ����� ����� ����� (��������� FRAC)
// 5: ������� 'e'/'E' (�������� ����������)
// 6: ������� ���� +/- ����� 'e'/'E'
// 7: ����� ����� 'e'/'E' ��� ����� �����
// 8: ������� �������/���������/CR
// 18: ������� �������� ������ LF
// 21: ���������� �������������
// 27: ���������� ����� �����
// >=100: �������� ���������, ������� ����� (��� = �������� - 100)
// 122: ����� �������������� (�� �����/�����/e)
// 123: ����� �������������� (����� \n)
// 128: ����� ������ ����� (�� �����, �� '.', �� 'e')
// 129: ����� ������ ����� (����� \n)
// 130: ����� float (����� ������� ����� ��� ����������)
// 131: ����� float (����� \n)
// 99: ������

// --- ����������� ---
Lexer::Lexer(const std::string& source, SymbolTable& symTable)
    : sourceCode(source), currentPos(0), currentLine(1), lineStartPos(0),
    symbolTable(symTable) {
}

// --- ��������������� ������ --- (��� ���������)
char Lexer::peekNextChar() const { /* ... */ }
char Lexer::consumeChar() { /* ... */ }
int Lexer::getCurrentColumn() const { /* ... */ }

// --- �������� ������� --- (��������� float, bool)
Token Lexer::createToken(TokenType type) { /* ... */ }
Token Lexer::createToken(TokenType type, const std::string& text) { /* ... */ }
Token Lexer::createToken(TokenType type, int value) { /* ... */ }
Token Lexer::createToken(TokenType type, double value) { // �����
    return Token(type, value, currentLine, getCurrentColumn()); // ������� ���������
}
Token Lexer::createToken(TokenType type, bool value) { // �����
    return Token(type, value, currentLine, getCurrentColumn()); // ������� ���������
}

Token Lexer::createErrorToken(const std::string& message) {
    char badChar = '?';
    if (currentPos < sourceCode.length()) {
        badChar = sourceCode[currentPos];
    }
    else if (currentPos > 0 && currentPos == sourceCode.length()) {
        badChar = sourceCode[currentPos - 1];
    }
    std::stringstream ss;
    ss << message << " (char: '"
        << (isprint(static_cast<unsigned char>(badChar)) ? std::string(1, badChar) : "\\?") << "', "
        << "ASCII: " << static_cast<int>(static_cast<unsigned char>(badChar)) << ")";
    std::cerr << "Lexical Error (Line " << currentLine << ", Col " << getCurrentColumn() << "): "
        << ss.str() << std::endl;
    return Token(TokenType::T_ERROR, currentLine, getCurrentColumn());
}


// --- ��������� ���������������/�������� ���� ---
Token Lexer::handleIdentifierOrKeyword(std::string& lexeme) {
    // ��������� ������� �� bool ��������
    if (lexeme == "true") {
        return createToken(TokenType::T_KW_TRUE, true);
    }
    if (lexeme == "false") {
        return createToken(TokenType::T_KW_FALSE, false);
    }
    // ����� �� ������ �������� �����
    if (auto kwType = symbolTable.getKeywordType(lexeme); kwType.has_value()) {
        return createToken(kwType.value(), lexeme);
    }
    else {
        return createToken(TokenType::T_IDENTIFIER, lexeme);
    }
}

// --- �������� �����: �������� ��������� ����� ---
Token Lexer::getNextToken() {
    LexerState currentState = LexerState::START;
    std::string currentLexeme = "";

    while (true) {
        char currentChar = peekNextChar();
        int category = 22; // other �� ���������

        if (currentPos >= sourceCode.length()) {
            category = 22; // ���������� ��������� 'other' ��� EOF
        }
        else if (currentChar >= 0 && currentChar < 128) {
            category = asciiCategoryTable[static_cast<unsigned char>(currentChar)];
        }

        int action = 99; // ������ �� ���������
        if (static_cast<int>(currentState) < 10) {
            // ��������� ������� ���������
            if (category < 0 || category >= 23) { // ���������� 23 ��� ������
                category = 22; // ���� ��������� �����������, ������� 'other'
            }
            action = stateTransitionTable[static_cast<int>(currentState)][category];
        }

        // --- ��������� EOF � ��������� START ---
        if (currentState == LexerState::START && currentPos >= sourceCode.length()) {
            return createToken(TokenType::T_EOF); // ���� ���������� EOF
        }
        // --- ����� ��������� EOF ---


        if (action == 99) {
            // ���� �� � ��������� AND1 ��� OR1 � ��������� �� ��, ��� ������
            if (currentState == LexerState::OP_AND_1 || currentState == LexerState::OP_OR_1) {
                consumeChar(); // ���������� ������, ��������� ������
                return createErrorToken("Incomplete operator: expected '&' or '|'");
            }
            // ����� - ����� ������
            consumeChar();
            return createErrorToken("Invalid character or state");
        }

        // ��������� �������� ��������� (>= 100)
        if (action >= 100) {
            TokenType finalType = static_cast<TokenType>(action - 100);
            // ... (��������� T_IDENTIFIER, T_NUMBER_INT, T_NUMBER_FLOAT ��� ������) ...
            // ... (���������� std::stoi � std::strtod � try-catch) ...
            if (finalType == TokenType::T_IDENTIFIER) { // ����� �������������� (122)
                return handleIdentifierOrKeyword(currentLexeme);
            }
            else if (finalType == TokenType::T_NUMBER_INT) { // ����� ������ (128)
                try {
                    size_t processedChars = 0;
                    int val = std::stoi(currentLexeme, &processedChars);
                    // �������������� ��������, ��� ��� ������ ���� ������ (�� ������ ������ � ��)
                    if (processedChars != currentLexeme.length()) {
                        return createErrorToken("Invalid integer literal format (stray characters)");
                    }
                    return createToken(TokenType::T_NUMBER_INT, val);
                }
                catch (const std::invalid_argument&) {
                    return createErrorToken("Invalid integer literal format");
                }
                catch (const std::out_of_range&) {
                    return createErrorToken("Integer literal out of range");
                }
            }
            else if (finalType == TokenType::T_NUMBER_FLOAT) { // ����� float (130)
                try {
                    char* endPtr;
                    double val = std::strtod(currentLexeme.c_str(), &endPtr);
                    // ���������, ��� ��� ������ ���� ���������
                    if (endPtr != currentLexeme.c_str() + currentLexeme.length()) {
                        return createErrorToken("Invalid float literal format (stray characters)");
                    }
                    // �������� �� INF / NAN (strtod �� ����������, �� ��� ��� �� �����)
                    if (!std::isfinite(val)) { // ����� #include <cmath>
                        return createErrorToken("Floating point literal results in infinity or NaN");
                    }
                    return createToken(TokenType::T_NUMBER_FLOAT, val);
                }
                catch (...) { // ����� ������ ��������� ��������
                    return createErrorToken("Invalid float literal format");
                }
            }
            else { // �������������� ������
                consumeChar();
                return createToken(finalType);
            }
        }

        // ��������� ������������� �������� (< 100)
        switch (action) {
            // ... (case 1-7, 8, 18, 21, 27 ��� ������) ...
        case 1: // ������ �������������
            currentState = LexerState::IDENTIFIER;
            currentLexeme += consumeChar();
            break;
        case 2: // ������ ����� �����
            currentState = LexerState::NUMBER_INT;
            currentLexeme += consumeChar();
            break;
        case 3: // ������� �����
            currentState = LexerState::NUMBER_FLOAT_DOT;
            currentLexeme += consumeChar();
            break;
        case 4: // ����� ����� ����� -> FRAC
            currentState = LexerState::NUMBER_FLOAT_FRAC;
            currentLexeme += consumeChar();
            break;
        case 5: // ������� 'e'/'E' -> EXP
            currentState = LexerState::NUMBER_FLOAT_EXP;
            currentLexeme += consumeChar();
            break;
        case 6: // ���� +/- ����� 'e'/'E' -> EXP_SIGN
            currentState = LexerState::NUMBER_FLOAT_EXP_SIGN;
            currentLexeme += consumeChar();
            break;
        case 7: // ����� ����� 'e', ����� ��� ������ ����� � ���������� -> EXP_DIGIT
            currentState = LexerState::NUMBER_FLOAT_EXP_DIGIT;
            currentLexeme += consumeChar();
            break;
        case 8: // ������� �������/���������/CR
            consumeChar();
            // currentState = LexerState::START; // �� ������ ���������! �������� � START
            break;
        case 18: // ������� �������� ������ LF
            consumeChar();
            // currentState = LexerState::START; // �� ������ ���������! �������� � START
            break;
        case 21: // ���������� �������������
            currentLexeme += consumeChar();
            break;
        case 27: // ���������� ����� �����
            currentLexeme += consumeChar();
            break;

            // --- ����� �������� ��� && � || ---
        case 30: // ������� ������ '&'
            currentState = LexerState::OP_AND_1;
            consumeChar(); // ���������� ������ '&'
            // �� ��������� � lexeme, �.�. ��� ����� ��� ����� &&
            break;
        case 31: // ������� ������ '|'
            currentState = LexerState::OP_OR_1;
            consumeChar(); // ���������� ������ '|'
            break;
        case 32: // ������� ������ '&'
            consumeChar(); // ���������� ������ '&'
            return createToken(TokenType::T_OP_AND); // ���������� ����� &&
        case 33: // ������� ������ '|'
            consumeChar(); // ���������� ������ '|'
            return createToken(TokenType::T_OP_OR); // ���������� ����� ||

            // ��������� ���������� �� \n (�������� 123, 129, 131)
        case 123: // ����� �������������� ����� \n
            return handleIdentifierOrKeyword(currentLexeme);
        case 129: // ����� ������ ����� \n
            try { /* ... stoi ... */ }
            catch (...) { /* ... */ }
        case 131: // ����� float ����� \n
            try { /* ... strtod ... */ }
            catch (...) { /* ... */ }

        default:
            consumeChar();
            return createErrorToken("Internal Lexer Error - Unexpected action: " + std::to_string(action));
        }
    }
    // ���� �� ������ �����
    return createErrorToken("Internal Lexer Error - Lexer loop exited unexpectedly");
}