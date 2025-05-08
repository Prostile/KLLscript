#include "Parser.h"
#include <iostream> // Для printRPN и временных ошибок
#include <iomanip>  // Для форматирования вывода RPN

// Конструктор
Parser::Parser(Lexer& lex, SymbolTable& symTab /*, ErrorHandler& errHandler*/)
    : lexer(lex), symbolTable(symTab) /*, errorHandler(errHandler)*/, declarationPhase(true) {
    // Получаем первый токен
    nextToken();
}

// --- Вспомогательные методы ---

void Parser::nextToken() {
    currentToken = lexer.getNextToken();
    // Пропускаем ошибки лексера, если нужно продолжать анализ
    while (currentToken.type == TokenType::T_ERROR) {
        // errorHandler.logError(...) или просто пропускаем
        currentToken = lexer.getNextToken();
    }
}

bool Parser::match(TokenType expectedType) {
    if (currentToken.type == expectedType) {
        nextToken();
        return true;
    }
    // Ошибка: Ожидался другой токен
    syntaxError("Expected token " + std::to_string(static_cast<int>(expectedType)) +
        " but found " + std::to_string(static_cast<int>(currentToken.type)));
    return false;
}

void Parser::syntaxError(const std::string& message) {
    // TODO: Использовать ErrorHandler
    std::cerr << "Syntax Error (Line " << currentToken.line << ", Col " << currentToken.column << "): "
        << message << ". Found token: " << currentToken.text
        << " (" << static_cast<int>(currentToken.type) << ")" << std::endl;
    // Пытаемся восстановиться (простейший вариант - пропуск до ';')
    // while (currentToken.type != TokenType::T_SEMICOLON && currentToken.type != TokenType::T_EOF) {
    //     nextToken();
    // }
    // Более сложная стратегия восстановления может понадобиться
    exit(EXIT_FAILURE); // Пока просто завершаем работу
}

void Parser::emit(RPNOpCode opCode) {
    rpnCode.emplace_back(opCode);
}

void Parser::emit(RPNOpCode opCode, const SymbolValue& value) {
    rpnCode.emplace_back(opCode, value);
}

void Parser::emit(RPNOpCode opCode, int symbolIndex) {
    rpnCode.emplace_back(opCode, symbolIndex);
}

int Parser::emitPlaceholder(RPNOpCode jumpType) {
    // Используем конструктор с bool для явного указания, что это переход
    rpnCode.emplace_back(jumpType, true);
    return static_cast<int>(rpnCode.size()) - 1; // Возвращаем индекс добавленной операции
}

void Parser::patchJump(int placeholderIndex) {
    if (placeholderIndex < 0 || placeholderIndex >= rpnCode.size()) {
        syntaxError("Internal error: Invalid jump placeholder index for patching.");
        return;
    }
    if (rpnCode[placeholderIndex].opCode != RPNOpCode::JUMP && rpnCode[placeholderIndex].opCode != RPNOpCode::JUMP_FALSE) {
        syntaxError("Internal error: Attempting to patch non-jump instruction.");
        return;
    }
    rpnCode[placeholderIndex].jumpTarget = getCurrentRPNAddress();
}
void Parser::patchJumpTo(int placeholderIndex, int targetAddress) {
    if (placeholderIndex < 0 || placeholderIndex >= rpnCode.size()) {
        syntaxError("Internal error: Invalid jump placeholder index for patching.");
        return;
    }
    if (rpnCode[placeholderIndex].opCode != RPNOpCode::JUMP && rpnCode[placeholderIndex].opCode != RPNOpCode::JUMP_FALSE) {
        syntaxError("Internal error: Attempting to patch non-jump instruction.");
        return;
    }
    rpnCode[placeholderIndex].jumpTarget = targetAddress;
}


int Parser::getCurrentRPNAddress() const {
    return static_cast<int>(rpnCode.size());
}


// --- Основной метод парсинга ---
bool Parser::parse() {
    try {
        parseProgram();
        if (currentToken.type != TokenType::T_EOF) {
            syntaxError("Expected EOF but found other tokens.");
            return false;
        }
        return true; // Успешный разбор
    }
    catch (...) { // Перехватываем исключения, если syntaxError будет их бросать
        return false;
    }
}

// --- Реализация методов рекурсивного спуска ---

// P → <DeclarationList> <StatementList> EOF
void Parser::parseProgram() {
    parseDeclarationList();
    declarationPhase = false; // Завершили фазу объявлений
    parseStatementList();
    // EOF проверяется в parse()
}

