// parser.h
#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <stack>   // Для стека меток (адресов для патчинга)
#include <optional>

#include "definitions.h"    // TokenType, RPNOpCode, SymbolType
#include "token.h"          // Структура Token
#include "rpn_op.h"         // Структура RPNOperation
#include "lexer.h"          // Класс Lexer
#include "symbol_table.h"   // Класс SymbolTable
#include "error_handler.h"  // Класс ErrorHandler

class Parser {
private:
    Lexer& lexer;
    SymbolTable& symbolTable;
    ErrorHandler& errorHandler;

    Token currentToken;
    // Флаг для определения, находимся ли мы в фазе объявления типов
    // или уже разбираем исполняемый код. Помогает различать контекст.
    bool declarationContextActive;
    SymbolType lastDeclaredType; // Для запоминания типа при объявлении массивов (int arr A[10] или float arr B[5])


    std::vector<RPNOperation> rpnCode; // Генерируемый код ОПС

    // Вспомогательные методы
    void nextToken(); // Получить следующий токен от лексера
    bool match(TokenType expectedType); // Проверить тип текущего токена и перейти к следующему
    void consume(TokenType expectedType); // Как match, но без возвращаемого значения, просто требует совпадения

    // Обертки для ErrorHandler для удобства
    void reportSyntaxError(const std::string& message);
    void reportSemanticError(const std::string& message, int line = 0, int col = 0);

    // Генерация ОПС
    void emit(RPNOpCode opCode);
    void emit(RPNOpCode opCode, int value);      // Для PUSH_CONST_INT
    void emit(RPNOpCode opCode, float value);    // Для PUSH_CONST_FLOAT
    void emit(RPNOpCode opCode, size_t symbolIndex); // Для PUSH_VAR_ADDR, PUSH_ARRAY_ADDR
    void emit(RPNOpCode opCode, bool isIoPlaceholder); // Для READ/WRITE операций (для разрешения перегрузки)

    int emitJumpPlaceholder(RPNOpCode jumpOpCode); // Генерирует JUMP/JUMP_FALSE с -1, возвращает индекс
    void patchJump(int rpnAddressToPatch, int targetRpnAddress); // Патчит переход по указанному адресу
    int getCurrentRPNAddress() const; // Текущий "адрес" в rpnCode (равен rpnCode.size())

    // --- Методы рекурсивного спуска для разбора грамматики ---
    // Грамматика из вашей документации (с адаптациями для float и рекурсивного спуска)
    // P → <OptDeclarationList> begin A end EOF (A - StatementList)
    // <OptDeclarationList> → <Declaration> ; <OptDeclarationList> | λ
    // <Declaration> → int L | float L | arr <ArrayTypeSpec> M (L - VarList, M - ArrList)
    // <ArrayTypeSpec> -> int | float (Это новое, чтобы знать тип элементов массива)
    // L → a <VarListTail>
    // <VarListTail> → , a <VarListTail> | λ
    // M → a [ k ] <ArrListTail>
    // <ArrListTail> → , a [ k ] <ArrListTail> | λ
    // A → <Statement> <StatementTail> (StatementTail обрабатывает '; A' или 'λ')
    // <StatementTail> → ; A | λ
    // <Statement> → aH = G | if (C) <ScopedStatement> E_else | while (C) <ScopedStatement> | cin(aH) | cout(G) | begin A end | λ
    // <ScopedStatement> -> <Statement> | begin A end (чтобы if/while могли иметь один оператор или блок)
    // H → [G] | λ (индекс массива)
    // G → <Term> <ExpressionPrime> (Выражение)
    // <ExpressionPrime> → + <Term> <ExpressionPrime> | - <Term> <ExpressionPrime> | λ
    // <Term> → <Factor> <TermPrime>
    // <TermPrime> → * <Factor> <TermPrime> | / <Factor> <TermPrime> | λ
    // <Factor> → (G) | aH | k_int | k_float | -<Factor> (Унарный минус)
    // E_else → else <ScopedStatement> | λ
    // C → G <ComparisonOp> G (Условие)
    // <ComparisonOp> → ~ | > | < | !

    void parseProgram();
    void parseOptDeclarationList();
    void parseDeclaration();
    SymbolType parseTypeSpec(); // Для int, float
    SymbolType parseArrayTypeSpec(); // Для типа элементов массива после 'arr'
    void parseVarList(SymbolType varType);
    void parseVarListTail(SymbolType varType);
    void parseArrList(SymbolType elementType); // elementType - тип элементов массива
    void parseArrListTail(SymbolType elementType);

    void parseStatementList(); // A в грамматике
    void parseStatementTail();
    void parseStatement();
    void parseScopedStatement(); // Для then/else/while тел

    // Возвращает true, если был разобран индекс (и ОПС для него сгенерирован)
    // symbolInfo - информация о символе (массиве)
    bool parseArrayIndexOpt(const SymbolInfo& symbolInfo); // H в грамматике

    void parseAssignmentOrExpressionStatement(); // Разбирает либо aH=G, либо просто G (если будет разрешено)
    // Пока будет только aH=G

    void parseIfStatement();
    void parseElseClause(int jumpOverElsePlaceholder); // E_else
    void parseWhileStatement();
    void parseCinStatement();
    void parseCoutStatement();
    void parseBeginEndBlock();

    // Для выражений и условий
    SymbolType parseExpression(); // G, возвращает предполагаемый тип результата выражения
    SymbolType parseTerm();
    SymbolType parseFactor();
    SymbolType parseExpressionPrime(SymbolType leftOperandType); // Принимает тип левого операнда для проверки и преобразования
    SymbolType parseTermPrime(SymbolType leftOperandType);

    void parseCondition(); // C
    RPNOpCode parseComparisonOp();

    // Семантические действия и проверки
    // Проверяет, объявлен ли идентификатор, и возвращает его индекс и тип
    std::optional<std::pair<size_t, SymbolType>> checkIdentifier(const Token& idToken, bool isAssignmentTarget = false);
    // Выполняет преобразование типа на стеке ОПС, если необходимо
    void ensureTypesMatchOrConvert(SymbolType type1, SymbolType type2, bool forAssignment = false);


public:
    Parser(Lexer& lex, SymbolTable& symTab, ErrorHandler& errHandler);

    bool parse(); // Запуск парсинга
    const std::vector<RPNOperation>& getRPNCode() const; // Получение сгенерированного ОПС
    void printRPN() const; // Отладочный вывод ОПС
};

#endif // PARSER_H