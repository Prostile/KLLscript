// parser.cpp
#include "parser.h"
#include <iostream> 
#include <iomanip>  
#include <algorithm> // Не используется активно, но может пригодиться

// --- Конструктор ---
Parser::Parser(Lexer& lex, SymbolTable& symTab, ErrorHandler& errHandler)
    : lexer(lex), symbolTable(symTab), errorHandler(errHandler),
    declarationContextActive(true), lastDeclaredType(SymbolType::VARIABLE_INT)
{
    nextToken();
}

// --- Вспомогательные методы ---
void Parser::nextToken() {
    currentToken = lexer.getNextToken();
    // Пропускаем T_ERROR токены, так как лексер уже должен был сообщить об ошибке через errorHandler
    // и вернуть T_ERROR. Если мы здесь зациклимся, значит лексер не возвращает T_EOF при ошибках.
    while (currentToken.type == TokenType::T_ERROR && currentToken.type != TokenType::T_EOF) {
        // errorHandler уже должен был зарегистрировать ошибку
        currentToken = lexer.getNextToken();
    }
}

bool Parser::match(TokenType expectedType) {
    if (currentToken.type == expectedType) {
        nextToken();
        return true;
    }
    return false;
}

void Parser::consume(TokenType expectedType) {
    if (currentToken.type == expectedType) {
        nextToken();
    }
    else {
        std::string expectedTokenText = "token of type " + std::to_string(static_cast<int>(expectedType));
        // Можно будет улучшить, добавив маппинг TokenType -> string
        reportSyntaxError("Expected " + expectedTokenText + " but found '" + currentToken.text +
            "' (type " + std::to_string(static_cast<int>(currentToken.type)) + ")");
        // После серьезной синтаксической ошибки, возможно, стоит остановить парсинг
        // или применить более сложную стратегию восстановления.
        // Пока что мы просто регистрируем ошибку.
    }
}

void Parser::reportSyntaxError(const std::string& message) {
    errorHandler.logSyntaxError(message, currentToken.line, currentToken.column);
}

void Parser::reportSemanticError(const std::string& message, int line, int col) {
    int errorLine = (line == 0 && currentToken.line != 0) ? currentToken.line : line; // Используем currentToken если line=0 и он валиден
    int errorCol = (col == 0 && currentToken.column != 0) ? currentToken.column : col; // Аналогично для столбца
    errorHandler.logSemanticError(message, errorLine, errorCol);
}


// --- Генерация ОПС ---
void Parser::emit(RPNOpCode opCode) {
    rpnCode.emplace_back(opCode);
}

void Parser::emit(RPNOpCode opCode, int value) {
    rpnCode.emplace_back(opCode, value);
}

void Parser::emit(RPNOpCode opCode, float value) {
    rpnCode.emplace_back(opCode, value);
}

void Parser::emit(RPNOpCode opCode, size_t symbolIndex) {
    rpnCode.emplace_back(opCode, symbolIndex);
}

void Parser::emit(RPNOpCode opCode, bool isIoPlaceholder) {
    rpnCode.emplace_back(opCode, isIoPlaceholder);
}

int Parser::emitJumpPlaceholder(RPNOpCode jumpOpCode) {
    rpnCode.emplace_back(jumpOpCode, -1, true); // Используем конструктор RPNOperation(RPNOpCode, int, bool)
    return static_cast<int>(rpnCode.size()) - 1;
}

void Parser::patchJump(int rpnAddressToPatch, int targetRpnAddress) {
    if (rpnAddressToPatch < 0 || static_cast<size_t>(rpnAddressToPatch) >= rpnCode.size()) {
        reportSemanticError("Internal Parser Error: Invalid RPN address (" + std::to_string(rpnAddressToPatch) + ") for jump patching.");
        return;
    }
    // Проверяем, что это действительно операция перехода
    if (rpnCode[rpnAddressToPatch].opCode != RPNOpCode::JUMP && rpnCode[rpnAddressToPatch].opCode != RPNOpCode::JUMP_FALSE) {
        reportSemanticError("Internal Parser Error: Attempting to patch non-jump instruction at address " + std::to_string(rpnAddressToPatch) + ".");
        return;
    }
    rpnCode[rpnAddressToPatch].jumpTarget = targetRpnAddress;
}

int Parser::getCurrentRPNAddress() const {
    return static_cast<int>(rpnCode.size());
}

// --- Основной метод парсинга ---
bool Parser::parse() {
    declarationContextActive = true;
    parseProgram();
    // Проверяем, что после конца программы нет "мусора", только если не было других ошибок
    if (currentToken.type != TokenType::T_EOF && !errorHandler.hasErrors()) {
        reportSyntaxError("Unexpected tokens found after end of program.");
    }
    return !errorHandler.hasErrors();
}

// P → <OptDeclarationList> begin A end EOF
void Parser::parseProgram() {
    parseOptDeclarationList();
    declarationContextActive = false;

    consume(TokenType::T_KW_BEGIN);
    parseStatementList(); // A
    consume(TokenType::T_KW_END);
    // EOF проверяется в parse()
}

// <OptDeclarationList> → <Declaration> ; <OptDeclarationList> | λ
void Parser::parseOptDeclarationList() {
    while (currentToken.type == TokenType::T_KW_INT ||
        currentToken.type == TokenType::T_KW_FLOAT ||
        currentToken.type == TokenType::T_KW_ARR) {
        if (errorHandler.hasErrors()) return; // Прерываем, если уже есть ошибки
        parseDeclaration();
        if (errorHandler.hasErrors()) return;
        consume(TokenType::T_SEMICOLON);
    }
}

