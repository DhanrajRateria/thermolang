#include "thermolang/ir/ThermoIR.h"
#include <sstream>

namespace thermolang::ir
{

    std::string to_string(const Operand &operand)
    {
        std::stringstream ss;
        std::visit([&ss](auto &&arg)
                   { ss << arg; },
                   operand);
        return ss.str();
    }

    std::string to_string(const OpCode &opcode)
    {
        switch (opcode)
        {
        case OpCode::LOAD_CONST:
            return "load_const";
        case OpCode::ASSIGN:
            return "assign";
        case OpCode::ADD:
            return "add";
        case OpCode::SUB:
            return "sub";
        case OpCode::MUL:
            return "mul";
        case OpCode::DIV:
            return "div";
        case OpCode::JUMP:
            return "jump";
        case OpCode::BRANCH:
            return "branch";
        case OpCode::RETURN:
            return "return";
        case OpCode::CALL:
            return "call";
        default:
            return "unknown_op";
        }
    }

    std::string LoadConstInstr::toString() const
    {
        return "    " + result_reg + " = " + to_string(opcode) + " " + to_string(value);
    }

    std::string AssignInstr::toString() const
    {
        return "    " + dest_reg + " = " + to_string(opcode) + " " + to_string(source);
    }

    std::string BinaryOpInstr::toString() const
    {
        return "    " + result_reg + " = " + to_string(opcode) + " " + to_string(arg1) + ", " + to_string(arg2);
    }

    std::string ReturnInstr::toString() const
    {
        if (return_value)
        {
            return "    " + to_string(opcode) + " " + to_string(*return_value);
        }
        return "    " + to_string(opcode);
    }

    std::string CallInstr::toString() const
    {
        std::stringstream ss;
        if (result_reg)
        {
            ss << "    " << *result_reg << " = ";
        }
        else
        {
            ss << "    ";
        }

        ss << to_string(opcode) << " " << callee_name << "(";
        for (size_t i = 0; i < args.size(); ++i)
        {
            ss << to_string(args[i]);
            if (i < args.size() - 1)
            {
                ss << ", ";
            }
        }
        ss << ")";
        return ss.str();
    }

    std::string FunctionIR::toString() const
    {
        std::stringstream ss;
        ss << "function " << name << "(";
        for (size_t i = 0; i < parameters.size(); ++i)
        {
            ss << parameters[i];
            if (i < parameters.size() - 1)
            {
                ss << ", ";
            }
        }
        ss << ") {\n";

        for (const auto &block : basic_blocks)
        {
            ss << block->label << ":\n";
            for (const auto &instr : block->instructions)
            {
                ss << instr->toString() << "\n";
            }
        }
        ss << "}\n";
        return ss.str();
    }

} // namespace thermolang::ir