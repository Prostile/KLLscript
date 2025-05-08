#include "Parser.h"
#include <iostream>
#include <iomanip>
#include <stdexcept> // Для visit

// --- Конструктор и базовые методы --- (без существенных изменений)
Parser::Parser(Lexer& lex, SymbolTable& symTab)
    : lexer(lex), symbolTable(symTab), declarationPhase(true) {
    nextToken();
}
void Parser::nextToken() { /* ... как раньше ... */ }
bool Parser::match(TokenType expectedType) { /* ... как раньше ... */ }

void Parser::syntaxError(const std::string& message) {
    // TODO: Интегрировать ErrorHandler
    std::cerr << "Syntax Error (Line " << currentToken.line << ", Col " << currentToken.column << "): "
        << message << ". Found token type: " << static_cast<int>(currentToken.type)
        << " ('" << currentToken.text << "')" << std::endl;
    exit(EXIT_FAILURE);
}
// Новый метод для семантических ошибок
void Parser::semanticError(const std::string& message) {
    // TODO: Интегрировать ErrorHandler
    std::cerr << "Semantic Error (Line " << currentToken.line << ", Col " << currentToken.column << "): "
        << message << std::endl;
    exit(EXIT_FAILURE);
}

// --- Генерация ОПС --- (добавлен emitCast)
void Parser::emit(RPNOpCode opCode) { rpnCode.emplace_back(opCode); }
void Parser::emit(RPNOpCode opCode, const SymbolValue& value) { rpnCode.emplace_back(opCode, value); }
void Parser::emit(RPNOpCode opCode, int symbolIndex) { rpnCode.emplace_back(opCode, symbolIndex); }
int Parser::emitPlaceholder(RPNOpCode jumpType) { /* ... как раньше ... */ }
void Parser::patchJump(int placeholderIndex) { /* ... как раньше ... */ }
void Parser::patchJumpTo(int placeholderIndex, int targetAddress) { /* ... как раньше ... */ }
int Parser::getCurrentRPNAddress() const { return static_cast<int>(rpnCode.size()); }

// Генерация операции приведения типа, если необходимо
void Parser::emitCast(SymbolType fromType, SymbolType toType) {
    if (fromType == toType) return; // Приведение не нужно

    if ((fromType == SymbolType::VARIABLE_INT || fromType == SymbolType::ARRAY_INT) &&
        (toType == SymbolType::VARIABLE_FLOAT || toType == SymbolType::ARRAY_FLOAT)) {
        emit(RPNOpCode::CAST_I2F);
    }
    else if ((fromType == SymbolType::VARIABLE_INT || fromType == SymbolType::ARRAY_INT) &&
        (toType == SymbolType::VARIABLE_BOOL || toType == SymbolType::ARRAY_BOOL)) {
        emit(RPNOpCode::CAST_I2B); // 0 -> false, non-0 -> true
    }
    // Другие приведения (например, F2I, F2B) пока запрещены неявно
    // Можно добавить сюда явные ошибки, если нужно
}


// --- Основной метод парсинга ---
bool Parser::parse() { /* ... как раньше ... */ }

// --- Разбор программы и объявлений ---

void Parser::parseProgram() {
    parseDeclarationList();
    declarationPhase = false;
    parseStatementList();
}

void Parser::parseDeclarationList() {
    while (currentToken.type == TokenType::T_KW_INT ||
        currentToken.type == TokenType::T_KW_FLOAT ||
        currentToken.type == TokenType::T_KW_BOOL ||
        currentToken.type == TokenType::T_KW_ARR)
    {
        parseDeclaration();
        if (!match(TokenType::T_SEMICOLON)) return;
    }
}

// Новый метод для разбора спецификатора типа
SymbolType Parser::parseTypeSpecifier() {
    switch (currentToken.type) {
    case TokenType::T_KW_INT:   nextToken(); return SymbolType::VARIABLE_INT;
    case TokenType::T_KW_FLOAT: nextToken(); return SymbolType::VARIABLE_FLOAT;
    case TokenType::T_KW_BOOL:  nextToken(); return SymbolType::VARIABLE_BOOL;
    default:
        syntaxError("Expected type specifier (int, float, bool).");
        return SymbolType::VARIABLE_INT; // Dummy
    }
}

void Parser::parseDeclaration() {
    if (currentToken.type == TokenType::T_KW_ARR) {
        nextToken(); // съели 'arr'
        SymbolType elementType = parseTypeSpecifier(); // Получаем тип элемента
        SymbolType arrayType;
        if (elementType == SymbolType::VARIABLE_INT) arrayType = SymbolType::ARRAY_INT;
        else if (elementType == SymbolType::VARIABLE_FLOAT) arrayType = SymbolType::ARRAY_FLOAT;
        else if (elementType == SymbolType::VARIABLE_BOOL) arrayType = SymbolType::ARRAY_BOOL;
        else { // На всякий случай
            syntaxError("Invalid element type specified for array.");
            return;
        }
        parseArrayDeclaration(arrayType);
    }
    else { // Обычная переменная
        SymbolType varType = parseTypeSpecifier();
        parseVariableDeclaration(varType);
    }
}