// <Declaration> → int L | float L | arr <ArrayTypeSpec> M
void Parser::parseDeclaration() {
    if (currentToken.type == TokenType::T_KW_INT) {
        nextToken(); // съели 'int'
        parseVarList(SymbolType::VARIABLE_INT);
    }
    else if (currentToken.type == TokenType::T_KW_FLOAT) {
        nextToken(); // съели 'float'
        parseVarList(SymbolType::VARIABLE_FLOAT);
    }
    else if (currentToken.type == TokenType::T_KW_ARR) {
        nextToken(); // съели 'arr'
        SymbolType elementType = parseArrayTypeSpec();
        SymbolType arrayAggregateType = (elementType == SymbolType::VARIABLE_INT) ? SymbolType::ARRAY_INT : SymbolType::ARRAY_FLOAT;
        parseArrList(arrayAggregateType);
    }
    else {
        reportSyntaxError("Expected 'int', 'float', or 'arr' for declaration.");
    }
}

// <ArrayTypeSpec> -> int | float (для типа элементов массива)
SymbolType Parser::parseArrayTypeSpec() {
    if (currentToken.type == TokenType::T_KW_INT) {
        nextToken();
        lastDeclaredType = SymbolType::ARRAY_INT; // Запоминаем полный тип массива
        return SymbolType::VARIABLE_INT; // Возвращаем тип ЭЛЕМЕНТА
    }
    else if (currentToken.type == TokenType::T_KW_FLOAT) {
        nextToken();
        lastDeclaredType = SymbolType::ARRAY_FLOAT; // Запоминаем полный тип массива
        return SymbolType::VARIABLE_FLOAT; // Возвращаем тип ЭЛЕМЕНТА
    }
    else {
        reportSyntaxError("Expected array element type specifier 'int' or 'float' after 'arr'.");
        lastDeclaredType = SymbolType::ARRAY_INT;
        return SymbolType::VARIABLE_INT;
    }
}

// L → a <VarListTail>
void Parser::parseVarList(SymbolType varType) {
    if (currentToken.type == TokenType::T_IDENTIFIER) {
        Token idToken = currentToken;
        nextToken();
        symbolTable.addVariable(idToken.text, varType, idToken.line);
        parseVarListTail(varType);
    }
    else {
        reportSyntaxError("Expected identifier in variable list.");
    }
}

// <VarListTail> → , a <VarListTail> | λ
void Parser::parseVarListTail(SymbolType varType) {
    while (match(TokenType::T_COMMA)) {
        if (errorHandler.hasErrors()) return;
        if (currentToken.type == TokenType::T_IDENTIFIER) {
            Token idToken = currentToken;
            nextToken();
            symbolTable.addVariable(idToken.text, varType, idToken.line);
        }
        else {
            reportSyntaxError("Expected identifier after comma in variable list.");
            break;
        }
    }
}

// M → a [ k ] <ArrListTail>
void Parser::parseArrList(SymbolType arrAggregateType) {
    if (currentToken.type == TokenType::T_IDENTIFIER) {
        Token idToken = currentToken;
        nextToken();
        consume(TokenType::T_LBRACKET);
        if (errorHandler.hasErrors()) return;
        if (currentToken.type == TokenType::T_NUMBER_INT) {
            int arraySize = currentToken.getIntValue();
            if (arraySize <= 0) {
                reportSemanticError("Array size must be a positive integer.", currentToken.line, currentToken.column);
            }
            nextToken();
            symbolTable.addArray(idToken.text, arrAggregateType, idToken.line, static_cast<size_t>(arraySize > 0 ? arraySize : 1));
        }
        else {
            reportSyntaxError("Expected integer number for array size.");
        }
        if (errorHandler.hasErrors()) return;
        consume(TokenType::T_RBRACKET);
        parseArrListTail(arrAggregateType);
    }
    else {
        reportSyntaxError("Expected identifier in array list.");
    }
}

// <ArrListTail> → , a [ k ] <ArrListTail> | λ
void Parser::parseArrListTail(SymbolType arrAggregateType) {
    while (match(TokenType::T_COMMA)) {
        if (errorHandler.hasErrors()) return;
        if (currentToken.type == TokenType::T_IDENTIFIER) {
            Token idToken = currentToken;
            nextToken();
            consume(TokenType::T_LBRACKET);
            if (errorHandler.hasErrors()) return;
            if (currentToken.type == TokenType::T_NUMBER_INT) {
                int arraySize = currentToken.getIntValue();
                if (arraySize <= 0) {
                    reportSemanticError("Array size must be a positive integer.", currentToken.line, currentToken.column);
                }
                nextToken();
                symbolTable.addArray(idToken.text, arrAggregateType, idToken.line, static_cast<size_t>(arraySize > 0 ? arraySize : 1));
            }
            else {
                reportSyntaxError("Expected integer number for array size.");
            }
            if (errorHandler.hasErrors()) return;
            consume(TokenType::T_RBRACKET);
        }
        else {
            reportSyntaxError("Expected identifier after comma in array list.");
            break;
        }
    }
}

// A → <Statement> <StatementTail>
void Parser::parseStatementList() {
    if (currentToken.type != TokenType::T_KW_END && currentToken.type != TokenType::T_EOF) {
        if (errorHandler.hasErrors()) return;
        parseStatement();
        if (errorHandler.hasErrors()) return;
        parseStatementTail();
    }
}

// <StatementTail> → ; A | λ
void Parser::parseStatementTail() {
    if (currentToken.type == TokenType::T_SEMICOLON) {
        nextToken();
        if (currentToken.type != TokenType::T_KW_END && currentToken.type != TokenType::T_EOF) {
            if (errorHandler.hasErrors()) return;
            parseStatementList();
        }
    }
}