// <DeclarationList> → <Declaration> ; <DeclarationList> | λ
void Parser::parseDeclarationList() {
    // Определяем, есть ли объявления (смотрим на int или arr)
    while (currentToken.type == TokenType::T_KW_INT || currentToken.type == TokenType::T_KW_ARR) {
        parseDeclaration();
        if (!match(TokenType::T_SEMICOLON)) return; // Ожидаем ';' после объявления
    }
    // λ - если нет объявлений или они закончились
}

// <Declaration> → int <VarList> | arr <ArrList>
void Parser::parseDeclaration() {
    if (currentToken.type == TokenType::T_KW_INT) {
        nextToken();
        parseVarList(); // Разбираем список переменных int
    }
    else if (currentToken.type == TokenType::T_KW_ARR) {
        nextToken();
        parseArrList(); // Разбираем список массивов int
    }
    else {
        syntaxError("Expected 'int' or 'arr' for declaration.");
    }
}

// <VarList> → a <VarListTail>
void Parser::parseVarList() {
    // В исходной грамматике не было списка через запятую, обрабатываем только один идентификатор
    if (currentToken.type == TokenType::T_IDENTIFIER) {
        std::string name = currentToken.text;
        int line = currentToken.line;
        nextToken();
        if (!symbolTable.addVariable(name, SymbolType::VARIABLE_INT, line).has_value()) {
            syntaxError("Failed to declare variable '" + name + "' (maybe redeclaration?).");
        }
        // Здесь можно было бы вызвать parseVarListTail(), если бы был список через ','
    }
    else {
        syntaxError("Expected identifier in variable declaration.");
    }
}

// <ArrList> → a [ k ] <ArrListTail>
void Parser::parseArrList() {
    // Обрабатываем только один массив, как в VarList
    if (currentToken.type == TokenType::T_IDENTIFIER) {
        std::string name = currentToken.text;
        int line = currentToken.line;
        nextToken();

        if (!match(TokenType::T_LBRACKET)) return;

        if (currentToken.type == TokenType::T_NUMBER_INT) {
            int arraySize = currentToken.intValue;
            if (arraySize <= 0) {
                syntaxError("Array size must be a positive integer.");
                return;
            }
            nextToken();
            // Добавляем массив в таблицу символов
            auto indexOpt = symbolTable.addArray(name, SymbolType::ARRAY_INT, line);
            if (!indexOpt.has_value()) {
                syntaxError("Failed to declare array '" + name + "' (maybe redeclaration?).");
                return;
            }
            // Выделяем память (статически) - это задача интерпретатора,
            // но для статических массивов можно и здесь
            symbolTable.resizeArray(indexOpt.value(), arraySize);

        }
        else {
            syntaxError("Expected integer number for array size.");
            return;
        }

        if (!match(TokenType::T_RBRACKET)) return;
        // Здесь можно было бы вызвать parseArrListTail()
    }
    else {
        syntaxError("Expected identifier in array declaration.");
    }
}


// <StatementList> → <Statement> ; <StatementList> | λ
// Позволяет пустой список операторов
void Parser::parseStatementList() {
    // Пока не конец файла и не 'end' (если мы внутри begin-end)
    while (currentToken.type != TokenType::T_EOF && currentToken.type != TokenType::T_KW_END) {
        parseStatement();
        // Если оператор был не пустым (например, не просто ';'), ожидаем ';'
        // Простой способ: всегда требовать ';' если это не последний оператор перед 'end' или EOF
        if (currentToken.type != TokenType::T_KW_END && currentToken.type != TokenType::T_EOF) {
            if (!match(TokenType::T_SEMICOLON)) break; // Выходим, если нет ';'
            // Если после ';' сразу 'end' или EOF, это нормально (пустой оператор)
            if (currentToken.type == TokenType::T_KW_END || currentToken.type == TokenType::T_EOF) {
                break;
            }
        }
        else {
            break; // Достигли конца блока или файла
        }
    }
}