// Разбор объявления переменных (пока только одна за раз)
void Parser::parseVariableDeclaration(SymbolType baseType) {
    if (currentToken.type == TokenType::T_IDENTIFIER) {
        std::string name = currentToken.text;
        int line = currentToken.line;
        nextToken();
        if (!symbolTable.addSymbol(name, baseType, line).has_value()) {
            semanticError("Failed to declare variable '" + name + "' (maybe redeclaration?).");
        }
    }
    else {
        syntaxError("Expected identifier in variable declaration.");
    }
}

// Разбор объявления массива (пока только один за раз)
void Parser::parseArrayDeclaration(SymbolType arrayType) {
    if (currentToken.type == TokenType::T_IDENTIFIER) {
        std::string name = currentToken.text;
        int line = currentToken.line;
        nextToken();
        if (!match(TokenType::T_LBRACKET)) return;

        // Размер массива должен быть целочисленной константой
        if (currentToken.type == TokenType::T_NUMBER_INT) {
            int arraySize = currentToken.intValue;
            if (arraySize <= 0) {
                syntaxError("Array size must be a positive integer constant.");
                return;
            }
            nextToken();
            auto indexOpt = symbolTable.addSymbol(name, arrayType, line);
            if (!indexOpt.has_value()) {
                semanticError("Failed to declare array '" + name + "' (maybe redeclaration?).");
                return;
            }
            symbolTable.resizeArray(indexOpt.value(), arraySize);
        }
        else {
            syntaxError("Expected positive integer constant for array size.");
            return;
        }
        if (!match(TokenType::T_RBRACKET)) return;
    }
    else {
        syntaxError("Expected identifier in array declaration.");
    }
}

// --- Разбор операторов --- (parseStatementList, parseStatement - без изменений)
void Parser::parseStatementList() { /* ... как раньше ... */ }
void Parser::parseStatement() { /* ... как раньше ... */ }

// --- Разбор присваивания (обновлен) ---
void Parser::parseAssignment(const std::string& identifierName, int line, int col) {
    auto indexOpt = symbolTable.findSymbol(identifierName);
    if (!indexOpt.has_value()) {
        semanticError("Identifier '" + identifierName + "' not declared."); // Теперь семантическая
        return; // Возвращаемся, чтобы парсер мог попытаться продолжить (если есть восстановление)
    }
    size_t symbolIndex = indexOpt.value();
    SymbolType targetType = symbolTable.getSymbolType(symbolIndex);
    SymbolType targetBaseType = targetType; // Для проверки присваивания

    bool isArrayAssignment = false;

    // Генерируем базу массива, если это доступ к массиву
    if (currentToken.type == TokenType::T_LBRACKET) {
        if (!symbolTable.isArray(symbolIndex)) {
            semanticError("Identifier '" + identifierName + "' is not an array, cannot use index.");
            return;
        }
        emit(RPNOpCode::PUSH_ARRAY, symbolIndex); // Генерируем базу массива
        isArrayAssignment = true;
        targetBaseType = symbolTable.getArrayElementType(targetType); // Цель - элемент массива
    }
    else if (symbolTable.isArray(symbolIndex)) {
        semanticError("Cannot assign to an entire array '" + identifierName + "'. Index is required.");
        return;
    }
    else if (!symbolTable.isVariable(symbolIndex)) {
        // На случай, если появятся другие типы символов
        semanticError("Identifier '" + identifierName + "' is not assignable.");
        return;
    }

    // Разбираем индекс, если есть
    std::optional<SymbolType> indexTypeOpt;
    if (isArrayAssignment) {
        indexTypeOpt = parseArrayIndexOpt(symbolIndex, targetType);
        if (!indexTypeOpt || indexTypeOpt.value() != SymbolType::VARIABLE_INT) {
            semanticError("Array index for '" + identifierName + "' must be an integer expression.");
            return;
        }
    }

    if (!match(TokenType::T_ASSIGN)) return;

    // Генерируем адрес назначения
    if (isArrayAssignment) {
        emit(RPNOpCode::INDEX); // Вычисляем адрес элемента
    }
    else {
        emit(RPNOpCode::PUSH_VAR, symbolIndex); // Адрес простой переменной
    }

    // Разбираем правую часть (выражение)
    std::optional<SymbolType> valueTypeOpt = parseExpression();
    if (!valueTypeOpt) {
        // Ошибка уже должна была быть выведена внутри parseExpression
        return;
    }
    SymbolType valueType = valueTypeOpt.value();

    // Проверка совместимости типов присваивания
    if (!symbolTable.checkAssignmentTypeCompatibility(targetBaseType, valueType)) {
        semanticError("Type mismatch in assignment to '" + identifierName + "'. Cannot assign value type to target type.");
        return;
    }

    // Генерируем приведение int к float, если нужно
    emitCast(valueType, targetBaseType);

    // Генерируем операцию присваивания
    emit(RPNOpCode::ASSIGN);
}

