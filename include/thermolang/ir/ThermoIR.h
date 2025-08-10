#ifndef THERMOLANG_IR_H
#define THERMOLANG_IR_H

#include "thermolang/types/Type.h"
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <unordered_map>

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

        // Comparison Operations
        EQUAL, // Equal
        NOT_EQUAL,
        LESS,
        LESS_EQUAL,
        GREATER,
        GREATER_EQUAL,

        // Control Flow
        JUMP,
        BRANCH, // Conditional branch
        RETURN,

        // Function Calls
        CALL,

        // Domain-Specific Operations
        SAMPLE_GAUSSIAN,
        SAMPLE_UNIFORM,
        SAMPLE_BERNOULLI,
        CREATE_ENERGY_FUNC,
        MINIMIZE_ENERGY,
        THERMAL_ANNEAL,
        SET_TEMPERATURE,
        COUPLE_CIRCUITS,
        PARALLEL_FOR,
        THERMAL_STEP,
        VARIANCE_TRACK,
        QUADRATIC_FORM,
        ISING_HAMILTONIAN,
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

    struct JumpInstr : Instruction
    {
        std::string target_label;

        JumpInstr(std::string label)
            : Instruction(OpCode::JUMP), target_label(std::move(label)) {}

        std::string toString() const override;
    };

    struct BranchInstr : Instruction
    {
        Operand condition_reg;
        std::string true_label;
        std::string false_label;

        BranchInstr(Operand cond, std::string true_lbl, std::string false_lbl)
            : Instruction(OpCode::BRANCH), condition_reg(cond),
              true_label(std::move(true_lbl)), false_label(std::move(false_lbl)) {}

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

    // Domain-specific instructions

    struct SampleGaussianInstr : Instruction
    {
        std::string result_reg;
        Operand mean;
        Operand variance;

        SampleGaussianInstr(std::string result, Operand m, Operand v)
            : Instruction(OpCode::SAMPLE_GAUSSIAN), result_reg(result), mean(m), variance(v) {}

        std::string toString() const override;
    };

    struct SampleUniformInstr : Instruction
    {
        std::string result_reg;
        Operand low;
        Operand high;

        SampleUniformInstr(std::string result, Operand l, Operand h)
            : Instruction(OpCode::SAMPLE_UNIFORM), result_reg(result), low(l), high(h) {}

        std::string toString() const override;
    };

    struct SampleBernoulliInstr : Instruction
    {
        std::string result_reg;
        Operand probability;

        SampleBernoulliInstr(std::string result, Operand p)
            : Instruction(OpCode::SAMPLE_BERNOULLI), result_reg(result), probability(p) {}

        std::string toString() const override;
    };

    struct CreateEnergyFuncInstr : Instruction
    {
        std::string result_reg;
        std::vector<Operand> var_regs;
        std::string energy_expr_id; // Reference to an energy expression

        CreateEnergyFuncInstr(std::string result, std::vector<Operand> vars, std::string expr_id)
            : Instruction(OpCode::CREATE_ENERGY_FUNC), result_reg(result),
              var_regs(std::move(vars)), energy_expr_id(expr_id) {}

        std::string toString() const override;
    };

    struct MinimizeEnergyInstr : Instruction
    {
        std::string result_reg;
        Operand energy_func;
        std::vector<Operand> initial_values;

        MinimizeEnergyInstr(std::string result, Operand func, std::vector<Operand> init_vals)
            : Instruction(OpCode::MINIMIZE_ENERGY), result_reg(result),
              energy_func(func), initial_values(std::move(init_vals)) {}

        std::string toString() const override;
    };

    struct ThermalAnnealInstr : Instruction
    {
        std::string result_reg;
        Operand energy_func_reg; // Register holding the energy function
        Operand schedule_reg;    // Register holding the cooling schedule object

        ThermalAnnealInstr(std::string result, Operand func, Operand schedule)
            : Instruction(OpCode::THERMAL_ANNEAL), result_reg(std::move(result)),
              energy_func_reg(std::move(func)), schedule_reg(std::move(schedule)) {}

        std::string toString() const override;
    };

    struct SetTemperatureInstr : Instruction
    {
        Operand temperature;

        SetTemperatureInstr(Operand temp)
            : Instruction(OpCode::SET_TEMPERATURE), temperature(temp) {}

        std::string toString() const override;
    };

    struct CoupleCircuitsInstr : Instruction
    {
        std::string result_reg;
        std::vector<Operand> circuit_regs;
        Operand coupling_strength;

        CoupleCircuitsInstr(std::string result, std::vector<Operand> circuits, Operand strength)
            : Instruction(OpCode::COUPLE_CIRCUITS), result_reg(result),
              circuit_regs(std::move(circuits)), coupling_strength(strength) {}

        std::string toString() const override;
    };

    struct ParallelForInstr : Instruction
    {
        std::string iterator_reg;
        Operand collection;
        std::string body_block_label;
        std::string exit_block_label;

        ParallelForInstr(std::string iter, Operand coll, std::string body, std::string exit)
            : Instruction(OpCode::PARALLEL_FOR), iterator_reg(iter),
              collection(coll), body_block_label(body), exit_block_label(exit) {}

        std::string toString() const override;
    };

    struct ThermalStepInstr : Instruction
    {
        std::string result_reg;
        Operand current_state;
        Operand temperature;

        ThermalStepInstr(std::string result, Operand state, Operand temp)
            : Instruction(OpCode::THERMAL_STEP), result_reg(result),
              current_state(state), temperature(temp) {}

        std::string toString() const override;
    };

    struct VarianceTrackInstr : Instruction
    {
        std::string result_reg;
        Operand value;
        Operand variance;

        VarianceTrackInstr(std::string result, Operand val, Operand var)
            : Instruction(OpCode::VARIANCE_TRACK), result_reg(result),
              value(val), variance(var) {}

        std::string toString() const override;
    };

    struct QuadraticFormInstr : Instruction
    {
        std::string result_reg;
        std::vector<Operand> variables;
        // In a real scenario, this might be a matrix. For this IR,
        // we'll point to a data structure holding the coefficients.
        std::string matrix_id;

        QuadraticFormInstr(std::string result, std::vector<Operand> vars, std::string id)
            : Instruction(OpCode::QUADRATIC_FORM), result_reg(result),
              variables(std::move(vars)), matrix_id(std::move(id)) {}

        std::string toString() const override;
    };

    struct IsingHamiltonianInstr : Instruction
    {
        std::string result_reg;
        std::vector<Operand> spins;                // Registers holding the spin variables
        std::vector<std::vector<double>> J_matrix; // Coupling matrix
        std::vector<double> h_vector;              // External field vector

        IsingHamiltonianInstr(std::string result, std::vector<Operand> s,
                              std::vector<std::vector<double>> J, std::vector<double> h)
            : Instruction(OpCode::CREATE_ENERGY_FUNC), // Re-using an appropriate OpCode
              result_reg(std::move(result)), spins(std::move(s)),
              J_matrix(std::move(J)), h_vector(std::move(h))
        {
        }

        std::string toString() const override;
    };

    // A "Basic Block" is a straight-line sequence of instructions ending with a terminator.
    struct BasicBlock
    {
        std::string label;
        std::vector<std::unique_ptr<Instruction>> instructions;

        BasicBlock(std::string lbl) : label(std::move(lbl)) {}
    };

    // Energy expression representation
    struct EnergyExpression
    {
        std::string id;
        std::vector<std::string> var_names;
        std::unique_ptr<BasicBlock> expression_block;
        std::string result_reg;

        EnergyExpression(std::string id, std::vector<std::string> vars,
                         std::unique_ptr<BasicBlock> block, std::string result)
            : id(std::move(id)), var_names(std::move(vars)),
              expression_block(std::move(block)), result_reg(std::move(result)) {}
    };

    // A complete IR representation of a single function.
    struct FunctionIR
    {
        std::string name;
        std::vector<std::string> parameters; // Parameter registers
        std::vector<std::unique_ptr<BasicBlock>> basic_blocks;
        std::unordered_map<std::string, std::unique_ptr<EnergyExpression>> energy_expressions;
        bool is_stochastic = false;
        bool is_energy_function = false;

        std::string toString() const;
    };

} // namespace thermolang::ir

#endif // THERMOLANG_IR_H