// <Statement> → <Assignment> | <IfStatement> | <WhileStatement> | <CinStatement> | <CoutStatement> | begin <StatementList> end | λ
void Parser::parseStatement() {
    switch (currentToken.type) {
    case TokenType::T_IDENTIFIER: {
        // Это может быть присваивание
        std::string name = currentToken.text;
        int line = currentToken.line;
        int col = currentToken.column;
        nextToken(); // Потребляем идентификатор
        parseAssignment(name, line, col);
        break;
    }
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
        nextToken(); // Потребляем 'begin'
        parseStatementList();
        match(TokenType::T_KW_END); // Ожидаем 'end'
        break;
    case TokenType::T_SEMICOLON:
        // Пустой оператор - ничего не делаем, ';' будет обработан в parseStatementList
        break;
    case TokenType::T_EOF:
    case TokenType::T_KW_END:
        // Достигли конца, это нормально для пустого оператора в конце списка
        break;
    default:
        syntaxError("Expected statement (identifier, if, while, cin, cout, begin).");
        break;
    }
}

// <Assignment> → a <ArrayIndexOpt> = <Expression>
// Идентификатор 'a' уже считан и передан как параметр
void Parser::parseAssignment(const std::string& identifierName, int line, int col) {
    auto indexOpt = symbolTable.findSymbol(identifierName);
    if (!indexOpt.has_value()) {
        // Используем позицию сохраненного токена
        std::cerr << "Syntax Error (Line " << line << ", Col " << col << "): Identifier '"
            << identifierName << "' not declared." << std::endl;
        exit(EXIT_FAILURE);
    }
    size_t symbolIndex = indexOpt.value();
    SymbolType symbolType = symbolTable.getSymbolType(symbolIndex);

    // Разбираем возможный индекс массива
    bool isArrayAssignment = false; // Флаг, что это присваивание элементу массива

    // Генерируем PUSH_ARRAY *ДО* разбора индекса, если это массив
    if (currentToken.type == TokenType::T_LBRACKET) { // Смотрим, есть ли '['
        if (symbolType != SymbolType::ARRAY_INT) {
            syntaxError("Identifier '" + identifierName + "' is not an array, cannot use index.");
            return;
        }
        emit(RPNOpCode::PUSH_ARRAY, symbolIndex); // Генерируем базу массива
        isArrayAssignment = true;
    }
    else { // Если нет '[', это должна быть простая переменная
        if (symbolType != SymbolType::VARIABLE_INT) {
            syntaxError("Identifier '" + identifierName + "' is not a simple variable.");
            return;
        }
        // PUSH_VAR для простой переменной генерируется ПОСЛЕ разбора выражения
    }

    // Разбираем возможный индекс массива (он сгенерирует PUSH для индекса)
    bool hasIndex = parseArrayIndexOpt(symbolIndex);

    // Проверка на случай a = ... (когда слева массив без индекса) - недопустимо
    if (symbolType == SymbolType::ARRAY_INT && !hasIndex) {
        syntaxError("Cannot assign to an entire array '" + identifierName + "'. Index is required.");
        return;
    }

    if (!match(TokenType::T_ASSIGN)) return;
    
    if (isArrayAssignment) {
        // PUSH_ARRAY уже сгенерирован
        // Выражение индекса уже сгенерировано parseArrayIndexOpt
        emit(RPNOpCode::INDEX); // Генерируем вычисление адреса элемента
    }
    else {
        // Простая переменная - генерируем PUSH_VAR только сейчас
        emit(RPNOpCode::PUSH_VAR, symbolIndex);
    }

    // Разбираем правую часть (выражение)
    parseExpression(); // Генерирует ОПС для вычисления значения

    // Генерируем операцию присваивания
    emit(RPNOpCode::ASSIGN);
}

// <ArrayIndexOpt> → [ <Expression> ] | λ
// Возвращает true, если индекс был разобран, и генерирует PUSH_CONST_INT для индекса
bool Parser::parseArrayIndexOpt(int identifierIndex) { // identifierIndex больше не нужен здесь
    if (currentToken.type == TokenType::T_LBRACKET) {
        nextToken(); // Потребляем '['
        parseExpression(); // Разбираем выражение индекса, оно попадет в ОПС
        match(TokenType::T_RBRACKET); // Потребляем ']'
        return true;
    }
    // λ - индекса нет
    return false;
}

// <IfStatement> → if ( <Condition> ) <Statement> <ElseClauseOpt>
void Parser::parseIfStatement() {
    match(TokenType::T_KW_IF);
    match(TokenType::T_LPAREN);
    parseCondition(); // Генерирует ОПС для условия
    match(TokenType::T_RPAREN);

    // Генерируем переход по лжи (jf), оставляем место для адреса
    int jumpFalsePlaceholder = emitPlaceholder(RPNOpCode::JUMP_FALSE);

    // Разбираем блок 'then'
    parseStatement();

    // Обрабатываем 'else'
    parseElseClauseOpt(jumpFalsePlaceholder);
}