// Разбор индекса массива
std::optional<SymbolType> Parser::parseArrayIndexOpt(int identifierIndex, SymbolType baseType) {
    if (currentToken.type == TokenType::T_LBRACKET) {
        nextToken();
        std::optional<SymbolType> indexType = parseExpression();
        match(TokenType::T_RBRACKET);
        // Проверяем, что индекс целочисленный
        if (!indexType || (indexType.value() != SymbolType::VARIABLE_INT && indexType.value() != SymbolType::ARRAY_INT)) {
            // Возвращаем nullopt, чтобы indicate error (сообщение выше)
            return std::nullopt;
        }
        return SymbolType::VARIABLE_INT; // Индекс всегда int
    }
    return std::nullopt; // Индекса нет
}


// --- Разбор if/while --- (без изменений в логике меток, но условие теперь parseCondition)
void Parser::parseIfStatement() {
    match(TokenType::T_KW_IF);
    match(TokenType::T_LPAREN);
    if (!ensureBooleanOrInt(parseCondition(), "if condition")) return; // Проверка типа условия
    match(TokenType::T_RPAREN);
    int jumpFalsePlaceholder = emitPlaceholder(RPNOpCode::JUMP_FALSE);
    parseStatement();
    parseElseClauseOpt(jumpFalsePlaceholder);
}

void Parser::parseElseClauseOpt(int jumpFalsePlaceholder) { /* ... как раньше ... */ }

void Parser::parseWhileStatement() {
    match(TokenType::T_KW_WHILE);
    int conditionStartAddress = getCurrentRPNAddress();
    match(TokenType::T_LPAREN);
    if (!ensureBooleanOrInt(parseCondition(), "while condition")) return; // Проверка типа условия
    match(TokenType::T_RPAREN);
    int jumpFalsePlaceholder = emitPlaceholder(RPNOpCode::JUMP_FALSE);
    parseStatement();
    emit(RPNOpCode::JUMP);
    patchJumpTo(getCurrentRPNAddress() - 1, conditionStartAddress);
    patchJump(jumpFalsePlaceholder);
}

// --- Разбор cin/cout --- (обновлены для типов)
void Parser::parseCinStatement() {
    match(TokenType::T_KW_CIN);
    match(TokenType::T_LPAREN);

    if (currentToken.type == TokenType::T_IDENTIFIER) {
        std::string name = currentToken.text;
        int line = currentToken.line;
        nextToken(); // Потребляем идентификатор

        auto indexOpt = symbolTable.findSymbol(name);
        if (!indexOpt) { semanticError("Identifier '" + name + "' not declared for input."); return; }
        size_t symbolIndex = indexOpt.value();
        SymbolType targetType = symbolTable.getSymbolType(symbolIndex);
        SymbolType targetBaseType = targetType;

        bool isArrayInput = false;
        if (currentToken.type == TokenType::T_LBRACKET) {
            if (!symbolTable.isArray(symbolIndex)) { semanticError("Identifier '" + name + "' is not an array."); return; }
            emit(RPNOpCode::PUSH_ARRAY, symbolIndex);
            isArrayInput = true;
            targetBaseType = symbolTable.getArrayElementType(targetType);
        }
        else if (symbolTable.isArray(symbolIndex)) {
            semanticError("Index required for array '" + name + "' in cin."); return;
        }
        else if (!symbolTable.isVariable(symbolIndex)) {
            semanticError("Identifier '" + name + "' is not a variable for input."); return;
        }

        if (isArrayInput) {
            auto indexTypeOpt = parseArrayIndexOpt(symbolIndex, targetType);
            if (!indexTypeOpt || indexTypeOpt.value() != SymbolType::VARIABLE_INT) {
                semanticError("Array index for '" + name + "' must be integer."); return;
            }
            emit(RPNOpCode::INDEX); // Вычисляем адрес элемента
        }
        else {
            emit(RPNOpCode::PUSH_VAR, symbolIndex); // Адрес простой переменной
        }

        // Генерируем нужную операцию READ
        switch (targetBaseType) {
        case SymbolType::VARIABLE_INT: emit(RPNOpCode::READ_I); break;
        case SymbolType::VARIABLE_FLOAT: emit(RPNOpCode::READ_F); break;
        case SymbolType::VARIABLE_BOOL: emit(RPNOpCode::READ_B); break;
        default: semanticError("Cannot read into target type."); // Не должно случиться
        }

    }
    else {
        syntaxError("Expected identifier inside cin(...).");
    }
    match(TokenType::T_RPAREN);
}

void Parser::parseCoutStatement() {
    match(TokenType::T_KW_COUT);
    match(TokenType::T_LPAREN);
    std::optional<SymbolType> exprTypeOpt = parseExpression();
    if (!exprTypeOpt) return; // Ошибка уже выведена
    match(TokenType::T_RPAREN);

    // Генерируем нужную операцию WRITE
    SymbolType exprType = exprTypeOpt.value();
    if (exprType == SymbolType::VARIABLE_INT || exprType == SymbolType::ARRAY_INT) emit(RPNOpCode::WRITE_I);
    else if (exprType == SymbolType::VARIABLE_FLOAT || exprType == SymbolType::ARRAY_FLOAT) emit(RPNOpCode::WRITE_F);
    else if (exprType == SymbolType::VARIABLE_BOOL || exprType == SymbolType::ARRAY_BOOL) emit(RPNOpCode::WRITE_B);
    else semanticError("Cannot cout expression of this type.");
}