// <Statement> → aH = G | if (C) <ScopedStatement> E_else | while (C) <ScopedStatement> | cin(aH) | cout(G) | begin A end | λ
void Parser::parseStatement() {
    if (currentToken.type == TokenType::T_SEMICOLON ||
        currentToken.type == TokenType::T_KW_END ||
        currentToken.type == TokenType::T_EOF ||
        currentToken.type == TokenType::T_KW_ELSE) {
        return; // Пустой оператор
    }
    if (errorHandler.hasErrors()) return; // Прерываем, если уже есть ошибки на входе в оператор

    switch (currentToken.type) {
    case TokenType::T_IDENTIFIER:
        parseAssignmentOrExpressionStatement();
        break;
    case TokenType::T_KW_IF:
        parseIfStatement();
        break;
    case TokenType::T_KW_WHILE:
        parseWhileStatement();
        break;
    case TokenType::T_KW_CIN:
        parseCinStatement();
        break;
    case TokenType::T_KW_COUT:
        parseCoutStatement();
        break;
    case TokenType::T_KW_BEGIN:
        parseBeginEndBlock();
        break;
    default:
        reportSyntaxError("Expected a statement (identifier, if, while, cin, cout, begin, or ';').");
        // Попытка пропустить до следующей точки с запятой для простого восстановления
        while (currentToken.type != TokenType::T_SEMICOLON &&
            currentToken.type != TokenType::T_EOF &&
            currentToken.type != TokenType::T_KW_END &&
            !errorHandler.hasErrors()) { // Добавим !errorHandler.hasErrors() чтоб не зациклиться если лексер сломался
            nextToken();
        }
        break;
    }
}

// <ScopedStatement> -> <Statement> | begin A end
void Parser::parseScopedStatement() {
    if (errorHandler.hasErrors()) return;
    if (currentToken.type == TokenType::T_KW_BEGIN) {
        parseBeginEndBlock();
    }
    else {
        parseStatement();
    }
}

// <Assignment> → aH = G
void Parser::parseAssignmentOrExpressionStatement() {
    Token idToken = currentToken;
    nextToken();

    auto symbolOpt = symbolTable.findSymbol(idToken.text);
    if (!symbolOpt) {
        reportSemanticError("Identifier '" + idToken.text + "' not declared.", idToken.line, idToken.column);
        return;
    }
    size_t symbolIndex = symbolOpt.value();
    const SymbolInfo* symbolInfoPtr = symbolTable.getSymbolInfo(symbolIndex);
    if (!symbolInfoPtr) {
        reportSemanticError("Internal: Symbol info not found for declared identifier '" + idToken.text + "'.", idToken.line, idToken.column);
        return;
    }
    const SymbolInfo& symbolInfo = *symbolInfoPtr;

    SymbolType actualLHSItemType;

    if (symbolInfo.type == SymbolType::ARRAY_INT || symbolInfo.type == SymbolType::ARRAY_FLOAT) {
        emit(RPNOpCode::PUSH_ARRAY_ADDR, symbolIndex);
        if (parseArrayIndexOpt(symbolInfo)) {
            emit(RPNOpCode::INDEX);
            actualLHSItemType = (symbolInfo.type == SymbolType::ARRAY_INT) ? SymbolType::VARIABLE_INT : SymbolType::VARIABLE_FLOAT;
        }
        else {
            reportSemanticError("Cannot assign to an entire array '" + symbolInfo.name + "'. Index required.", idToken.line, idToken.column);
            return;
        }
    }
    else if (symbolInfo.type == SymbolType::VARIABLE_INT || symbolInfo.type == SymbolType::VARIABLE_FLOAT) {
        emit(RPNOpCode::PUSH_VAR_ADDR, symbolIndex);
        actualLHSItemType = symbolInfo.type;
    }
    else { // Этого не должно происходить, если типы символов ограничены
        reportSemanticError("Identifier '" + idToken.text + "' is not a variable or array that can be assigned to.", idToken.line, idToken.column);
        return;
    }

    if (errorHandler.hasErrors()) return;
    consume(TokenType::T_ASSIGN);
    if (errorHandler.hasErrors()) return;

    SymbolType expressionType = parseExpression();

    if (expressionType == SymbolType::VARIABLE_INT || expressionType == SymbolType::VARIABLE_FLOAT) {
        ensureTypesMatchOrConvert(actualLHSItemType, expressionType, true /*forAssignment*/);
        emit(RPNOpCode::ASSIGN);
    }
    else if (!errorHandler.hasErrors()) { // Сообщаем об ошибке только если ее не было ранее в выражении
        reportSemanticError("Invalid expression on the right side of assignment for '" + symbolInfo.name + "'.", currentToken.line, currentToken.column);
    }
}

// H → [G] | λ
bool Parser::parseArrayIndexOpt(const SymbolInfo& symbolInfo) {
    if (match(TokenType::T_LBRACKET)) {
        if (errorHandler.hasErrors()) { consume(TokenType::T_RBRACKET); return true; } // Попытка съесть ] если ошибка
        SymbolType indexExprType = parseExpression();

        if (!errorHandler.hasErrors()) { // Проверяем тип индекса только если выражение было корректным
            if (indexExprType == SymbolType::VARIABLE_FLOAT) {
                reportSemanticError("Warning: Array index expression for '" + symbolInfo.name + "' is float, truncating to int.",
                    currentToken.type == TokenType::T_RBRACKET ? currentToken.line : lexer.getNextToken().line, // Попытка получить позицию конца выражения
                    currentToken.type == TokenType::T_RBRACKET ? currentToken.column - 1 : lexer.getNextToken().column - 1);
                emit(RPNOpCode::CONVERT_TO_INT);
            }
            else if (indexExprType != SymbolType::VARIABLE_INT) {
                reportSemanticError("Array index expression for '" + symbolInfo.name + "' must evaluate to an integer.",
                    currentToken.type == TokenType::T_RBRACKET ? currentToken.line : lexer.getNextToken().line,
                    currentToken.type == TokenType::T_RBRACKET ? currentToken.column - 1 : lexer.getNextToken().column - 1);
            }
        }
        if (errorHandler.hasErrors() && currentToken.type != TokenType::T_RBRACKET) {
            // Если были ошибки в выражении индекса и мы не на ']', пытаемся пропустить
            while (currentToken.type != TokenType::T_RBRACKET && currentToken.type != TokenType::T_SEMICOLON && currentToken.type != TokenType::T_EOF) nextToken();
        }
        consume(TokenType::T_RBRACKET);
        return true;
    }
    return false;
}

