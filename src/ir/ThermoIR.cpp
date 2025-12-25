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
        case OpCode::EQUAL:
            return "eq";
        case OpCode::NOT_EQUAL:
            return "neq";
        case OpCode::LESS:
            return "lt";
        case OpCode::LESS_EQUAL:
            return "lte";
        case OpCode::GREATER:
            return "gt";
        case OpCode::GREATER_EQUAL:
            return "gte";
        case OpCode::QUADRATIC_FORM:
            return "quadratic_form";
        case OpCode::SAMPLE_GAUSSIAN:
            return "sample_gaussian";
        case OpCode::SAMPLE_UNIFORM:
            return "sample_uniform";
        case OpCode::SAMPLE_BERNOULLI:
            return "sample_bernoulli";
        case OpCode::CREATE_ENERGY_FUNC:
            return "create_energy_func";
        case OpCode::MINIMIZE_ENERGY:
            return "minimize_energy";
        case OpCode::THERMAL_ANNEAL:
            return "thermal_anneal";
        case OpCode::SET_TEMPERATURE:
            return "set_temperature";
        case OpCode::COUPLE_CIRCUITS:
            return "couple_circuits";
        case OpCode::PARALLEL_FOR:
            return "parallel_for";
        case OpCode::THERMAL_STEP:
            return "thermal_step";
        case OpCode::VARIANCE_TRACK:
            return "variance_track";
        case OpCode::DISCRETE_EBM:
            return "discrete_ebm";
        case OpCode::DENOISE:
            return "denoise";
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

    std::string JumpInstr::toString() const
    {
        return "    jump " + target_label;
    }

    std::string BranchInstr::toString() const
    {
        return "    branch " + to_string(condition_reg) + ", " + true_label + ", " + false_label;
    }

    std::string QuadraticFormInstr::toString() const
    {
        std::stringstream ss;
        ss << "    " << result_reg << " = quadratic_form matrix=" << matrix_id << ", vars=[";
        for (size_t i = 0; i < variables.size(); ++i)
        {
            ss << to_string(variables[i]) << (i == variables.size() - 1 ? "" : ", ");
        }
        ss << "]";
        return ss.str();
    }

    std::string DenoiseInstr::toString() const
    {
        return "    " + result_reg + " = denoise target=" + target_energy_func +
               ", sigma=" + to_string(initial_sigma) + ", steps=" + to_string(steps);
    }

    std::string DiscreteEBMInstr::toString() const
    {
        std::stringstream ss;
        ss << "    " << result_reg << " = discrete_ebm spins=[";
        for (size_t i = 0; i < spins.size(); ++i)
        {
            ss << to_string(spins[i]) << (i == spins.size() - 1 ? "" : ", ");
        }
        ss << "]";

        // concise matrix output
        ss << ", J=<" << J_matrix.size() << "x" << (J_matrix.empty() ? 0 : J_matrix[0].size()) << ">";
        ss << ", h=<size " << h_vector.size() << ">";

        // Display optimization status
        if (!color_groups.empty())
        {
            ss << ", colored_blocks=" << color_groups.size();
        }
        else
        {
            ss << ", uncolored";
        }

        return ss.str();
    }

    // Domain-specific instruction toString implementations
    std::string SampleGaussianInstr::toString() const
    {
        return "    " + result_reg + " = " + to_string(opcode) + " " +
               to_string(mean) + ", " + to_string(variance);
    }

    std::string SampleUniformInstr::toString() const
    {
        return "    " + result_reg + " = " + to_string(opcode) + " " +
               to_string(low) + ", " + to_string(high);
    }

    std::string SampleBernoulliInstr::toString() const
    {
        return "    " + result_reg + " = " + to_string(opcode) + " " + to_string(probability);
    }

    std::string CreateEnergyFuncInstr::toString() const
    {
        std::stringstream ss;
        ss << "    " << result_reg << " = " << to_string(opcode) << " ";

        ss << "[";
        for (size_t i = 0; i < var_regs.size(); ++i)
        {
            ss << to_string(var_regs[i]);
            if (i < var_regs.size() - 1)
            {
                ss << ", ";
            }
        }
        ss << "] -> " << energy_expr_id;

        return ss.str();
    }

    std::string MinimizeEnergyInstr::toString() const
    {
        std::stringstream ss;
        ss << "    " << result_reg << " = " << to_string(opcode) << " "
           << to_string(energy_func) << " [";

        for (size_t i = 0; i < initial_values.size(); ++i)
        {
            ss << to_string(initial_values[i]);
            if (i < initial_values.size() - 1)
            {
                ss << ", ";
            }
        }
        ss << "]";

        return ss.str();
    }

    std::string ThermalAnnealInstr::toString() const
    {
        return "    " + result_reg + " = thermal_anneal func=" + to_string(energy_func_reg) + ", schedule=" + to_string(schedule_reg);
    }

    std::string SetTemperatureInstr::toString() const
    {
        return "    " + to_string(opcode) + " " + to_string(temperature);
    }

    std::string CoupleCircuitsInstr::toString() const
    {
        std::stringstream ss;
        ss << "    " << result_reg << " = " << to_string(opcode) << " [";

        for (size_t i = 0; i < circuit_regs.size(); ++i)
        {
            ss << to_string(circuit_regs[i]);
            if (i < circuit_regs.size() - 1)
            {
                ss << ", ";
            }
        }
        ss << "], " << to_string(coupling_strength);

        return ss.str();
    }

    std::string ParallelForInstr::toString() const
    {
        return "    " + to_string(opcode) + " " + iterator_reg + " in " +
               to_string(collection) + " -> " + body_block_label + ", exit: " + exit_block_label;
    }

    std::string ThermalStepInstr::toString() const
    {
        return "    " + result_reg + " = " + to_string(opcode) + " " +
               to_string(current_state) + ", " + to_string(temperature);
    }

    std::string VarianceTrackInstr::toString() const
    {
        return "    " + result_reg + " = " + to_string(opcode) + " " +
               to_string(value) + ", variance=" + to_string(variance);
    }

    std::string FunctionIR::toString() const
    {
        std::stringstream ss;

        // Add function modifiers
        if (is_stochastic)
            ss << "stochastic ";
        if (is_energy_function)
            ss << "energy ";

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

        // Energy expressions
        if (!energy_expressions.empty())
        {
            ss << "  // Energy expressions\n";
            for (const auto &[id, expr] : energy_expressions)
            {
                ss << "  energy_expr " << id << "(" << expr->var_names.size() << " vars) -> "
                   << expr->result_reg << " {\n";

                for (const auto &instr : expr->expression_block->instructions)
                {
                    ss << "  " << instr->toString() << "\n";
                }

                ss << "  }\n\n";
            }
        }

        // Basic blocks
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