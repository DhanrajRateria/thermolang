#include "thermolang/optimizer/VarianceTrackingPass.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace thermolang::optimizer
{

    bool VarianceTrackingPass::run(ir::FunctionIR &function_ir)
    {
        if (!function_ir.is_stochastic)
        {
            return false;
        }

        std::cout << "Running variance tracking on stochastic function '"
                  << function_ir.name << "'" << std::endl;

        auto variances = propagate_variance(function_ir);

        if (variances.empty())
        {
            return false;
        }

        return insert_variance_tracking(function_ir, variances);
    }

    std::unordered_map<std::string, double> VarianceTrackingPass::propagate_variance(
        ir::FunctionIR &function_ir)
    {
        std::unordered_map<std::string, double> variances;
        std::unordered_map<std::string, double> means;

        for (auto &block : function_ir.basic_blocks)
        {
            for (auto &instr : block->instructions)
            {
                // (The propagation logic is complex and correct, no changes needed here)
                // ... existing propagation logic ...
            }
        }
        return variances;
    }

    bool VarianceTrackingPass::insert_variance_tracking(
        ir::FunctionIR &function_ir,
        const std::unordered_map<std::string, double> &variances)
    {
        bool modified = false;

        for (auto &block : function_ir.basic_blocks)
        {
            std::vector<std::unique_ptr<ir::Instruction>> new_instructions;
            for (auto &instr : block->instructions)
            {
                std::string result_reg = "";
                if (auto i = dynamic_cast<ir::SampleGaussianInstr *>(instr.get()))
                    result_reg = i->result_reg;
                else if (auto i = dynamic_cast<ir::SampleUniformInstr *>(instr.get()))
                    result_reg = i->result_reg;
                else if (auto i = dynamic_cast<ir::SampleBernoulliInstr *>(instr.get()))
                    result_reg = i->result_reg;

                // Always add the original instruction
                new_instructions.push_back(std::move(instr));

                if (!result_reg.empty() && variances.count(result_reg))
                {
                    // *** FIX: Check if a tracking instruction for this register ALREADY exists ***
                    bool already_tracked = false;
                    for (const auto &existing_instr : block->instructions)
                    {
                        if (auto track_instr = dynamic_cast<ir::VarianceTrackInstr *>(existing_instr.get()))
                        {
                            if (track_instr->result_reg == result_reg)
                            {
                                already_tracked = true;
                                break;
                            }
                        }
                    }

                    if (!already_tracked)
                    {
                        double variance = variances.at(result_reg);
                        new_instructions.push_back(std::make_unique<ir::VarianceTrackInstr>(
                            result_reg, result_reg, variance));
                        modified = true;
                    }
                }
            }
            // Replace with the potentially modified list of instructions
            block->instructions = std::move(new_instructions);
        }

        if (modified)
        {
            std::cout << "  Inserted variance tracking instructions." << std::endl;
        }

        return modified;
    }

    // (Rest of the file remains unchanged)
    // ...
    double VarianceTrackingPass::compute_addition_variance(double var1, double var2)
    {
        // For addition/subtraction, variances add (assuming independence)
        return var1 + var2;
    }

    double VarianceTrackingPass::compute_multiplication_variance(
        double mean1, double mean2, double var1, double var2)
    {

        // For multiplication z = x*y, the variance is:
        // Var(z) = (Mean(x)^2 * Var(y)) + (Mean(y)^2 * Var(x)) + (Var(x) * Var(y))

        return std::pow(mean1, 2) * var2 + std::pow(mean2, 2) * var1 + var1 * var2;
    }

    double VarianceTrackingPass::compute_sampling_variance(const ir::Instruction *instr)
    {
        // Compute variance based on the type of sampling instruction
        if (const auto *gaussian = dynamic_cast<const ir::SampleGaussianInstr *>(instr))
        {
            if (std::holds_alternative<double>(gaussian->variance))
            {
                return std::get<double>(gaussian->variance);
            }
        }
        else if (const auto *uniform = dynamic_cast<const ir::SampleUniformInstr *>(instr))
        {
            double low = 0.0, high = 1.0;

            if (std::holds_alternative<double>(uniform->low))
            {
                low = std::get<double>(uniform->low);
            }
            if (std::holds_alternative<double>(uniform->high))
            {
                high = std::get<double>(uniform->high);
            }

            return std::pow(high - low, 2) / 12.0;
        }
        else if (const auto *bernoulli = dynamic_cast<const ir::SampleBernoulliInstr *>(instr))
        {
            double p = 0.5;

            if (std::holds_alternative<double>(bernoulli->probability))
            {
                p = std::get<double>(bernoulli->probability);
            }

            return p * (1.0 - p);
        }

        return 1.0; // Default variance
    }
} // namespace thermolang::optimizer