void Parser::parseIfStatement() {
    consume(TokenType::T_KW_IF);
    if (errorHandler.hasErrors()) return;
    consume(TokenType::T_LPAREN);
    if (errorHandler.hasErrors()) return;
    parseCondition();
    if (errorHandler.hasErrors()) { consume(TokenType::T_RPAREN); return; }
    consume(TokenType::T_RPAREN);

    int jumpFalsePlaceholder = emitJumpPlaceholder(RPNOpCode::JUMP_FALSE);
    parseScopedStatement();

    int jumpOverElsePlaceholder = -1;
    if (currentToken.type == TokenType::T_KW_ELSE) {
        jumpOverElsePlaceholder = emitJumpPlaceholder(RPNOpCode::JUMP);
    }

    patchJump(jumpFalsePlaceholder, getCurrentRPNAddress());

    if (currentToken.type == TokenType::T_KW_ELSE) {
        if (errorHandler.hasErrors()) return; // Не парсим else, если 'then' блок с ошибками
        parseElseClause(jumpOverElsePlaceholder);
    }
}

void Parser::parseElseClause(int jumpOverElsePlaceholder) {
    consume(TokenType::T_KW_ELSE);
    if (errorHandler.hasErrors()) return;
    parseScopedStatement();
    if (jumpOverElsePlaceholder != -1) { // Убедимся, что метка была создана
        patchJump(jumpOverElsePlaceholder, getCurrentRPNAddress());
    }
}

void Parser::parseWhileStatement() {
    consume(TokenType::T_KW_WHILE);
    if (errorHandler.hasErrors()) return;

    int conditionStartAddress = getCurrentRPNAddress();

    consume(TokenType::T_LPAREN);
    if (errorHandler.hasErrors()) return;
    parseCondition();
    if (errorHandler.hasErrors()) { consume(TokenType::T_RPAREN); return; }
    consume(TokenType::T_RPAREN);

    int jumpFalseToEndPlaceholder = emitJumpPlaceholder(RPNOpCode::JUMP_FALSE);
    parseScopedStatement();

    emit(RPNOpCode::JUMP);
    patchJump(getCurrentRPNAddress() - 1, conditionStartAddress);

    patchJump(jumpFalseToEndPlaceholder, getCurrentRPNAddress());
}

void Parser::parseCinStatement() {
    consume(TokenType::T_KW_CIN);
    if (errorHandler.hasErrors()) return;
    consume(TokenType::T_LPAREN);
    if (errorHandler.hasErrors()) return;

    if (currentToken.type == TokenType::T_IDENTIFIER) {
        Token idToken = currentToken;
        nextToken();

        auto symbolOpt = symbolTable.findSymbol(idToken.text);
        if (!symbolOpt) {
            reportSemanticError("Identifier '" + idToken.text + "' not declared for input.", idToken.line, idToken.column);
            if (currentToken.type == TokenType::T_RPAREN) consume(TokenType::T_RPAREN);
            return;
        }
        size_t symbolIndex = symbolOpt.value();
        const SymbolInfo* symInfoPtr = symbolTable.getSymbolInfo(symbolIndex);
        if (!symInfoPtr) {
            reportSemanticError("Internal: Symbol info missing for '" + idToken.text + "'.", idToken.line, idToken.column);
            if (currentToken.type == TokenType::T_RPAREN) consume(TokenType::T_RPAREN);
            return;
        }
        const SymbolInfo& symInfo = *symInfoPtr;

        RPNOpCode readOpCode;
        SymbolType itemToReadType = symInfo.type;

        if (symInfo.type == SymbolType::ARRAY_INT || symInfo.type == SymbolType::ARRAY_FLOAT) {
            emit(RPNOpCode::PUSH_ARRAY_ADDR, symbolIndex);
            if (parseArrayIndexOpt(symInfo)) {
                emit(RPNOpCode::INDEX);
                itemToReadType = (symInfo.type == SymbolType::ARRAY_INT) ? SymbolType::VARIABLE_INT : SymbolType::VARIABLE_FLOAT;
            }
            else { // Ошибка (отсутствие индекса) уже обработана в parseArrayIndexOpt, если нужно
                if (!errorHandler.hasErrors()) // Если parseArrayIndexOpt не сообщил об ошибке
                    reportSemanticError("Cannot read into an entire array '" + symInfo.name + "'. Index required.", idToken.line, idToken.column);
                if (currentToken.type == TokenType::T_RPAREN) consume(TokenType::T_RPAREN);
                return;
            }
        }
        else if (symInfo.type == SymbolType::VARIABLE_INT || symInfo.type == SymbolType::VARIABLE_FLOAT) {
            emit(RPNOpCode::PUSH_VAR_ADDR, symbolIndex);
        }
        else {
            reportSemanticError("Identifier '" + symInfo.name + "' is not a variable or array element suitable for 'cin'.", idToken.line, idToken.column);
            if (currentToken.type == TokenType::T_RPAREN) consume(TokenType::T_RPAREN);
            return;
        }

        if (itemToReadType == SymbolType::VARIABLE_INT) readOpCode = RPNOpCode::READ_INT;
        else if (itemToReadType == SymbolType::VARIABLE_FLOAT) readOpCode = RPNOpCode::READ_FLOAT;
        else {
            reportSemanticError("Internal: Undetermined type for read operation on '" + symInfo.name + "'.", idToken.line, idToken.column);
            if (currentToken.type == TokenType::T_RPAREN) consume(TokenType::T_RPAREN); return;
        }
        if (!errorHandler.hasErrors()) emit(readOpCode, true /*isIoPlaceholder*/);

    }
    else {
        reportSyntaxError("Expected identifier inside cin(...).");
    }
    if (errorHandler.hasErrors() && currentToken.type != TokenType::T_RPAREN) {
        // пытаемся найти ')'
        while (currentToken.type != TokenType::T_RPAREN && currentToken.type != TokenType::T_SEMICOLON && currentToken.type != TokenType::T_EOF) nextToken();
    }
    consume(TokenType::T_RPAREN);
}