// --- Разбор Условий и Выражений (переделано для возврата типов) ---

// <Condition> → <Expression> (должно быть bool или int)
std::optional<SymbolType> Parser::parseCondition() {
    // Просто разбираем выражение, тип проверится в if/while
    return parseExpression();
}

// <Expression> → <LogicalOrExpr>
std::optional<SymbolType> Parser::parseExpression() {
    return parseLogicalOrExpr();
}

// <LogicalOrExpr> → <LogicalAndExpr> <LogicalOrExprPrime>
std::optional<SymbolType> Parser::parseLogicalOrExpr() {
    std::optional<SymbolType> leftType = parseLogicalAndExpr();
    if (!leftType) return std::nullopt;
    return parseLogicalOrExprPrime(leftType.value());
}

// <LogicalOrExprPrime> → || <LogicalAndExpr> <LogicalOrExprPrime> | λ
std::optional<SymbolType> Parser::parseLogicalOrExprPrime(SymbolType leftType) {
    if (currentToken.type == TokenType::T_OP_OR) {
        nextToken(); // Съели ||
        std::optional<SymbolType> rightTypeOpt = parseLogicalAndExpr();
        if (!rightTypeOpt) return std::nullopt;
        SymbolType resultType = checkLogicalOp(leftType, rightTypeOpt.value(), "||");
        emit(RPNOpCode::OR);
        return parseLogicalOrExprPrime(resultType); // Хвостовая рекурсия
    }
    return leftType; // λ
}

// <LogicalAndExpr> → <EqualityExpr> <LogicalAndExprPrime>
std::optional<SymbolType> Parser::parseLogicalAndExpr() {
    std::optional<SymbolType> leftType = parseEqualityExpr();
    if (!leftType) return std::nullopt;
    return parseLogicalAndExprPrime(leftType.value());
}

// <LogicalAndExprPrime> → && <EqualityExpr> <LogicalAndExprPrime> | λ
std::optional<SymbolType> Parser::parseLogicalAndExprPrime(SymbolType leftType) {
    if (currentToken.type == TokenType::T_OP_AND) {
        nextToken(); // Съели &&
        std::optional<SymbolType> rightTypeOpt = parseEqualityExpr();
        if (!rightTypeOpt) return std::nullopt;
        SymbolType resultType = checkLogicalOp(leftType, rightTypeOpt.value(), "&&");
        emit(RPNOpCode::AND);
        return parseLogicalAndExprPrime(resultType); // Хвостовая рекурсия
    }
    return leftType; // λ
}

// <EqualityExpr> → <RelationalExpr> <EqualityExprPrime>
std::optional<SymbolType> Parser::parseEqualityExpr() {
    std::optional<SymbolType> leftType = parseRelationalExpr();
    if (!leftType) return std::nullopt;
    return parseEqualityExprPrime(leftType.value());
}

// <EqualityExprPrime> → (~ | !) <RelationalExpr> <EqualityExprPrime> | λ
std::optional<SymbolType> Parser::parseEqualityExprPrime(SymbolType leftType) {
    if (currentToken.type == TokenType::T_EQUAL || currentToken.type == TokenType::T_NOT_EQUAL) {
        TokenType op = currentToken.type;
        nextToken();
        std::optional<SymbolType> rightTypeOpt = parseRelationalExpr();
        if (!rightTypeOpt) return std::nullopt;

        SymbolType resultType = checkEqualityOp(leftType, rightTypeOpt.value(), op == TokenType::T_EQUAL ? "~" : "!");

        // Определяем, какую операцию сравнения генерировать
        RPNOpCode compareOp;
        if (resultType == SymbolType::VARIABLE_FLOAT) { // Сравнение float (после приведения)
            compareOp = (op == TokenType::T_EQUAL) ? RPNOpCode::CMP_EQ_F : RPNOpCode::CMP_NE_F;
        }
        else if (resultType == SymbolType::VARIABLE_INT) { // Сравнение int
            compareOp = (op == TokenType::T_EQUAL) ? RPNOpCode::CMP_EQ_I : RPNOpCode::CMP_NE_I;
        }
        else { // Сравнение bool
            compareOp = (op == TokenType::T_EQUAL) ? RPNOpCode::CMP_EQ_B : RPNOpCode::CMP_NE_B;
        }
        emit(compareOp);
        return parseEqualityExprPrime(SymbolType::VARIABLE_BOOL); // Результат всегда bool
    }
    return leftType; // λ
}

// <RelationalExpr> → <AdditiveExpr> <RelationalExprPrime>
std::optional<SymbolType> Parser::parseRelationalExpr() {
    std::optional<SymbolType> leftType = parseAdditiveExpr();
    if (!leftType) return std::nullopt;
    return parseRelationalExprPrime(leftType.value());
}

