#include "thermolang/optimizer/IsingModelPass.h"
#include <iostream>
#include <map>
#include <vector>
#include <functional>

namespace thermolang::optimizer
{

    bool IsingModelPass::run(ir::FunctionIR &function_ir)
    {
        if (!function_ir.is_energy_function)
        {
            return false;
        }

        std::cout << "Running Ising Model Pass on '" << function_ir.name << "'" << std::endl;

        ir::ReturnInstr *return_instr = nullptr;
        ir::BasicBlock *final_block = nullptr;

        for (auto &block : function_ir.basic_blocks)
        {
            if (!block->instructions.empty())
            {
                if (auto *ret = dynamic_cast<ir::ReturnInstr *>(block->instructions.back().get()))
                {
                    return_instr = ret;
                    final_block = block.get();
                    break;
                }
            }
        }

        if (!return_instr || !return_instr->return_value.has_value() || !std::holds_alternative<std::string>(*return_instr->return_value))
        {
            return false; // Not returning a register
        }

        std::string energy_result_reg = std::get<std::string>(*return_instr->return_value);

        // Data structures to hold the Ising model parameters
        std::map<std::string, int> spin_to_index;
        std::vector<ir::Operand> spin_operands;

        // Find all function parameters, treat them as spins
        for (const auto &param_reg : function_ir.parameters)
        {
            if (spin_to_index.find(param_reg) == spin_to_index.end())
            {
                spin_to_index[param_reg] = spin_operands.size();
                spin_operands.push_back(param_reg);
            }
        }

        int num_spins = spin_operands.size();
        if (num_spins == 0)
            return false;

        std::vector<std::vector<double>> J(num_spins, std::vector<double>(num_spins, 0.0));
        std::vector<double> h(num_spins, 0.0);

        // Pattern match: H = - (sum(J_ij * s_i * s_j) + sum(h_i * s_i))
        // We look for the final ADD instruction that computes the total energy.
        auto final_instr = find_def(function_ir, energy_result_reg);
        auto *negation_instr = dynamic_cast<ir::BinaryOpInstr *>(final_instr);

        // Check for negation `mul <reg>, -1`
        if (!negation_instr || negation_instr->opcode != ir::OpCode::MUL)
            return false;

        std::string total_sum_reg = std::get<std::string>(negation_instr->arg1);
        auto *sum_instr = dynamic_cast<ir::BinaryOpInstr *>(find_def(function_ir, total_sum_reg));
        if (!sum_instr || sum_instr->opcode != ir::OpCode::ADD)
            return false;

        // Traverse the chain of additions to extract all terms
        std::vector<std::string> energy_terms;
        std::function<void(ir::Instruction *)> collect_terms =
            [&](ir::Instruction *instr)
        {
            auto *add_instr = dynamic_cast<ir::BinaryOpInstr *>(instr);
            if (add_instr && add_instr->opcode == ir::OpCode::ADD)
            {
                collect_terms(find_def(function_ir, std::get<std::string>(add_instr->arg1)));
                collect_terms(find_def(function_ir, std::get<std::string>(add_instr->arg2)));
            }
            else if (auto *mul_instr = dynamic_cast<ir::BinaryOpInstr *>(instr))
            {
                energy_terms.push_back(mul_instr->result_reg);
            }
        };

        collect_terms(sum_instr);

        // Process each term to populate J and h
        for (const auto &term_reg : energy_terms)
        {
            auto *mul_instr = dynamic_cast<ir::BinaryOpInstr *>(find_def(function_ir, term_reg));
            if (!mul_instr || mul_instr->opcode != ir::OpCode::MUL)
                continue;

            auto *op1_instr = find_def(function_ir, std::get<std::string>(mul_instr->arg1));
            auto *op2_instr = find_def(function_ir, std::get<std::string>(mul_instr->arg2));

            // Case 1: Coupling term (J_ij * s_i) * s_j
            if (auto *inner_mul = dynamic_cast<ir::BinaryOpInstr *>(op1_instr))
            {
                if (inner_mul->opcode == ir::OpCode::MUL)
                {
                    auto s_i_reg = std::get<std::string>(inner_mul->arg2);
                    auto s_j_reg = std::get<std::string>(mul_instr->arg2);

                    auto *coeff_instr = find_def(function_ir, std::get<std::string>(inner_mul->arg1));
                    if (auto *load_const = dynamic_cast<ir::LoadConstInstr *>(coeff_instr))
                    {
                        double J_ij = std::get<double>(load_const->value);
                        int idx_i = spin_to_index[s_i_reg];
                        int idx_j = spin_to_index[s_j_reg];
                        J[idx_i][idx_j] = J[idx_j][idx_i] = J_ij;
                    }
                }
            }
            // Case 2: Local field term (h_i * s_i)
            else if (auto *load_const = dynamic_cast<ir::LoadConstInstr *>(op1_instr))
            {
                auto s_i_reg = std::get<std::string>(mul_instr->arg2);
                double h_i = std::get<double>(load_const->value);
                h[spin_to_index[s_i_reg]] = h_i;
            }
        }

        std::cout << "  Detected Ising model pattern. Replacing with optimized instruction." << std::endl;

        // Rewrite the IR
        std::vector<std::unique_ptr<ir::Instruction>> new_instructions;
        new_instructions.push_back(std::make_unique<ir::IsingHamiltonianInstr>(
            energy_result_reg, spin_operands, std::move(J), std::move(h)));
        new_instructions.push_back(std::move(final_block->instructions.back())); // Keep the return instruction

        final_block->instructions = std::move(new_instructions);

        return true;
    }

    ir::Instruction *IsingModelPass::find_def(ir::FunctionIR &function_ir, const std::string &reg)
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

} // namespace thermolang::optimizer