void Parser::parseCoutStatement() {
    consume(TokenType::T_KW_COUT);
    if (errorHandler.hasErrors()) return;
    consume(TokenType::T_LPAREN);
    if (errorHandler.hasErrors()) return;

    SymbolType exprType = parseExpression();

    if (errorHandler.hasErrors()) { // Если ошибка в выражении, не генерируем write
        if (currentToken.type == TokenType::T_RPAREN) consume(TokenType::T_RPAREN);
        return;
    }

    RPNOpCode writeOpCode;
    if (exprType == SymbolType::VARIABLE_INT) writeOpCode = RPNOpCode::WRITE_INT;
    else if (exprType == SymbolType::VARIABLE_FLOAT) writeOpCode = RPNOpCode::WRITE_FLOAT;
    else {
        reportSemanticError("Cannot determine type of expression for 'cout'. Expression result type is invalid.", currentToken.line, currentToken.column);
        if (currentToken.type == TokenType::T_RPAREN) consume(TokenType::T_RPAREN);
        return;
    }
    emit(writeOpCode, true /*isIoPlaceholder*/);
    consume(TokenType::T_RPAREN);
}

void Parser::parseBeginEndBlock() {
    consume(TokenType::T_KW_BEGIN);
    if (errorHandler.hasErrors()) return;
    parseStatementList();
    if (errorHandler.hasErrors() && currentToken.type != TokenType::T_KW_END) {
        // Пытаемся найти 'end' если были ошибки внутри блока
        while (currentToken.type != TokenType::T_KW_END && currentToken.type != TokenType::T_EOF) nextToken();
    }
    consume(TokenType::T_KW_END);
}

// --- Разбор выражений и условий ---

// <Condition> → G <ComparisonOp> G
void Parser::parseCondition() {
    SymbolType leftExprType = parseExpression();
    if (errorHandler.hasErrors()) return;
    RPNOpCode comparisonOp = parseComparisonOp();
    if (errorHandler.hasErrors()) return;
    SymbolType rightExprType = parseExpression();
    if (errorHandler.hasErrors()) return;

    bool leftOk = (leftExprType == SymbolType::VARIABLE_INT || leftExprType == SymbolType::VARIABLE_FLOAT);
    bool rightOk = (rightExprType == SymbolType::VARIABLE_INT || rightExprType == SymbolType::VARIABLE_FLOAT);

    if (!leftOk) {
        reportSemanticError("Left operand of comparison is not a valid numeric type.", currentToken.line, currentToken.column);
    }
    if (!rightOk) {
        reportSemanticError("Right operand of comparison is not a valid numeric type.", currentToken.line, currentToken.column);
    }

    if (leftOk && rightOk) {
        ensureTypesMatchOrConvert(leftExprType, rightExprType, false /*not for assignment*/);
        emit(comparisonOp);
    }
}

// <ComparisonOp> → ~ | > | < | !
RPNOpCode Parser::parseComparisonOp() {
    Token opToken = currentToken;
    if (match(TokenType::T_EQUAL))     return RPNOpCode::CMP_EQ;
    if (match(TokenType::T_GREATER))   return RPNOpCode::CMP_GT;
    if (match(TokenType::T_LESS))      return RPNOpCode::CMP_LT;
    if (match(TokenType::T_NOT_EQUAL)) return RPNOpCode::CMP_NE;

    reportSyntaxError("Expected comparison operator (~, >, <, !). Found '" + opToken.text + "'.");
    return RPNOpCode::CMP_EQ;
}

// G → <Term> <ExpressionPrime>
SymbolType Parser::parseExpression() {
    SymbolType termType = parseTerm();
    if (errorHandler.hasErrors()) return SymbolType::VARIABLE_INT; // Возврат типа по умолчанию при ошибке
    return parseExpressionPrime(termType);
}