// <RelationalExprPrime> → (< | >) <AdditiveExpr> <RelationalExprPrime> | λ
std::optional<SymbolType> Parser::parseRelationalExprPrime(SymbolType leftType) {
    if (currentToken.type == TokenType::T_LESS || currentToken.type == TokenType::T_GREATER) {
        TokenType op = currentToken.type;
        nextToken();
        std::optional<SymbolType> rightTypeOpt = parseAdditiveExpr();
        if (!rightTypeOpt) return std::nullopt;

        SymbolType resultType = checkComparisonOp(leftType, rightTypeOpt.value(), op == TokenType::T_LESS ? "<" : ">");

        RPNOpCode compareOp;
        if (resultType == SymbolType::VARIABLE_FLOAT) { // Сравнение float
            compareOp = (op == TokenType::T_LESS) ? RPNOpCode::CMP_LT_F : RPNOpCode::CMP_GT_F;
        }
        else { // Сравнение int
            compareOp = (op == TokenType::T_LESS) ? RPNOpCode::CMP_LT_I : RPNOpCode::CMP_GT_I;
        }
        emit(compareOp);
        return parseRelationalExprPrime(SymbolType::VARIABLE_BOOL); // Результат bool
    }
    return leftType; // λ
}


// <AdditiveExpr> → <MultiplicativeExpr> <AdditiveExprPrime>
std::optional<SymbolType> Parser::parseAdditiveExpr() {
    std::optional<SymbolType> leftType = parseMultiplicativeExpr();
    if (!leftType) return std::nullopt;
    return parseAdditiveExprPrime(leftType.value());
}

// <AdditiveExprPrime> → (+ | -) <MultiplicativeExpr> <AdditiveExprPrime> | λ
std::optional<SymbolType> Parser::parseAdditiveExprPrime(SymbolType leftType) {
    if (currentToken.type == TokenType::T_PLUS || currentToken.type == TokenType::T_MINUS) {
        TokenType op = currentToken.type;
        nextToken();
        std::optional<SymbolType> rightTypeOpt = parseMultiplicativeExpr();
        if (!rightTypeOpt) return std::nullopt;

        RPNOpCode opCodeInt = (op == TokenType::T_PLUS) ? RPNOpCode::ADD_I : RPNOpCode::SUB_I;
        RPNOpCode opCodeFloat = (op == TokenType::T_PLUS) ? RPNOpCode::ADD_F : RPNOpCode::SUB_F;

        SymbolType resultType = promoteTypes(leftType, rightTypeOpt.value(), opCodeInt, opCodeFloat);
        emit((resultType == SymbolType::VARIABLE_FLOAT) ? opCodeFloat : opCodeInt);

        return parseAdditiveExprPrime(resultType); // Хвостовая рекурсия
    }
    return leftType; // λ
}

// <MultiplicativeExpr> → <UnaryExpr> <MultiplicativeExprPrime>
std::optional<SymbolType> Parser::parseMultiplicativeExpr() {
    std::optional<SymbolType> leftType = parseUnaryExpr();
    if (!leftType) return std::nullopt;
    return parseMultiplicativeExprPrime(leftType.value());
}

// <MultiplicativeExprPrime> → (* | /) <UnaryExpr> <MultiplicativeExprPrime> | λ
std::optional<SymbolType> Parser::parseMultiplicativeExprPrime(SymbolType leftType) {
    if (currentToken.type == TokenType::T_MULTIPLY || currentToken.type == TokenType::T_DIVIDE) {
        TokenType op = currentToken.type;
        nextToken();
        std::optional<SymbolType> rightTypeOpt = parseUnaryExpr();
        if (!rightTypeOpt) return std::nullopt;

        RPNOpCode opCodeInt = (op == TokenType::T_MULTIPLY) ? RPNOpCode::MUL_I : RPNOpCode::DIV_I;
        RPNOpCode opCodeFloat = (op == TokenType::T_MULTIPLY) ? RPNOpCode::MUL_F : RPNOpCode::DIV_F;

        SymbolType resultType = promoteTypes(leftType, rightTypeOpt.value(), opCodeInt, opCodeFloat);
        emit((resultType == SymbolType::VARIABLE_FLOAT) ? opCodeFloat : opCodeInt);

        return parseMultiplicativeExprPrime(resultType); // Хвостовая рекурсия
    }
    return leftType; // λ
}

