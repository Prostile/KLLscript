#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream> // ��� ������ ����� � ������

#include "definitions.h"
#include "DataStructures.h"
#include "SymbolTable.h"
#include "Lexer.h"
#include "Parser.h"
#include "Interpreter.h"
// #include "ErrorHandler.h" // � �������

int main(int argc, char* argv[]) {
    // --- 1. ��������� ���������� � ������ ����� ---
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

    // ������ ���� ���� � ������ (����� ������� � ���������� ������, ���� ����� ����� �������)
    std::stringstream buffer;
    buffer << sourceFile.rdbuf();
    std::string sourceCode = buffer.str();
    sourceFile.close();

    // ��������� ������ ����� �����, ���� ��� ��� (��� �������� �������)
    // ������ ������� '$' ��� ������ �����
    // sourceCode += '$'; // ������ ��� ������������ ����� ������

    // --- 2. ������������� ����������� ---
    // ErrorHandler errorHandler; // ������� ���������� ������ (����� �� �����)
    SymbolTable symbolTable;   // ������� ������� ��������
    Lexer lexer(sourceCode, symbolTable /*, errorHandler*/); // ������� ������
    Parser parser(lexer, symbolTable /*, errorHandler*/);     // ������� ������

    // --- 3. ���� ���������� (����������� + �������������� ������ + ��������� ���) ---
    std::cout << "Starting compilation..." << std::endl;
    bool parseSuccess = parser.parse();

    if (!parseSuccess) {
        std::cerr << "Compilation failed due to syntax errors." << std::endl;
        // errorHandler.printSummary(); // � �������
        return 1; // ���������, ���� ������� �� ������
    }

    std::cout << "Parsing successful. RPN code generated." << std::endl;

    // (�����������) ����� ���������������� ��� ��� �������
    parser.printRPN();


    // --- 4. ���� ������������� ---
    std::cout << "\nStarting execution..." << std::endl;
    std::cout << "---------------------" << std::endl;

    // �������� ��������������� ��� ���
    const auto& rpnCode = parser.getRPNCode();

    // ������� �������������
    Interpreter interpreter(rpnCode, symbolTable /*, errorHandler*/);

    // ��������� ����������
    try {
        interpreter.execute();
    }
    catch (const std::exception& e) {
        std::cerr << "Caught exception during execution: " << e.what() << std::endl;
        // errorHandler.logRuntimeError(...) // � �������
        return 1;
    }
    catch (...) {
        std::cerr << "Caught unknown exception during execution." << std::endl;
        // errorHandler.logRuntimeError(...) // � �������
        return 1;
    }


    std::cout << "---------------------" << std::endl;
    std::cout << "Execution finished." << std::endl;

    // (�����������) ����� ������� �������� � ����� ��� �������
    // TODO: �������� ����� print() � SymbolTable

    return 0; // �������� ����������
}