// <ExpressionPrime> → + <Term> <ExpressionPrime> | - <Term> <ExpressionPrime> | λ
SymbolType Parser::parseExpressionPrime(SymbolType leftOperandType) {
    SymbolType currentResultType = leftOperandType;
    while (currentToken.type == TokenType::T_PLUS || currentToken.type == TokenType::T_MINUS) {
        if (errorHandler.hasErrors()) return currentResultType; // Прерываем при ошибке

        Token opToken = currentToken;
        nextToken();

        SymbolType rightOperandType = parseTerm();
        if (errorHandler.hasErrors()) return currentResultType; // Прерываем, если ошибка в правом терме

        // Проверка типов операндов
        bool leftOk = (currentResultType == SymbolType::VARIABLE_INT || currentResultType == SymbolType::VARIABLE_FLOAT);
        bool rightOk = (rightOperandType == SymbolType::VARIABLE_INT || rightOperandType == SymbolType::VARIABLE_FLOAT);

        if (!leftOk || !rightOk) {
            reportSemanticError("Invalid operand type(s) for '" + opToken.text + "' operation.", opToken.line, opToken.column);
            // Тип результата не меняется, пропускаем emit
        }
        else {
            ensureTypesMatchOrConvert(currentResultType, rightOperandType, false /*for binary op*/);

            if (currentResultType == SymbolType::VARIABLE_FLOAT || rightOperandType == SymbolType::VARIABLE_FLOAT) {
                currentResultType = SymbolType::VARIABLE_FLOAT;
            }
            else {
                currentResultType = SymbolType::VARIABLE_INT;
            }
            emit(opToken.type == TokenType::T_PLUS ? RPNOpCode::ADD : RPNOpCode::SUB);
        }
    }
    return currentResultType;
}

// <Term> → <Factor> <TermPrime>
SymbolType Parser::parseTerm() {
    SymbolType factorType = parseFactor();
    if (errorHandler.hasErrors()) return SymbolType::VARIABLE_INT;
    return parseTermPrime(factorType);
}

// <TermPrime> → * <Factor> <TermPrime> | / <Factor> <TermPrime> | λ
SymbolType Parser::parseTermPrime(SymbolType leftOperandType) {
    SymbolType currentResultType = leftOperandType;
    while (currentToken.type == TokenType::T_MULTIPLY || currentToken.type == TokenType::T_DIVIDE) {
        if (errorHandler.hasErrors()) return currentResultType;

        Token opToken = currentToken;
        nextToken();

        SymbolType rightOperandType = parseFactor();
        if (errorHandler.hasErrors()) return currentResultType;

        bool leftOk = (currentResultType == SymbolType::VARIABLE_INT || currentResultType == SymbolType::VARIABLE_FLOAT);
        bool rightOk = (rightOperandType == SymbolType::VARIABLE_INT || rightOperandType == SymbolType::VARIABLE_FLOAT);

        if (!leftOk || !rightOk) {
            reportSemanticError("Invalid operand type(s) for '" + opToken.text + "' operation.", opToken.line, opToken.column);
        }
        else {
            ensureTypesMatchOrConvert(currentResultType, rightOperandType, false);

            if (currentResultType == SymbolType::VARIABLE_FLOAT || rightOperandType == SymbolType::VARIABLE_FLOAT) {
                currentResultType = SymbolType::VARIABLE_FLOAT;
            }
            else {
                currentResultType = SymbolType::VARIABLE_INT;
            }
            emit(opToken.type == TokenType::T_MULTIPLY ? RPNOpCode::MUL : RPNOpCode::DIV);
        }
    }
    return currentResultType;
}

// <Factor> → (G) | aH | k_int | k_float | -<Factor> (Унарный минус)
SymbolType Parser::parseFactor() {
    Token factorStartToken = currentToken;
    SymbolType factorType = SymbolType::VARIABLE_INT; // Тип по умолчанию при ошибке

    if (errorHandler.hasErrors()) return factorType;

    switch (currentToken.type) {
    case TokenType::T_LPAREN:
        nextToken();
        factorType = parseExpression();
        if (errorHandler.hasErrors() && currentToken.type != TokenType::T_RPAREN) {
            while (currentToken.type != TokenType::T_RPAREN && currentToken.type != TokenType::T_SEMICOLON && currentToken.type != TokenType::T_EOF) nextToken();
        }
        consume(TokenType::T_RPAREN);
        break;
    case TokenType::T_IDENTIFIER: {
        Token idToken = currentToken;
        nextToken();

        auto symbolOpt = symbolTable.findSymbol(idToken.text);
        if (!symbolOpt) {
            reportSemanticError("Identifier '" + idToken.text + "' not declared.", idToken.line, idToken.column);
            return factorType;
        }
        size_t symbolIndex = symbolOpt.value();
        const SymbolInfo* symInfoPtr = symbolTable.getSymbolInfo(symbolIndex);
        if (!symInfoPtr) {
            reportSemanticError("Internal: Symbol info missing for '" + idToken.text + "'.", idToken.line, idToken.column);
            return factorType;
        }
        const SymbolInfo& symInfo = *symInfoPtr;

        if (symInfo.type == SymbolType::ARRAY_INT || symInfo.type == SymbolType::ARRAY_FLOAT) {
            emit(RPNOpCode::PUSH_ARRAY_ADDR, symbolIndex);
            if (parseArrayIndexOpt(symInfo)) {
                emit(RPNOpCode::INDEX);
                factorType = (symInfo.type == SymbolType::ARRAY_INT) ? SymbolType::VARIABLE_INT : SymbolType::VARIABLE_FLOAT;
            }
            else {
                if (!errorHandler.hasErrors()) // Если parseArrayIndexOpt не сообщил об ошибке
                    reportSemanticError("Cannot use an entire array '" + symInfo.name + "' as a value in an expression. Index required.", idToken.line, idToken.column);
                // Тип factorType останется дефолтным (INT), что может вызвать проблемы далее.
            }
        }
        else if (symInfo.type == SymbolType::VARIABLE_INT || symInfo.type == SymbolType::VARIABLE_FLOAT) {
            // Для использования значения переменной в выражении, мы кладем ее АДРЕС.
            // Интерпретатор будет извлекать значение по этому адресу при выполнении операций.
            emit(RPNOpCode::PUSH_VAR_ADDR, symbolIndex);
            factorType = symInfo.type;
        }
        else { // Не должно случиться
            reportSemanticError("Identifier '" + symInfo.name + "' is not a recognized variable or array for use in expression.", idToken.line, idToken.column);
        }
        break;
    }
    case TokenType::T_NUMBER_INT:
        emit(RPNOpCode::PUSH_CONST_INT, currentToken.getIntValue());
        nextToken();
        factorType = SymbolType::VARIABLE_INT;
        break;
    case TokenType::T_NUMBER_FLOAT:
        emit(RPNOpCode::PUSH_CONST_FLOAT, currentToken.getFloatValue());
        nextToken();
        factorType = SymbolType::VARIABLE_FLOAT;
        break;
    case TokenType::T_MINUS:
        nextToken();
        {
            SymbolType subFactorType = parseFactor();
            if (errorHandler.hasErrors()) return factorType; // Если ошибка в subFactor

            if (subFactorType == SymbolType::VARIABLE_INT) {
                emit(RPNOpCode::PUSH_CONST_INT, 0); // 0
                ensureTypesMatchOrConvert(SymbolType::VARIABLE_INT, subFactorType, false); // 0 (int) - subFactor (int)
                emit(RPNOpCode::SUB);
                factorType = SymbolType::VARIABLE_INT;
            }
            else if (subFactorType == SymbolType::VARIABLE_FLOAT) {
                emit(RPNOpCode::PUSH_CONST_FLOAT, 0.0f); // 0.0
                ensureTypesMatchOrConvert(SymbolType::VARIABLE_FLOAT, subFactorType, false); // 0.0 (float) - subFactor (float)
                emit(RPNOpCode::SUB);
                factorType = SymbolType::VARIABLE_FLOAT;
            }
            else {
                reportSemanticError("Cannot apply unary minus to non-numeric type.", factorStartToken.line, factorStartToken.column);
            }
        }
        break;
    default:
        reportSyntaxError("Expected factor (expression in parenthesis, identifier, number, or unary minus). Found '" + factorStartToken.text + "'.");
        break;
    }
    return factorType;
}