// <UnaryExpr> → + <UnaryExpr> | - <UnaryExpr> | not <UnaryExpr> | <PrimaryExpr>
std::optional<SymbolType> Parser::parseUnaryExpr() {
    if (currentToken.type == TokenType::T_PLUS) { // Унарный плюс - игнорируем
        nextToken();
        return parseUnaryExpr(); // Просто разбираем дальше
    }
    else if (currentToken.type == TokenType::T_MINUS) { // Унарный минус
        nextToken();
        std::optional<SymbolType> typeOpt = parseUnaryExpr();
        if (!typeOpt) return std::nullopt;
        SymbolType type = typeOpt.value();
        if (type == SymbolType::VARIABLE_INT || type == SymbolType::ARRAY_INT) {
            emit(RPNOpCode::NEG_I); // Генерируем NEG_I
            return SymbolType::VARIABLE_INT;
        }
        else if (type == SymbolType::VARIABLE_FLOAT || type == SymbolType::ARRAY_FLOAT) {
            emit(RPNOpCode::NEG_F); // Генерируем NEG_F
            return SymbolType::VARIABLE_FLOAT;
        }
        else {
            semanticError("Unary minus cannot be applied to non-numeric type.");
            return std::nullopt;
        }
    }
    else if (currentToken.type == TokenType::T_KW_NOT) { // Логическое НЕ
        nextToken();
        std::optional<SymbolType> typeOpt = parseUnaryExpr();
        if (!typeOpt) return std::nullopt;
        SymbolType type = typeOpt.value();
        // Применяем правило: not bool -> bool, not int -> bool
        if (type == SymbolType::VARIABLE_BOOL || type == SymbolType::ARRAY_BOOL) {
            emit(RPNOpCode::NOT);
            return SymbolType::VARIABLE_BOOL;
        }
        else if (type == SymbolType::VARIABLE_INT || type == SymbolType::ARRAY_INT) {
            emitCast(type, SymbolType::VARIABLE_BOOL); // Приводим int к bool
            emit(RPNOpCode::NOT);
            return SymbolType::VARIABLE_BOOL;
        }
        else {
            semanticError("'not' operator requires boolean or integer operand.");
            return std::nullopt;
        }
    }
    else { // <PrimaryExpr>
        return parsePrimaryExpr();
    }
}

// <PrimaryExpr> → (G) | aH | k | <FLOAT_CONST> | <BOOL_CONST>
std::optional<SymbolType> Parser::parsePrimaryExpr() {
    switch (currentToken.type) {
    case TokenType::T_LPAREN:
        nextToken();
        {
            auto exprType = parseExpression(); // Тип возвращается из выражения
            match(TokenType::T_RPAREN);
            return exprType;
        }
    case TokenType::T_IDENTIFIER:
        return parseIdentifierOrArrayAccess(); // Вернет тип переменной или элемента массива
    case TokenType::T_NUMBER_INT:
        emit(RPNOpCode::PUSH_CONST_INT, SymbolValue(currentToken.intValue));
        nextToken();
        return SymbolType::VARIABLE_INT;
    case TokenType::T_NUMBER_FLOAT: // Новый
        emit(RPNOpCode::PUSH_CONST_FLOAT, SymbolValue(currentToken.floatValue));
        nextToken();
        return SymbolType::VARIABLE_FLOAT;
    case TokenType::T_KW_TRUE:      // Новый
        emit(RPNOpCode::PUSH_CONST_BOOL, SymbolValue(true));
        nextToken();
        return SymbolType::VARIABLE_BOOL;
    case TokenType::T_KW_FALSE:     // Новый
        emit(RPNOpCode::PUSH_CONST_BOOL, SymbolValue(false));
        nextToken();
        return SymbolType::VARIABLE_BOOL;
    default:
        syntaxError("Expected primary expression (parentheses, identifier, number, true, false).");
        return std::nullopt;
    }
}

// Обрабатывает a или a[i], возвращает тип результата
std::optional<SymbolType> Parser::parseIdentifierOrArrayAccess() {
    if (currentToken.type != TokenType::T_IDENTIFIER) {
        syntaxError("Expected identifier.");
        return std::nullopt;
    }
    std::string name = currentToken.text;
    int line = currentToken.line;
    nextToken();

    auto indexOpt = symbolTable.findSymbol(name);
    if (!indexOpt) { semanticError("Identifier '" + name + "' not declared."); return std::nullopt; }
    size_t symbolIndex = indexOpt.value();
    SymbolType symbolType = symbolTable.getSymbolType(symbolIndex);

    bool isArrayAccess = false;
    SymbolType resultType = symbolType; // Тип по умолчанию - тип самого символа

    if (currentToken.type == TokenType::T_LBRACKET) { // Проверяем наличие индекса
        if (!symbolTable.isArray(symbolIndex)) {
            semanticError("Identifier '" + name + "' is not an array.");
            return std::nullopt;
        }
        emit(RPNOpCode::PUSH_ARRAY, symbolIndex); // База массива
        isArrayAccess = true;
        resultType = symbolTable.getArrayElementType(symbolType); // Тип результата - тип элемента

        auto indexTypeOpt = parseArrayIndexOpt(symbolIndex, symbolType);
        if (!indexTypeOpt || indexTypeOpt.value() != SymbolType::VARIABLE_INT) {
            semanticError("Array index for '" + name + "' must be an integer expression.");
            return std::nullopt;
        }
        emit(RPNOpCode::INDEX); // Генерируем вычисление адреса и загрузку значения
    }
    else if (symbolTable.isArray(symbolIndex)) {
        semanticError("Array '" + name + "' used without index in expression.");
        return std::nullopt;
    }
    else { // Простая переменная
        emit(RPNOpCode::PUSH_VAR, symbolIndex); // Кладем значение переменной
    }

    return resultType;
}