// <ElseClauseOpt> → else <Statement> | λ
void Parser::parseElseClauseOpt(int jumpFalsePlaceholder) {
    if (currentToken.type == TokenType::T_KW_ELSE) {
        nextToken(); // Потребляем 'else'

        // Генерируем безусловный переход в конце 'then' блока, чтобы пропустить 'else'
        int jumpOverElsePlaceholder = emitPlaceholder(RPNOpCode::JUMP);

        // Заполняем адрес для перехода по лжи (jf) - он ведет на начало 'else'
        patchJump(jumpFalsePlaceholder);

        // Разбираем блок 'else'
        parseStatement();

        // Заполняем адрес для перехода в конце 'then' (j) - он ведет на конец 'if'
        patchJump(jumpOverElsePlaceholder);

    }
    else {
        // λ - нет 'else'
        // Заполняем адрес для перехода по лжи (jf) - он ведет на конец 'if'
        patchJump(jumpFalsePlaceholder);
    }
}

// <WhileStatement> → while ( <Condition> ) <Statement>
void Parser::parseWhileStatement() {
    match(TokenType::T_KW_WHILE);

    // Запоминаем адрес начала условия
    int conditionStartAddress = getCurrentRPNAddress();

    match(TokenType::T_LPAREN);
    parseCondition(); // Генерирует ОПС для условия
    match(TokenType::T_RPAREN);

    // Генерируем переход по лжи (jf) на конец цикла
    int jumpFalsePlaceholder = emitPlaceholder(RPNOpCode::JUMP_FALSE);

    // Разбираем тело цикла
    parseStatement();

    // Генерируем безусловный переход (j) на начало условия
    emit(RPNOpCode::JUMP); // Генерируем j
    patchJumpTo(getCurrentRPNAddress() - 1, conditionStartAddress); // Патчим его адрес

    // Заполняем адрес для перехода по лжи (jf) - он ведет на конец цикла
    patchJump(jumpFalsePlaceholder);
}

// <CinStatement> → cin ( a <ArrayIndexOpt> )
void Parser::parseCinStatement() {
    match(TokenType::T_KW_CIN);
    match(TokenType::T_LPAREN);

    if (currentToken.type == TokenType::T_IDENTIFIER) {
        std::string name = currentToken.text;
        int line = currentToken.line;
        int col = currentToken.column;
        nextToken();

        auto indexOpt = symbolTable.findSymbol(name);
        if (!indexOpt.has_value()) {
            std::cerr << "Syntax Error (Line " << line << ", Col " << col << "): Identifier '"
                << name << "' not declared for input." << std::endl;
            exit(EXIT_FAILURE);
        }
        size_t symbolIndex = indexOpt.value();
        SymbolType symbolType = symbolTable.getSymbolType(symbolIndex);

        bool isArrayInput = false;
        if (symbolType == SymbolType::ARRAY_INT && currentToken.type == TokenType::T_LBRACKET) {
            emit(RPNOpCode::PUSH_ARRAY, symbolIndex); // Генерируем базу
            isArrayInput = true;
        }
        else if (symbolType == SymbolType::VARIABLE_INT && currentToken.type != TokenType::T_LBRACKET) {
            // Ок, простая переменная
        }
        else {
            // Ошибки типов или использования индекса
            syntaxError("Invalid target for cin."); // Упрощенное сообщение
            return;
        }

        // Разбираем возможный индекс массива
        bool hasIndex = parseArrayIndexOpt(symbolIndex);

        if (symbolType == SymbolType::ARRAY_INT && !hasIndex) {
            syntaxError("Cannot read into an entire array '" + name + "'. Index is required.");
            return;
        }

        // Генерируем ОПС для адреса
        if (isArrayInput) {
            emit(RPNOpCode::INDEX); // Вычисляем адрес элемента
        }
        else {
            emit(RPNOpCode::PUSH_VAR, symbolIndex); // Адрес простой переменной
        }
        // Генерируем операцию чтения
        emit(RPNOpCode::READ);

    }
    else {
        syntaxError("Expected identifier inside cin(...).");
    }

    match(TokenType::T_RPAREN);
}

