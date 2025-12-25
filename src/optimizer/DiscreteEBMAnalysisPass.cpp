#include "thermolang/optimizer/DiscreteEBMAnalysisPass.h"
#include <iostream>
#include <map>
#include <vector>
#include <functional>
#include <optional>

namespace thermolang::optimizer
{
    // Helper to find the instruction that defines a register.
    ir::Instruction *find_defining_instr(ir::FunctionIR &function_ir, const std::string &reg)
    {
        for (auto &block : function_ir.basic_blocks)
        {
            for (auto &instr : block->instructions)
            {
                if (auto *bin_op = dynamic_cast<ir::BinaryOpInstr *>(instr.get()))
                {
                    if (bin_op->result_reg == reg)
                        return instr.get();
                }
                else if (auto *load_op = dynamic_cast<ir::LoadConstInstr *>(instr.get()))
                {
                    if (load_op->result_reg == reg)
                        return instr.get();
                }
                // else if (auto *call_op = dynamic_cast<ir::CallInstr *>(instr.get()))
                // {
                //     if (call_op->result_reg.has_value() && *call_op->result_reg == reg)
                //         return instr.get();
                // }
            }
        }
        return nullptr;
    }

    // Helper to extract a constant value from an operand, resolving registers.
    std::optional<double> get_const_value(ir::FunctionIR &ir, const ir::Operand &op)
    {
        if (std::holds_alternative<double>(op))
            return std::get<double>(op);
        if (std::holds_alternative<int64_t>(op))
            return static_cast<double>(std::get<int64_t>(op));

        if (std::holds_alternative<std::string>(op))
        {
            auto *def_instr = find_defining_instr(ir, std::get<std::string>(op));
            if (auto *load_instr = dynamic_cast<ir::LoadConstInstr *>(def_instr))
            {
                if (std::holds_alternative<double>(load_instr->value))
                    return std::get<double>(load_instr->value);
                if (std::holds_alternative<int64_t>(load_instr->value))
                    return static_cast<double>(std::get<int64_t>(load_instr->value));
            }
        }
        return std::nullopt;
    }