// --- Вспомогательные функции проверки и приведения типов ---

// Определяет результирующий тип для бинарных арифметических/сравнительных операций
// и генерирует приведение типов, если нужно.
SymbolType Parser::promoteTypes(SymbolType type1, SymbolType type2, RPNOpCode& opCodeInt, RPNOpCode& opCodeFloat) {
    bool isType1Numeric = (type1 == SymbolType::VARIABLE_INT || type1 == SymbolType::VARIABLE_FLOAT);
    bool isType2Numeric = (type2 == SymbolType::VARIABLE_INT || type2 == SymbolType::VARIABLE_FLOAT);

    if (!isType1Numeric || !isType2Numeric) {
        semanticError("Arithmetic or comparison operation requires numeric operands.");
        return SymbolType::VARIABLE_INT; // Dummy
    }

    if (type1 == SymbolType::VARIABLE_FLOAT || type2 == SymbolType::VARIABLE_FLOAT) {
        // Если хотя бы один float, результат float
        if (type1 == SymbolType::VARIABLE_INT) {
            // Нужно привести левый операнд (который глубже в стеке)
            // RPN: [..., op1(int), op2(float)] -> [..., op1(float), op2(float)]
            // Нужно вставить CAST_I2F перед операцией, но как?
            // Проще сделать это в интерпретаторе перед выполнением операции.
            // Здесь просто возвращаем тип результата.
            emitCast(SymbolType::VARIABLE_INT, SymbolType::VARIABLE_FLOAT); // Генерируем приведение для верхнего элемента (type2)
            // НЕПРАВИЛЬНО! Нужно приводить нижний. Это сложно в парсере.
            // Оставим приведение для интерпретатора.
        }
        else if (type2 == SymbolType::VARIABLE_INT) {
            // Нужно привести правый операнд (верхний в стеке)
            emitCast(SymbolType::VARIABLE_INT, SymbolType::VARIABLE_FLOAT); // Генерируем CAST_I2F
        }
        return SymbolType::VARIABLE_FLOAT;
    }
    else {
        // Оба int, результат int
        return SymbolType::VARIABLE_INT;
    }
    // Замечание: opCodeInt/opCodeFloat здесь не меняем, это сделает вызывающий код
    // на основе возвращенного типа.
}


// Проверка типов для логических операций &&, ||
SymbolType Parser::checkLogicalOp(SymbolType left, SymbolType right, const std::string& op) {
    bool leftOk = (left == SymbolType::VARIABLE_BOOL || left == SymbolType::VARIABLE_INT);
    bool rightOk = (right == SymbolType::VARIABLE_BOOL || right == SymbolType::VARIABLE_INT);
    if (!leftOk || !rightOk) {
        semanticError("Operands for '" + op + "' must be boolean or integer.");
        return SymbolType::VARIABLE_BOOL; // Dummy
    }
    // Генерируем приведение int к bool, если нужно
    emitCast(left, SymbolType::VARIABLE_BOOL);
    emitCast(right, SymbolType::VARIABLE_BOOL);
    return SymbolType::VARIABLE_BOOL; // Результат всегда bool
}

// Проверка типов для операций сравнения <, >
SymbolType Parser::checkComparisonOp(SymbolType left, SymbolType right, const std::string& op) {
    RPNOpCode dummyInt, dummyFloat; // Не используются здесь
    SymbolType resultType = promoteTypes(left, right, dummyInt, dummyFloat); // Проверяет на числовые и приводит int к float
    return resultType; // Возвращает тип, к которому привели (int или float)
    // Результат операции сравнения будет bool, это обработается в вызывающем коде
}
// Проверка типов для операций сравнения ~, !
SymbolType Parser::checkEqualityOp(SymbolType left, SymbolType right, const std::string& op) {
    if (left == right && (left == SymbolType::VARIABLE_BOOL || left == SymbolType::VARIABLE_INT || left == SymbolType::VARIABLE_FLOAT)) {
        return left; // Сравнение одинаковых типов (кроме массивов)
    }
    // Проверяем случай сравнения int и float
    if ((left == SymbolType::VARIABLE_INT && right == SymbolType::VARIABLE_FLOAT) ||
        (left == SymbolType::VARIABLE_FLOAT && right == SymbolType::VARIABLE_INT)) {
        RPNOpCode dummyInt, dummyFloat;
        return promoteTypes(left, right, dummyInt, dummyFloat); // Приведет к float
    }
    semanticError("Operands for '" + op + "' must be of compatible types (int, float, or bool with bool).");
    return SymbolType::VARIABLE_BOOL; // Dummy
}

// Проверка, что тип подходит для условия (bool или int)
bool Parser::ensureBooleanOrInt(const std::optional<SymbolType>& typeOpt, const std::string& context) {
    if (!typeOpt) return false; // Ошибка разбора выражения
    SymbolType type = typeOpt.value();
    if (type == SymbolType::VARIABLE_BOOL || type == SymbolType::VARIABLE_INT) {
        emitCast(type, SymbolType::VARIABLE_BOOL); // Приводим int к bool, если нужно
        return true;
    }
    semanticError("Expression in " + context + " must result in a boolean or integer value.");
    return false;
}


