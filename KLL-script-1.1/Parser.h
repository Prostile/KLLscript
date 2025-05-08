#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <stack>
#include <optional> // Для возврата типа выражения

#include "definitions.h"
#include "DataStructures.h"
#include "Lexer.h"
#include "SymbolTable.h"

class Parser {
private:
    Lexer& lexer;
    SymbolTable& symbolTable;

    Token currentToken;
    bool declarationPhase; // Остается для разделения фаз

    std::vector<RPNOperation> rpnCode;
    std::stack<int> labelStack;

    // --- Вспомогательные методы ---
    void nextToken();
    bool match(TokenType expectedType);
    void syntaxError(const std::string& message);
    void semanticError(const std::string& message); // Для ошибок типизации и т.п.

    // Генерация ОПС
    void emit(RPNOpCode opCode);
    void emit(RPNOpCode opCode, const SymbolValue& value);
    void emit(RPNOpCode opCode, int symbolIndex);
    int emitPlaceholder(RPNOpCode jumpType);
    void patchJump(int placeholderIndex);
    void patchJumpTo(int placeholderIndex, int targetAddress);
    int getCurrentRPNAddress() const;
    // Генерация приведения типов
    void emitCast(SymbolType fromType, SymbolType toType);


    // --- Методы рекурсивного спуска ---

    void parseProgram();
    void parseDeclarationList();
    void parseDeclaration();
    // Вспомогательные для объявлений
    SymbolType parseTypeSpecifier(); // Распознает int, float, bool
    void parseVariableDeclaration(SymbolType baseType);
    void parseArrayDeclaration(SymbolType baseType);

    void parseStatementList();
    void parseStatement();

    // Операторы
    void parseAssignment(const std::string& identifierName, int line, int col);
    // <ArrayIndexOpt> теперь возвращает тип индекса (должен быть int)
    std::optional<SymbolType> parseArrayIndexOpt(int identifierIndex, SymbolType baseType); // baseType - тип массива

    void parseIfStatement();
    void parseElseClauseOpt(int jumpAfterThenPlaceholder);
    void parseWhileStatement();
    void parseCinStatement();
    void parseCoutStatement();

    // Выражения и условия
    // Методы разбора выражений теперь возвращают тип результата выражения
    std::optional<SymbolType> parseCondition(); // Условие должно быть bool (или int)
    std::optional<SymbolType> parseExpression();
    std::optional<SymbolType> parseLogicalOrExpr();
    std::optional<SymbolType> parseLogicalOrExprPrime(SymbolType leftType);
    std::optional<SymbolType> parseLogicalAndExpr();
    std::optional<SymbolType> parseLogicalAndExprPrime(SymbolType leftType);
    std::optional<SymbolType> parseEqualityExpr();
    std::optional<SymbolType> parseEqualityExprPrime(SymbolType leftType);
    std::optional<SymbolType> parseRelationalExpr();
    std::optional<SymbolType> parseRelationalExprPrime(SymbolType leftType);
    std::optional<SymbolType> parseAdditiveExpr();
    std::optional<SymbolType> parseAdditiveExprPrime(SymbolType leftType);
    std::optional<SymbolType> parseMultiplicativeExpr();
    std::optional<SymbolType> parseMultiplicativeExprPrime(SymbolType leftType);
    std::optional<SymbolType> parseUnaryExpr();
    std::optional<SymbolType> parsePrimaryExpr();
    std::optional<SymbolType> parseIdentifierOrArrayAccess(); // Обрабатывает a или a[i]

    // Вспомогательные для проверки и приведения типов выражений
    SymbolType promoteTypes(SymbolType type1, SymbolType type2, RPNOpCode& opCodeInt, RPNOpCode& opCodeFloat);
    SymbolType checkLogicalOp(SymbolType left, SymbolType right, const std::string& op);
    SymbolType checkComparisonOp(SymbolType left, SymbolType right, const std::string& op);
    SymbolType checkEqualityOp(SymbolType left, SymbolType right, const std::string& op);
    bool ensureBooleanOrInt(const std::optional<SymbolType>& type, const std::string& context);


public:
    Parser(Lexer& lex, SymbolTable& symTab);
    bool parse();
    const std::vector<RPNOperation>& getRPNCode() const;
    void printRPN() const; // Обновим для новых кодов ОПС
};

#endif // PARSER_H