    bool DiscreteEBMAnalysisPass::run(ir::FunctionIR &function_ir)
    {
        bool is_inverted = false;
        if (!function_ir.is_energy_function)
        {
            return false;
        }

        // Idempotency Check
        for (const auto &block : function_ir.basic_blocks)
        {
            for (const auto &instr : block->instructions)
            {
                if (dynamic_cast<ir::DiscreteEBMInstr *>(instr.get()))
                {
                    return false;
                }
            }
        }

        std::cout << "Running Discrete EBM Analysis Pass on '" << function_ir.name << "'" << std::endl;

        ir::BasicBlock *final_block = nullptr;
        std::string final_energy_reg;
        std::unique_ptr<ir::Instruction> return_instr_ptr = nullptr;
        bool found = false;

        // Safely find and extract the return instruction's unique_ptr before modification.
        for (auto &block : function_ir.basic_blocks)
        {
            auto &instructions = block->instructions;
            for (auto it = instructions.begin(); it != instructions.end(); ++it)
            {
                if (auto *ret = dynamic_cast<ir::ReturnInstr *>(it->get()))
                {
                    if (ret->return_value.has_value() && std::holds_alternative<std::string>(*ret->return_value))
                    {
                        final_energy_reg = std::get<std::string>(*ret->return_value);
                        final_block = block.get();

                        // Safely take ownership of the return instruction.
                        return_instr_ptr = std::move(*it);
                        instructions.erase(it); // Remove the now-empty unique_ptr from the vector.
                        found = true;
                        break;
                    }
                }
            }
            if (found)
                break;
        }

        if (!return_instr_ptr)
        {
            return false; // No suitable return instruction was found.
        }

        // --- Pattern Matching Logic ---
        // Trace back from the return value to find the Hamiltonian structure.
        auto *negation = dynamic_cast<ir::BinaryOpInstr *>(find_defining_instr(function_ir, final_energy_reg));
        std::string sum_reg;

        if (negation && negation->opcode == ir::OpCode::MUL)
        {
            auto val1 = get_const_value(function_ir, negation->arg1);
            auto val2 = get_const_value(function_ir, negation->arg2);

            // Check commutativity: -1 * sum OR sum * -1
            if (val1.has_value() && std::abs(*val1 + 1.0) < 1e-9 && std::holds_alternative<std::string>(negation->arg2))
            {
                sum_reg = std::get<std::string>(negation->arg2);
            }
            else if (val2.has_value() && std::abs(*val2 + 1.0) < 1e-9 && std::holds_alternative<std::string>(negation->arg1))
            {
                sum_reg = std::get<std::string>(negation->arg1);
            }
        }

        if (sum_reg.empty())
        {
            sum_reg = final_energy_reg;
            is_inverted = false;
        }
        else
        {
            is_inverted = true;
        }

        // 3. Map Parameters
        std::map<std::string, int> spin_to_index;
        std::vector<ir::Operand> spin_operands;
        for (const auto &param_reg : function_ir.parameters)
        {
            spin_to_index[param_reg] = spin_operands.size();
            spin_operands.push_back(param_reg);
        }
        int n_spins = spin_operands.size();
        std::vector<std::vector<double>> J(n_spins, std::vector<double>(n_spins, 0.0));
        std::vector<double> h(n_spins, 0.0);

        // 4. Recursive Term Extraction
        std::function<void(const ir::Operand &, double)> extract_terms_weighted;

        extract_terms_weighted = [&](const ir::Operand &op, double multiplier)
        {
            if (!std::holds_alternative<std::string>(op))
                return;
            std::string reg = std::get<std::string>(op);

            // Base Case 1: The variable itself (Linear term h_i)
            if (spin_to_index.count(reg))
            {
                h[spin_to_index[reg]] += 1.0 * multiplier;
                return;
            }

            auto *def = find_defining_instr(function_ir, reg);
            if (!def)
                return;

            // Recursive Step: Distribute multiplier over Addition
            if (auto *add = dynamic_cast<ir::BinaryOpInstr *>(def))
            {
                if (add->opcode == ir::OpCode::ADD)
                {
                    extract_terms_weighted(add->arg1, multiplier);
                    extract_terms_weighted(add->arg2, multiplier);
                    return;
                }
            }

            // Analysis Step: Handle Multiplication
            if (auto *mul = dynamic_cast<ir::BinaryOpInstr *>(def))
            {
                if (mul->opcode == ir::OpCode::MUL)
                {
                    auto c1 = get_const_value(function_ir, mul->arg1);
                    auto c2 = get_const_value(function_ir, mul->arg2);

                    std::string r1 = std::holds_alternative<std::string>(mul->arg1) ? std::get<std::string>(mul->arg1) : "";
                    std::string r2 = std::holds_alternative<std::string>(mul->arg2) ? std::get<std::string>(mul->arg2) : "";

                    // Case A: Constant * Expression (Recurse down)
                    if (c1.has_value() && !r2.empty())
                    {
                        extract_terms_weighted(mul->arg2, multiplier * (*c1));
                        return;
                    }
                    if (c2.has_value() && !r1.empty())
                    {
                        extract_terms_weighted(mul->arg1, multiplier * (*c2));
                        return;
                    }

                    // Case B: Spin * Spin (Quadratic Coupling J_ij)
                    if (!r1.empty() && !r2.empty() && spin_to_index.count(r1) && spin_to_index.count(r2))
                    {
                        int i = spin_to_index[r1];
                        int j = spin_to_index[r2];
                        // Add contribution to J matrix (symmetric)
                        J[i][j] += 1.0 * multiplier;
                        J[j][i] += 1.0 * multiplier;
                        return;
                    }
                }
            }
        };

        // Start traversal
        extract_terms_weighted(ir::Operand{sum_reg}, 1.0);

        std::cout << "  Detected Discrete EBM pattern. J Matrix size: " << n_spins << "x" << n_spins << std::endl;

        // 5. Polarity Correction
        // If E_user = sum (s * s), we need Hardware H = - sum (-1 * s * s)
        if (!is_inverted)
        {
            std::cout << "  (Implicit positive energy detected. Inverting weights for hardware Hamiltonian.)" << std::endl;
            for (auto &row : J)
                for (double &val : row)
                    val = -val;
            for (double &val : h)
                val = -val;
        }

        // 6. Rewrite IR
        auto ebm_instr = std::make_unique<ir::DiscreteEBMInstr>(
            final_energy_reg,
            spin_operands,
            std::move(J),
            std::move(h));

        // Clear the old, low-level instructions from the block.
        final_block->instructions.clear();

        // Add the new high-level instruction, followed by the saved return instruction.
        final_block->instructions.push_back(std::move(ebm_instr));
        final_block->instructions.push_back(std::move(return_instr_ptr));

        return true; // The IR was successfully modified.
    }
} // namespace thermolang::optimizer