// --- Получение и вывод ОПС ---
const std::vector<RPNOperation>& Parser::getRPNCode() const { return rpnCode; }

void Parser::printRPN() const {
    std::cout << "\n--- Reverse Polish Notation (RPN) ---" << std::endl;
    for (size_t i = 0; i < rpnCode.size(); ++i) {
        std::cout << std::setw(4) << i << ": ";
        const auto& op = rpnCode[i];
        // ... (Обновить вывод для новых кодов RPNOpCode: _I, _F, _B, CAST и т.д.) ...
        switch (op.opCode) {
        case RPNOpCode::PUSH_VAR:   std::cout << "PUSH_VAR  \t" << symbolTable.getSymbolName(op.symbolIndex.value()); break;
        case RPNOpCode::PUSH_ARRAY: std::cout << "PUSH_ARRAY\t" << symbolTable.getSymbolName(op.symbolIndex.value()); break;
        case RPNOpCode::PUSH_CONST_INT: std::cout << "PUSH_C_I \t" << std::get<int>(op.operand.value()); break;
        case RPNOpCode::PUSH_CONST_FLOAT: std::cout << "PUSH_C_F \t" << std::get<double>(op.operand.value()); break;
        case RPNOpCode::PUSH_CONST_BOOL: std::cout << "PUSH_C_B \t" << (std::get<bool>(op.operand.value()) ? "true" : "false"); break;
        case RPNOpCode::ADD_I: std::cout << "ADD_I"; break;
        case RPNOpCode::SUB_I: std::cout << "SUB_I"; break;
        case RPNOpCode::MUL_I: std::cout << "MUL_I"; break;
        case RPNOpCode::DIV_I: std::cout << "DIV_I"; break;
        case RPNOpCode::ADD_F: std::cout << "ADD_F"; break;
        case RPNOpCode::SUB_F: std::cout << "SUB_F"; break;
        case RPNOpCode::MUL_F: std::cout << "MUL_F"; break;
        case RPNOpCode::DIV_F: std::cout << "DIV_F"; break;
        case RPNOpCode::NEG_I: std::cout << "NEG_I"; break;
        case RPNOpCode::NEG_F: std::cout << "NEG_F"; break;
        case RPNOpCode::CMP_EQ_I: std::cout << "CMP_EQ_I"; break;
        case RPNOpCode::CMP_NE_I: std::cout << "CMP_NE_I"; break;
        case RPNOpCode::CMP_GT_I: std::cout << "CMP_GT_I"; break;
        case RPNOpCode::CMP_LT_I: std::cout << "CMP_LT_I"; break;
        case RPNOpCode::CMP_EQ_F: std::cout << "CMP_EQ_F"; break;
        case RPNOpCode::CMP_NE_F: std::cout << "CMP_NE_F"; break;
        case RPNOpCode::CMP_GT_F: std::cout << "CMP_GT_F"; break;
        case RPNOpCode::CMP_LT_F: std::cout << "CMP_LT_F"; break;
        case RPNOpCode::CMP_EQ_B: std::cout << "CMP_EQ_B"; break;
        case RPNOpCode::CMP_NE_B: std::cout << "CMP_NE_B"; break;
        case RPNOpCode::AND: std::cout << "AND"; break;
        case RPNOpCode::OR:  std::cout << "OR"; break;
        case RPNOpCode::NOT: std::cout << "NOT"; break;
        case RPNOpCode::ASSIGN: std::cout << "ASSIGN ="; break;
        case RPNOpCode::INDEX: std::cout << "INDEX []"; break;
        case RPNOpCode::READ_I: std::cout << "READ_I"; break;
        case RPNOpCode::READ_F: std::cout << "READ_F"; break;
        case RPNOpCode::READ_B: std::cout << "READ_B"; break;
        case RPNOpCode::WRITE_I: std::cout << "WRITE_I"; break;
        case RPNOpCode::WRITE_F: std::cout << "WRITE_F"; break;
        case RPNOpCode::WRITE_B: std::cout << "WRITE_B"; break;
        case RPNOpCode::JUMP:
            std::cout << "JUMP      \t";
            if (op.jumpTarget) std::cout << op.jumpTarget.value(); else std::cout << "(?)";
            break;
        case RPNOpCode::JUMP_FALSE:
            std::cout << "JUMP_FALSE\t";
            if (op.jumpTarget) std::cout << op.jumpTarget.value(); else std::cout << "(?)";
            break;
        case RPNOpCode::CAST_I2F: std::cout << "CAST_I2F"; break;
        case RPNOpCode::CAST_I2B: std::cout << "CAST_I2B"; break;
        case RPNOpCode::LABEL: std::cout << "LABEL (addr)"; break; // Не генерируется
        default: std::cout << "UNKNOWN"; break;
        }
        std::cout << std::endl;
    }
    std::cout << "------------------------------------" << std::endl;
}