// <CoutStatement> → cout ( <Expression> )
void Parser::parseCoutStatement() {
    match(TokenType::T_KW_COUT);
    match(TokenType::T_LPAREN);
    parseExpression(); // Генерирует ОПС для вычисления значения
    match(TokenType::T_RPAREN);
    // Генерируем операцию вывода
    emit(RPNOpCode::WRITE);
}


// <Condition> → <Expression> <ComparisonOp> <Expression>
void Parser::parseCondition() {
    parseExpression();       // Левый операнд
    RPNOpCode comparisonOp = parseComparisonOp(); // Оператор сравнения
    parseExpression();       // Правый операнд
    emit(comparisonOp);      // Генерируем операцию сравнения
}

// <ComparisonOp> → ~ | > | < | !
RPNOpCode Parser::parseComparisonOp() {
    switch (currentToken.type) {
    case TokenType::T_EQUAL:     nextToken(); return RPNOpCode::CMP_EQ;
    case TokenType::T_GREATER:   nextToken(); return RPNOpCode::CMP_GT;
    case TokenType::T_LESS:      nextToken(); return RPNOpCode::CMP_LT;
    case TokenType::T_NOT_EQUAL: nextToken(); return RPNOpCode::CMP_NE;
    default:
        syntaxError("Expected comparison operator (~, >, <, !).");
        return RPNOpCode::CMP_EQ; // Возвращаем что-то по умолчанию для продолжения
    }
}

// <Expression> → <Term> <ExpressionPrime>
void Parser::parseExpression() {
    parseTerm();
    parseExpressionPrime();
}

// <ExpressionPrime> → + <Term> <ExpressionPrime> | - <Term> <ExpressionPrime> | λ
void Parser::parseExpressionPrime() {
    while (currentToken.type == TokenType::T_PLUS || currentToken.type == TokenType::T_MINUS) {
        TokenType opType = currentToken.type;
        nextToken(); // Потребляем '+' или '-'
        parseTerm();
        // Генерируем соответствующую операцию ОПС
        emit(opType == TokenType::T_PLUS ? RPNOpCode::ADD : RPNOpCode::SUB);
    }
    // λ - если нет '+' или '-'
}

// <Term> → <Factor> <TermPrime>
void Parser::parseTerm() {
    parseFactor();
    parseTermPrime();
}

// <TermPrime> → * <Factor> <TermPrime> | / <Factor> <TermPrime> | λ
void Parser::parseTermPrime() {
    while (currentToken.type == TokenType::T_MULTIPLY || currentToken.type == TokenType::T_DIVIDE) {
        TokenType opType = currentToken.type;
        nextToken(); // Потребляем '*' или '/'
        parseFactor();
        // Генерируем соответствующую операцию ОПС
        emit(opType == TokenType::T_MULTIPLY ? RPNOpCode::MUL : RPNOpCode::DIV);
    }
    // λ - если нет '*' или '/'
}

// <Factor> → ( <Expression> ) | <IdentifierOrArray> | NUMBER_INT
void Parser::parseFactor() {
    switch (currentToken.type) {
        case TokenType::T_LPAREN:
            nextToken(); // Потребляем '('
            parseExpression();
            match(TokenType::T_RPAREN); // Потребляем ')'
            break;
        case TokenType::T_IDENTIFIER:
            parseIdentifierOrArray();
            break;
        case TokenType::T_NUMBER_INT:
            emit(RPNOpCode::PUSH_CONST_INT, SymbolValue(currentToken.intValue));
            nextToken(); // Потребляем число
            break;
        case TokenType::T_MINUS: // !!! ОБРАБОТКА УНАРНОГО МИНУСА !!!
            nextToken(); // Потребляем '-'
            emit(RPNOpCode::PUSH_CONST_INT, SymbolValue(0)); // Кладем 0
            parseFactor();
            emit(RPNOpCode::SUB); // Вычитаем значение из нуля (0 - value = -value)
            break;
        default:
            syntaxError("Expected factor (expression in parenthesis, identifier, array element, number, or unary minus).");
            break;
    }
}