// --- Семантические проверки и утилиты ---
void Parser::ensureTypesMatchOrConvert(SymbolType typeLHS, SymbolType typeRHS, bool forAssignment) {
    // typeLHS - тип левого операнда (или цели присваивания). Уже на стеке (или адрес для присваивания).
    // typeRHS - тип правого операнда (или источника для присваивания). Только что сгенерирован ОПС для него (на вершине).

    bool lhsIsNum = (typeLHS == SymbolType::VARIABLE_INT || typeLHS == SymbolType::VARIABLE_FLOAT);
    bool rhsIsNum = (typeRHS == SymbolType::VARIABLE_INT || typeRHS == SymbolType::VARIABLE_FLOAT);

    if (!lhsIsNum || !rhsIsNum) {
        // Если один из операндов нечисловой, и это не ошибка, уже обработанная ранее,
        // то здесь можно дополнительно сообщить, но обычно предыдущие проверки типа должны были это покрыть.
        // reportSemanticError("Type mismatch: operands must be numeric for this operation.");
        return;
    }

    if (typeLHS == typeRHS) {
        return; // Типы совпадают, конвертация не нужна
    }

    if (forAssignment) {
        // Присваивание: RHS (typeRHS, на вершине стека) конвертируется к типу LHS (typeLHS)
        if (typeLHS == SymbolType::VARIABLE_INT && typeRHS == SymbolType::VARIABLE_FLOAT) {
            // Цель int, источник float -> конвертируем float на вершине в int
            emit(RPNOpCode::CONVERT_TO_INT);
        }
        else if (typeLHS == SymbolType::VARIABLE_FLOAT && typeRHS == SymbolType::VARIABLE_INT) {
            // Цель float, источник int -> конвертируем int на вершине в float
            emit(RPNOpCode::CONVERT_TO_FLOAT);
        }
    }
    else {
        // Бинарные арифметические/сравнительные операции: оба операнда приводятся к float, если один из них float
        // На стеке: ... val_LHS val_RHS
        if (typeLHS == SymbolType::VARIABLE_FLOAT && typeRHS == SymbolType::VARIABLE_INT) {
            // val_LHS (float), val_RHS (int) -> конвертируем val_RHS (на вершине) в float
            emit(RPNOpCode::CONVERT_TO_FLOAT);
        }
        else if (typeLHS == SymbolType::VARIABLE_INT && typeRHS == SymbolType::VARIABLE_FLOAT) {
            // val_LHS (int), val_RHS (float)
            // Нужно конвертировать val_LHS. Это требует:
            // 1. Сохранить val_RHS (на вершине) во временную переменную (или специальную ОПС команду)
            // 2. Вытащить val_LHS, конвертировать, положить обратно
            // 3. Вернуть val_RHS на стек
            // Это сложно для текущей ОПС.
            // Проще всего, если интерпретатор делает неявное повышение или
            // мы пересматриваем порядок генерации ОПС для левого операнда.

            // Текущее УПРОЩЕННОЕ решение: Мы НЕ конвертируем здесь левый операнд (typeLHS).
            // Вместо этого, логика в parseExpressionPrime/parseTermPrime ДОЛЖНА была обеспечить,
            // что если currentResultType (который станет typeLHS здесь) был INT,
            // а только что разобранный правый операнд (который станет typeRHS здесь) FLOAT,
            // то currentResultType сам становится FLOAT (и его значение на стеке должно было быть
            // преобразовано, если это был результат предыдущей операции).
            // Однако, если typeLHS - это результат parseFactor (например, int переменная), а typeRHS - float,
            // то именно здесь нужно вставить конвертацию для typeLHS.
            // Это все еще указывает на сложность.

            // Более прагматичный подход для ЯВНОЙ конверсии парсером:
            // Если мы определили, что итоговая операция будет float (т.к. один из операндов float),
            // то перед emit(OPERATOR) мы должны убедиться, что ОБА операнда на стеке - float.
            // В parseExpressionPrime/TermPrime, после разбора правого операнда:
            // if (currentResultType_был_INT && rightOperandType_стал_FLOAT) -> КОНВЕРТИРОВАТЬ ЛЕВЫЙ (сложно)
            // if (currentResultType_был_FLOAT && rightOperandType_стал_INT) -> КОНВЕРТИРОВАТЬ ПРАВЫЙ (просто, emit CONVERT_TO_FLOAT)

            // ОСТАВИМ ТАК: если левый INT, а правый FLOAT, то интерпретатор должен будет повысить левый.
            // Если левый FLOAT, а правый INT, то мы конвертируем правый (на вершине).
            // Эта асимметрия - компромисс.
            // reportSemanticError("Performing implicit promotion for binary operation: left operand (int) with right operand (float). Interpreter will handle promotion.", currentToken.line, currentToken.column);
            // Ничего не генерируем здесь, полагаемся на интерпретатор для (INT op FLOAT)
        }
    }
}

