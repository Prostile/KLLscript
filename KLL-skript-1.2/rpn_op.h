// rpn_op.h
#ifndef RPN_OP_H
#define RPN_OP_H

#include <variant>
#include <optional>   // ��� ������������ �����
#include <string>     // ��� ����������� ������ ��� �������� ���� �����, ���� �����������

#include "definitions.h" // ����� RPNOpCode

// ��������� ��� �������� ��� (RPN - Reverse Polish Notation)
struct RPNOperation {
    RPNOpCode opCode; // ��� ��������

    // �������, ���� �������� ��� ������� (��������, ���������)
    // ���������� std::variant ��� �������� int ��� float.
    // std::monostate �������� ���������� ������ �������� �������� (��������, ��� ADD, SUB).
    std::variant<std::monostate, int, float> operandValue;

    // ������ ������� � ������� �������� (��� PUSH_VAR_ADDR, PUSH_ARRAY_ADDR)
    std::optional<size_t> symbolIndex;

    // ���� �������� (������ � ������� ���) ��� �������� JUMP, JUMP_FALSE
    // ���������������� -1 ��� ������������� ���������, ����� ��������, ��� ����� ��� �� ���������� (placeholder)
    std::optional<int> jumpTarget;

    // --- ������������ ---

    // ����������� ��� �������� ��� ������ ��������-�������� � ��� ������� �������
    // (��������, ADD, SUB, MUL, DIV, INDEX, CONVERT_TO_INT, CONVERT_TO_FLOAT)
    explicit RPNOperation(RPNOpCode code)
        : opCode(code), operandValue(std::monostate{}), symbolIndex(std::nullopt), jumpTarget(std::nullopt) {
    }

    // ����������� ��� �������� � ������������� ���������-��������� (PUSH_CONST_INT)
    RPNOperation(RPNOpCode code, int val)
        : opCode(code), operandValue(val), symbolIndex(std::nullopt), jumpTarget(std::nullopt) {
        // ��������, ��� ����������� ������������ ��� ����������� opCode
        // if (code != RPNOpCode::PUSH_CONST_INT) { /* ������ ������ ��� �������������� */ }
    }

    // ����������� ��� �������� � ������������ ���������-��������� (PUSH_CONST_FLOAT)
    RPNOperation(RPNOpCode code, float val)
        : opCode(code), operandValue(val), symbolIndex(std::nullopt), jumpTarget(std::nullopt) {
        // if (code != RPNOpCode::PUSH_CONST_FLOAT) { /* ������ ������ ��� �������������� */ }
    }

    // ����������� ��� ��������, ������������ ������ ������� � ������� ��������
    // (PUSH_VAR_ADDR, PUSH_ARRAY_ADDR)
    RPNOperation(RPNOpCode code, size_t symIdx)
        : opCode(code), operandValue(std::monostate{}), symbolIndex(symIdx), jumpTarget(std::nullopt) {
        // if (code != RPNOpCode::PUSH_VAR_ADDR && code != RPNOpCode::PUSH_ARRAY_ADDR) { /* ... */ }
    }

    // ����������� ��� �������� �����/������ (��� ����� �� ����� ������ ��������� � ���� ���������)
    // ���������� 'bool' ��� ��������� ��������, ����� �������� �� ������� ������������
    RPNOperation(RPNOpCode code, bool isIoOperationPlaceholder)
        : opCode(code), operandValue(std::monostate{}), symbolIndex(std::nullopt), jumpTarget(std::nullopt) {
        // ��� ���������� �����, ����� �������� ��������������� � RPNOperation(RPNOpCode code)
        // ����� �������� ��������, ��� code ��� READ_INT, READ_FLOAT, WRITE_INT, WRITE_FLOAT
        // if (code != RPNOpCode::READ_INT && code != RPNOpCode::READ_FLOAT &&
        //     code != RPNOpCode::WRITE_INT && code != RPNOpCode::WRITE_FLOAT) {
        //     // ������ ������, ���� ���� ����������� ������ �� ��� �������� �����/������
        // }
    }


    // ����������� ��� �������� �������� (JUMP, JUMP_FALSE) - ������� "��������"
    // ���������� 'int' ��� ��������� �������� placeholderTarget, ����� �������� �� ������
    // ��� ����� ���� �� ������������ ������ bool.
    // �������������� jumpTarget ��������� �� ��������� (��������, -1 ��� std::nullopt).
    RPNOperation(RPNOpCode code, int placeholderTarget, bool isJumpOperation)
        : opCode(code), operandValue(std::monostate{}), symbolIndex(std::nullopt) {
        if (code == RPNOpCode::JUMP || code == RPNOpCode::JUMP_FALSE) {
            jumpTarget = placeholderTarget; // ������ -1 ��� ����������� �������� ��� �������������� ������
            // ������ ����� ������������ std::nullopt ��� -1 ��� ���������� ��������
        }
        else {
            jumpTarget = std::nullopt; // �� �������, ���� ���
            // ����� �������� ��������� �� ������, ���� ���� ����������� ������ �� ��� JUMP/JUMP_FALSE
        }
    }
};

#endif // RPN_OP_H