// <IdentifierOrArray> -> a <ArrayIndexOpt>
void Parser::parseIdentifierOrArray() {
    if (currentToken.type != TokenType::T_IDENTIFIER) {
        syntaxError("Expected identifier.");
        return;
    }
    std::string name = currentToken.text;
    int line = currentToken.line;
    int col = currentToken.column;
    nextToken(); // Потребляем идентификатор

    auto indexOpt = symbolTable.findSymbol(name);
    if (!indexOpt.has_value()) {
        std::cerr << "Syntax Error (Line " << line << ", Col " << col << "): Identifier '"
            << name << "' not declared." << std::endl;
        exit(EXIT_FAILURE);
    }
    size_t symbolIndex = indexOpt.value();
    SymbolType symbolType = symbolTable.getSymbolType(symbolIndex);

    bool isArrayAccess = false;
    if (symbolType == SymbolType::ARRAY_INT && currentToken.type == TokenType::T_LBRACKET) {
        emit(RPNOpCode::PUSH_ARRAY, symbolIndex); // Генерируем базу массива
        isArrayAccess = true;
    }
    else if (symbolType == SymbolType::VARIABLE_INT && currentToken.type != TokenType::T_LBRACKET) {
        // Простая переменная - PUSH_VAR будет сгенерирован ниже
    }
    else if (symbolType == SymbolType::ARRAY_INT && currentToken.type != TokenType::T_LBRACKET) {
        syntaxError("Array '" + name + "' used without index.");
        return;
    }
    else if (symbolType == SymbolType::VARIABLE_INT && currentToken.type == TokenType::T_LBRACKET) {
        syntaxError("Variable '" + name + "' cannot be indexed.");
        return;
    }

    // Разбираем возможный индекс массива
    bool hasIndex = parseArrayIndexOpt(symbolIndex); // Генерирует ОПС для выражения индекса

    // Генерируем ОПС
    if (!isArrayAccess) { // Простая переменная
        emit(RPNOpCode::PUSH_VAR, symbolIndex);
    }
    else { // Элемент массива
        emit(RPNOpCode::INDEX); // Вычисляем адрес элемента
    }
}

// Получение сгенерированного кода ОПС
const std::vector<RPNOperation>& Parser::getRPNCode() const {
    return rpnCode;
}

// --- Вывод ОПС (для отладки) ---
void Parser::printRPN() const {
    std::cout << "\n--- Reverse Polish Notation (RPN) ---" << std::endl;
    for (size_t i = 0; i < rpnCode.size(); ++i) {
        std::cout << std::setw(4) << i << ": ";
        const auto& op = rpnCode[i];

        switch (op.opCode) {
        case RPNOpCode::PUSH_VAR:
            std::cout << "PUSH_VAR  \t" << symbolTable.getSymbolName(op.symbolIndex.value());
            break;
        case RPNOpCode::PUSH_ARRAY:
            std::cout << "PUSH_ARRAY\t" << symbolTable.getSymbolName(op.symbolIndex.value());
            break;
        case RPNOpCode::PUSH_CONST_INT:
            std::cout << "PUSH_CONST\t" << std::get<int>(op.operand.value());
            break;
        case RPNOpCode::ADD: std::cout << "ADD"; break;
        case RPNOpCode::SUB: std::cout << "SUB"; break;
        case RPNOpCode::MUL: std::cout << "MUL"; break;
        case RPNOpCode::DIV: std::cout << "DIV"; break;
        case RPNOpCode::CMP_EQ: std::cout << "CMP_EQ ~"; break;
        case RPNOpCode::CMP_NE: std::cout << "CMP_NE !"; break;
        case RPNOpCode::CMP_GT: std::cout << "CMP_GT >"; break;
        case RPNOpCode::CMP_LT: std::cout << "CMP_LT <"; break;
        case RPNOpCode::ASSIGN: std::cout << "ASSIGN ="; break;
        case RPNOpCode::INDEX: std::cout << "INDEX []"; break;
        case RPNOpCode::READ: std::cout << "READ cin"; break;
        case RPNOpCode::WRITE: std::cout << "WRITE cout"; break;
        case RPNOpCode::JUMP:
            std::cout << "JUMP      \t";
            if (op.jumpTarget.has_value() && op.jumpTarget.value() != -1) std::cout << op.jumpTarget.value(); else std::cout << "(?)";
            break;
        case RPNOpCode::JUMP_FALSE:
            std::cout << "JUMP_FALSE\t";
            if (op.jumpTarget.has_value() && op.jumpTarget.value() != -1) std::cout << op.jumpTarget.value(); else std::cout << "(?)";
            break;
        case RPNOpCode::LABEL: // Метки как таковые не генерируются, это просто адрес
            std::cout << "LABEL (addr)"; break;

        default: std::cout << "UNKNOWN"; break;
        }
        std::cout << std::endl;
    }
    std::cout << "------------------------------------" << std::endl;
}