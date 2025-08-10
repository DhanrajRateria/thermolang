#include "thermolang/optimizer/IsingModelPass.h"
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
                else if (auto *call_op = dynamic_cast<ir::CallInstr *>(instr.get()))
                {
                    if (call_op->result_reg.has_value() && *call_op->result_reg == reg)
                        return instr.get();
                }
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

    bool IsingModelPass::run(ir::FunctionIR &function_ir)
    {
        if (!function_ir.is_energy_function)
        {
            return false;
        }

        // Idempotency Check: Skip if the function has already been optimized by this pass.
        for (const auto &block : function_ir.basic_blocks)
        {
            for (const auto &instr : block->instructions)
            {
                if (dynamic_cast<ir::IsingHamiltonianInstr *>(instr.get()))
                {
                    return false;
                }
            }
        }

        std::cout << "Running Ising Model Pass on '" << function_ir.name << "'" << std::endl;

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
        if (!negation || negation->opcode != ir::OpCode::MUL || !get_const_value(function_ir, negation->arg2).has_value() || get_const_value(function_ir, negation->arg2) != -1.0)
        {
            final_block->instructions.push_back(std::move(return_instr_ptr)); // Restore IR and exit
            return false;
        }

        std::string sum_reg = std::get<std::string>(negation->arg1);
        auto *hamiltonian_add = dynamic_cast<ir::BinaryOpInstr *>(find_defining_instr(function_ir, sum_reg));
        if (!hamiltonian_add || hamiltonian_add->opcode != ir::OpCode::ADD)
        {
            final_block->instructions.push_back(std::move(return_instr_ptr)); // Restore IR and exit
            return false;
        }

        // --- Parameter Extraction ---
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

        // Recursive lambda to traverse the Add tree and extract terms.
        std::function<void(const ir::Operand &)> extract_terms =
            [&](const ir::Operand &op)
        {
            if (!std::holds_alternative<std::string>(op))
                return;
            auto *term_def = find_defining_instr(function_ir, std::get<std::string>(op));

            if (auto *add = dynamic_cast<ir::BinaryOpInstr *>(term_def))
            {
                if (add->opcode == ir::OpCode::ADD)
                {
                    extract_terms(add->arg1);
                    extract_terms(add->arg2);
                    return;
                }
            }

            auto *mul = dynamic_cast<ir::BinaryOpInstr *>(term_def);
            if (!mul || mul->opcode != ir::OpCode::MUL)
                return;

            // Case 1: Field Term (h_i * s_i)
            if (auto h_val = get_const_value(function_ir, mul->arg1))
            {
                if (auto s_i_op = std::get_if<std::string>(&mul->arg2))
                {
                    if (spin_to_index.count(*s_i_op))
                        h[spin_to_index[*s_i_op]] += *h_val;
                }
            }
            // Case 2: Coupling Term ( (J_ij * s_i) * s_j )
            else if (auto *inner_mul = dynamic_cast<ir::BinaryOpInstr *>(find_defining_instr(function_ir, std::get<std::string>(mul->arg1))))
            {
                if (auto j_val = get_const_value(function_ir, inner_mul->arg1))
                {
                    if (auto s_i_op = std::get_if<std::string>(&inner_mul->arg2))
                    {
                        if (auto s_j_op = std::get_if<std::string>(&mul->arg2))
                        {
                            if (spin_to_index.count(*s_i_op) && spin_to_index.count(*s_j_op))
                            {
                                int idx_i = spin_to_index[*s_i_op];
                                int idx_j = spin_to_index[*s_j_op];
                                J[idx_i][idx_j] += *j_val;
                                J[idx_j][idx_i] += *j_val; // Ensure symmetry
                            }
                        }
                    }
                }
            }
        };

        extract_terms(hamiltonian_add->arg1);
        extract_terms(hamiltonian_add->arg2);

        std::cout << "  Detected Ising model pattern. Rewriting with optimized ISING_HAMILTONIAN instruction." << std::endl;

        // --- Rewrite the IR ---
        // Create the new, single instruction representing the entire Hamiltonian.
        auto ising_instr = std::make_unique<ir::IsingHamiltonianInstr>(
            final_energy_reg, // The result is stored in the same register as the original return value.
            spin_operands,
            std::move(J),
            std::move(h));

        // Clear the old, low-level instructions from the block.
        final_block->instructions.clear();

        // Add the new high-level instruction, followed by the saved return instruction.
        final_block->instructions.push_back(std::move(ising_instr));
        final_block->instructions.push_back(std::move(return_instr_ptr));

        return true; // The IR was successfully modified.
    }
} // namespace thermolang::optimizer