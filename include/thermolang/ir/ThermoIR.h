#ifndef THERMOLANG_IR_H
#define THERMOLANG_IR_H

#include "thermolang/types/Type.h"
#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace thermolang::ir
{
    // An Operand can be a virtual register, a constant value, or a label.
    using Operand = std::variant<std::string, int64_t, double, bool>;

    std::string to_string(const Operand &operand);

    // The set of all possible operations in ThermoIR.
    enum class OpCode
    {
        // Value and Memory
        LOAD_CONST,
        ASSIGN,

        // Binary Operations
        ADD,
        SUB,
        MUL,
        DIV,

        // Control Flow
        JUMP,
        BRANCH, // Conditional branch
        RETURN,

        // Function Calls
        CALL,

        // Domain-Specific Operations
        SAMPLE_GAUSSIAN,
        CREATE_ENERGY_FUNC,
        THERMAL_ANNEAL,
    };

    // Base class for all IR instructions.
    struct Instruction
    {
        OpCode opcode;

        Instruction(OpCode op) : opcode(op) {}
        virtual ~Instruction() = default;
        virtual std::string toString() const = 0;
    };

    // Specific instruction types
    // Format: result = op(arg1, arg2)

    struct BinaryOpInstr : Instruction
    {
        std::string result_reg;
        Operand arg1;
        Operand arg2;

        BinaryOpInstr(OpCode op, std::string result, Operand a1, Operand a2)
            : Instruction(op), result_reg(result), arg1(a1), arg2(a2) {}

        std::string toString() const override;
    };

    struct LoadConstInstr : Instruction
    {
        std::string result_reg;
        Operand value; // Will hold int, float, or bool

        LoadConstInstr(std::string result, Operand val)
            : Instruction(OpCode::LOAD_CONST), result_reg(result), value(val) {}

        std::string toString() const override;
    };

    struct AssignInstr : Instruction
    {
        std::string dest_reg;
        Operand source;

        AssignInstr(std::string dest, Operand src)
            : Instruction(OpCode::ASSIGN), dest_reg(dest), source(src) {}

        std::string toString() const override;
    };

    struct ReturnInstr : Instruction
    {
        std::optional<Operand> return_value;

        ReturnInstr(std::optional<Operand> val)
            : Instruction(OpCode::RETURN), return_value(val) {}

        std::string toString() const override;
    };

    struct CallInstr : Instruction
    {
        std::optional<std::string> result_reg;
        std::string callee_name;
        std::vector<Operand> args;

        CallInstr(std::optional<std::string> result, std::string name, std::vector<Operand> arguments)
            : Instruction(OpCode::CALL), result_reg(result), callee_name(name), args(std::move(arguments)) {}

        std::string toString() const override;
    };

    // A "Basic Block" is a straight-line sequence of instructions ending with a terminator.
    struct BasicBlock
    {
        std::string label;
        std::vector<std::unique_ptr<Instruction>> instructions;

        BasicBlock(std::string lbl) : label(std::move(lbl)) {}
    };

    // A complete IR representation of a single function.
    struct FunctionIR
    {
        std::string name;
        std::vector<std::string> parameters; // Parameter registers
        std::vector<std::unique_ptr<BasicBlock>> basic_blocks;

        std::string toString() const;
    };

} // namespace thermolang::ir

#endif // THERMOLANG_IR_H