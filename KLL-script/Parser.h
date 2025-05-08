#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <stack> // Для стека меток

#include "definitions.h"
#include "DataStructures.h" // Token, RPNOperation, SymbolInfo etc.
#include "Lexer.h"
#include "SymbolTable.h"
// #include "ErrorHandler.h"

// --- Класс Синтаксического Анализатора и Генератора ОПС ---
class Parser {
private:
    Lexer& lexer;                 // Ссылка на лексический анализатор
    SymbolTable& symbolTable;     // Ссылка на таблицу символов
    // ErrorHandler& errorHandler; // Ссылка на обработчик ошибок

    Token currentToken;           // Текущий токен от лексера
    bool declarationPhase;        // Флаг: находимся ли мы в фазе объявления

    std::vector<RPNOperation> rpnCode; // Генерируемый код ОПС
    std::stack<int> labelStack;      // Стек для хранения адресов для патчинга переходов

    // --- Вспомогательные методы ---
    // Получение следующего токена
    void nextToken();
    // Проверка текущего токена и переход к следующему
    bool match(TokenType expectedType);
    // Сообщение об ошибке синтаксиса
    void syntaxError(const std::string& message);
    // Генерация ОПС
    void emit(RPNOpCode opCode);
    void emit(RPNOpCode opCode, const SymbolValue& value); // Для констант
    void emit(RPNOpCode opCode, int symbolIndex);         // Для переменных/массивов
    int emitPlaceholder(RPNOpCode jumpType); // Генерирует операцию перехода с -1, возвращает ее индекс
    void patchJump(int placeholderIndex);    // Заполняет адрес перехода для placeholderIndex текущим адресом
    void patchJumpTo(int placeholderIndex, int targetAddress); // Заполняет адрес перехода указанным адресом
    int getCurrentRPNAddress() const;      // Получить текущий "адрес" (индекс) в rpnCode

    // --- Методы рекурсивного спуска для разбора грамматики ---
    // (Названия соответствуют нетерминалам преобразованной грамматики без левой рекурсии)

    // P → <DeclarationList> <StatementList> | <StatementList> (Упрощенная версия оригинальной P)
    void parseProgram();

    // <DeclarationList> → <Declaration> ; <DeclarationList> | λ
    void parseDeclarationList();
    // <Declaration> → int <VarList> | arr <ArrList>
    void parseDeclaration();
    // <VarList> → a <VarListTail>
    void parseVarList();
    // <VarListTail> → , a <VarListTail> | λ (Запятая не была в исходной грамматике, добавляем для стандарта)
    // void parseVarListTail(); // Или просто цикл в parseVarList, если без запятой
    // <ArrList> → a [ k ] <ArrListTail>
    void parseArrList();
    // <ArrListTail> → , a [ k ] <ArrListTail> | λ
    // void parseArrListTail(); // Или цикл

    // <StatementList> → <Statement> <StatementListTail>
    // <StatementListTail> → ; <Statement> <StatementListTail> | λ (Обработка ';' как разделителя)
    // Или A → <Statement> ; A | λ (Как в оригинале)
    void parseStatementList();
    // <Statement> → <Assignment> | <IfStatement> | <WhileStatement> | <CinStatement> | <CoutStatement> | begin <StatementList> end | λ
    void parseStatement();

    // <Assignment> → a <ArrayIndexOpt> = <Expression>
    void parseAssignment(const std::string& identifierName, int line, int col); // Принимает уже распознанный идентификатор
    // <ArrayIndexOpt> → [ <Expression> ] | λ
    bool parseArrayIndexOpt(int identifierIndex); // Возвращает true, если был индекс

    // <IfStatement> → if ( <Condition> ) <Statement> <ElseClauseOpt>
    void parseIfStatement();
    // <ElseClauseOpt> → else <Statement> | λ
    void parseElseClauseOpt(int jumpAfterThenPlaceholder); // Принимает метку для патчинга else

    // <WhileStatement> → while ( <Condition> ) <Statement>
    void parseWhileStatement();

    // <CinStatement> → cin ( a <ArrayIndexOpt> )
    void parseCinStatement();
    // <CoutStatement> → cout ( <Expression> )
    void parseCoutStatement();

    // <Condition> → <Expression> <ComparisonOp> <Expression>
    void parseCondition();
    // <ComparisonOp> → ~ | > | < | !
    RPNOpCode parseComparisonOp();

    // --- Разбор выражений (без левой рекурсии) ---
    // <Expression> → <Term> <ExpressionPrime>
    void parseExpression();
    // <ExpressionPrime> → + <Term> <ExpressionPrime> | - <Term> <ExpressionPrime> | λ
    void parseExpressionPrime();
    // <Term> → <Factor> <TermPrime>
    void parseTerm();
    // <TermPrime> → * <Factor> <TermPrime> | / <Factor> <TermPrime> | λ
    void parseTermPrime();
    // <Factor> → ( <Expression> ) | <IdentifierOrArray> | NUMBER_INT
    void parseFactor();
    // <IdentifierOrArray> -> a <ArrayIndexOpt>
    void parseIdentifierOrArray();


public:
    // Конструктор
    Parser(Lexer& lex, SymbolTable& symTab /*, ErrorHandler& errHandler*/);

    // Запуск парсинга и генерации ОПС
    bool parse();

    // Получение сгенерированного кода ОПС
    const std::vector<RPNOperation>& getRPNCode() const;

    // Вывод ОПС (для отладки)
    void printRPN() const;
};

#endif // PARSER_H