// --- Получение и вывод ОПС ---
const std::vector<RPNOperation>& Parser::getRPNCode() const {
    return rpnCode;
}

void Parser::printRPN() const {
    std::cout << "\n--- Reverse Polish Notation (RPN) ---" << std::endl;
    std::cout << "Idx | OpCode            | Operand  | SymIdx | JumpTo" << std::endl;
    std::cout << "----|-------------------|----------|--------|--------" << std::endl;
    for (size_t i = 0; i < rpnCode.size(); ++i) {
        const auto& op = rpnCode[i];
        std::cout << std::setw(3) << i << " | ";

        switch (op.opCode) {
        case RPNOpCode::PUSH_VAR_ADDR:    std::cout << std::left << std::setw(17) << "PUSH_VAR_ADDR"; break;
        case RPNOpCode::PUSH_ARRAY_ADDR:  std::cout << std::left << std::setw(17) << "PUSH_ARRAY_ADDR"; break;
        case RPNOpCode::PUSH_CONST_INT:   std::cout << std::left << std::setw(17) << "PUSH_CONST_INT"; break;
        case RPNOpCode::PUSH_CONST_FLOAT: std::cout << std::left << std::setw(17) << "PUSH_CONST_FLOAT"; break;
        case RPNOpCode::ADD:              std::cout << std::left << std::setw(17) << "ADD"; break;
        case RPNOpCode::SUB:              std::cout << std::left << std::setw(17) << "SUB"; break;
        case RPNOpCode::MUL:              std::cout << std::left << std::setw(17) << "MUL"; break;
        case RPNOpCode::DIV:              std::cout << std::left << std::setw(17) << "DIV"; break;
        case RPNOpCode::CMP_EQ:           std::cout << std::left << std::setw(17) << "CMP_EQ"; break;
        case RPNOpCode::CMP_NE:           std::cout << std::left << std::setw(17) << "CMP_NE"; break;
        case RPNOpCode::CMP_GT:           std::cout << std::left << std::setw(17) << "CMP_GT"; break;
        case RPNOpCode::CMP_LT:           std::cout << std::left << std::setw(17) << "CMP_LT"; break;
        case RPNOpCode::ASSIGN:           std::cout << std::left << std::setw(17) << "ASSIGN"; break;
        case RPNOpCode::INDEX:            std::cout << std::left << std::setw(17) << "INDEX"; break;
        case RPNOpCode::READ_INT:         std::cout << std::left << std::setw(17) << "READ_INT"; break;
        case RPNOpCode::READ_FLOAT:       std::cout << std::left << std::setw(17) << "READ_FLOAT"; break;
        case RPNOpCode::WRITE_INT:        std::cout << std::left << std::setw(17) << "WRITE_INT"; break;
        case RPNOpCode::WRITE_FLOAT:      std::cout << std::left << std::setw(17) << "WRITE_FLOAT"; break;
        case RPNOpCode::JUMP:             std::cout << std::left << std::setw(17) << "JUMP"; break;
        case RPNOpCode::JUMP_FALSE:       std::cout << std::left << std::setw(17) << "JUMP_FALSE"; break;
        case RPNOpCode::CONVERT_TO_FLOAT: std::cout << std::left << std::setw(17) << "CONVERT_TO_FLOAT"; break;
        case RPNOpCode::CONVERT_TO_INT:   std::cout << std::left << std::setw(17) << "CONVERT_TO_INT"; break;
            // default убран, чтобы компилятор предупреждал о необработанных RPNOpCode
        }
        std::cout << " | ";

        if (std::holds_alternative<int>(op.operandValue)) {
            std::cout << std::setw(8) << std::get<int>(op.operandValue);
        }
        else if (std::holds_alternative<float>(op.operandValue)) {
            std::cout << std::fixed << std::setprecision(2) << std::setw(8) << std::get<float>(op.operandValue);
        }
        else {
            std::cout << std::setw(8) << "-";
        }
        std::cout << " | ";

        if (op.symbolIndex.has_value()) {
            std::cout << std::setw(6) << op.symbolIndex.value();
        }
        else {
            std::cout << std::setw(6) << "-";
        }
        std::cout << " | ";

        if (op.jumpTarget.has_value()) {
            if (op.jumpTarget.value() == -1) std::cout << std::setw(6) << "(ph)";
            else std::cout << std::setw(6) << op.jumpTarget.value();
        }
        else {
            std::cout << std::setw(6) << "-";
        }
        std::cout << std::endl;
    }
    std::cout << "-------------------------------------------------" << std::endl;
}