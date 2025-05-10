// main.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

// ������������ ����� ������� �������
// ���� ��� �� ����������, ���������� ����� ����� �� �������,
// �� ��� �������� ��������� ���������.
#include "definitions.h"
#include "token.h" // ���� Token ����� � ��������� �����
#include "error_handler.h"
#include "symbol_table.h"
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "rpn_op.h" // ���� RPNOperation ����� � ��������� �����


int main(int argc, char* argv[]) {
    // 1. ��������� ���������� ��������� ������
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }

    std::string sourceFileName = argv[1];
    std::ifstream sourceFile(sourceFileName);

    if (!sourceFile.is_open()) {
        std::cerr << "Error: Could not open file '" << sourceFileName << "'" << std::endl;
        return 1;
    }

    // 2. ������ ��������� ���� �� �����
    std::stringstream buffer;
    buffer << sourceFile.rdbuf();
    std::string sourceCode = buffer.str();
    sourceFile.close();

    // 3. ������������� ����������� �����������
    ErrorHandler errorHandler; // ������� ���������� ������
    SymbolTable symbolTable(errorHandler);   // ������� ������� ��������
    Lexer lexer(sourceCode, symbolTable, errorHandler); // ������� ������
    Parser parser(lexer, symbolTable, errorHandler);     // ������� ������

    std::cout << "Starting compilation of file: " << sourceFileName << std::endl;

    // 4. ���� ���������� (����������� + �������������� ������ + ��������� ���)
    bool parseSuccess = parser.parse();

    if (!parseSuccess || errorHandler.hasErrors()) {
        std::cerr << "Compilation failed." << std::endl;
        errorHandler.printErrors(); // ������� ��� ����������� ������
        return 1; // ���������, ���� ������� �� ������ ��� ���� ������
    }

    std::cout << "Compilation successful. RPN code generated." << std::endl;

    // (�����������) ����� ���������������� ��� ��� �������
    parser.printRPN(); // ���� ����� ����� ����� ����������� � Parser

    // 5. ���� �������������
    std::cout << "\nStarting execution..." << std::endl;
    std::cout << "---------------------" << std::endl;

    const std::vector<RPNOperation>& rpnCode = parser.getRPNCode();

    // ��������, ��� ��� �� ����, ���� ������� ��� �������, �� ��� ���� (��������, ������ ���������)
    if (rpnCode.empty() && parseSuccess) {
        std::cout << "Program is empty. Nothing to execute." << std::endl;
        std::cout << "---------------------" << std::endl;
        std::cout << "Execution finished." << std::endl;
        return 0;
    }


    Interpreter interpreter(rpnCode, symbolTable, errorHandler);

    // ��������� ����������
    interpreter.execute();

    if (errorHandler.hasErrors()) {
        std::cerr << "Execution failed with runtime errors." << std::endl;
        errorHandler.printErrors(); // ������� ������ ������� ����������
        return 1;
    }

    std::cout << "---------------------" << std::endl;
    std::cout << "Execution finished." << std::endl;

    // (�����������) ����� ������� �������� � ����� ��� �������
    // symbolTable.print(); // ���� ����� ����� ����� ����������� � SymbolTable

    return 0; // �������� ����������
}