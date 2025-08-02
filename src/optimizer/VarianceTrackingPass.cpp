#include "thermolang/optimizer/VarianceTrackingPass.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace thermolang::optimizer
{

    bool VarianceTrackingPass::run(ir::FunctionIR &function_ir)
    {
        // Only apply to stochastic functions
        if (!function_ir.is_stochastic)
        {
            return false;
        }

        std::cout << "Running variance tracking on stochastic function '"
                  << function_ir.name << "'" << std::endl;

        // Propagate variance through the function
        auto variances = propagate_variance(function_ir);

        if (variances.empty())
        {
            return false;
        }

        // Insert variance tracking instructions - only return true if we actually added instructions
        bool modified = insert_variance_tracking(function_ir, variances);

        return modified;
    }

    std::unordered_map<std::string, double> VarianceTrackingPass::propagate_variance(
        ir::FunctionIR &function_ir)
    {

        std::unordered_map<std::string, double> variances;
        std::unordered_map<std::string, double> means;

        // For each basic block, propagate variance through instructions
        for (auto &block : function_ir.basic_blocks)
        {
            for (auto &instr : block->instructions)
            {
                if (auto *gaussian = dynamic_cast<ir::SampleGaussianInstr *>(instr.get()))
                {
                    // For Gaussian sampling, the variance is explicitly given
                    if (std::holds_alternative<double>(gaussian->variance))
                    {
                        double variance = std::get<double>(gaussian->variance);
                        variances[gaussian->result_reg] = variance;
                    }
                    else if (std::holds_alternative<std::string>(gaussian->variance))
                    {
                        // If variance is a register, look up its value if known
                        std::string var_reg = std::get<std::string>(gaussian->variance);
                        if (variances.count(var_reg))
                        {
                            variances[gaussian->result_reg] = variances[var_reg];
                        }
                        else
                        {
                            // Default to 1.0 if unknown
                            variances[gaussian->result_reg] = 1.0;
                        }
                    }

                    // Track the mean for later variance computations
                    if (std::holds_alternative<double>(gaussian->mean))
                    {
                        means[gaussian->result_reg] = std::get<double>(gaussian->mean);
                    }
                    else
                    {
                        // Default to 0.0 if unknown
                        means[gaussian->result_reg] = 0.0;
                    }
                }
                else if (auto *uniform = dynamic_cast<ir::SampleUniformInstr *>(instr.get()))
                {
                    // For uniform distribution, variance is (high-low)^2 / 12
                    double low = 0.0, high = 1.0;

                    if (std::holds_alternative<double>(uniform->low))
                    {
                        low = std::get<double>(uniform->low);
                    }

                    if (std::holds_alternative<double>(uniform->high))
                    {
                        high = std::get<double>(uniform->high);
                    }

                    // Compute variance using formula for uniform distribution
                    double variance = std::pow(high - low, 2) / 12.0;
                    variances[uniform->result_reg] = variance;

                    // Mean is (high + low) / 2
                    means[uniform->result_reg] = (high + low) / 2.0;
                }
                else if (auto *bernoulli = dynamic_cast<ir::SampleBernoulliInstr *>(instr.get()))
                {
                    // For Bernoulli distribution, variance is p*(1-p)
                    double p = 0.5; // Default probability

                    if (std::holds_alternative<double>(bernoulli->probability))
                    {
                        p = std::get<double>(bernoulli->probability);
                    }

                    double variance = p * (1.0 - p);
                    variances[bernoulli->result_reg] = variance;

                    // Mean is p
                    means[bernoulli->result_reg] = p;
                }
                else if (auto *binop = dynamic_cast<ir::BinaryOpInstr *>(instr.get()))
                {
                    // Propagate variance through binary operations
                    std::string left_reg, right_reg;
                    if (std::holds_alternative<std::string>(binop->arg1))
                    {
                        left_reg = std::get<std::string>(binop->arg1);
                    }
                    if (std::holds_alternative<std::string>(binop->arg2))
                    {
                        right_reg = std::get<std::string>(binop->arg2);
                    }

                    bool has_left_variance = variances.count(left_reg) > 0;
                    bool has_right_variance = variances.count(right_reg) > 0;

                    if (has_left_variance || has_right_variance)
                    {
                        double left_variance = has_left_variance ? variances[left_reg] : 0.0;
                        double right_variance = has_right_variance ? variances[right_reg] : 0.0;

                        if (binop->opcode == ir::OpCode::ADD || binop->opcode == ir::OpCode::SUB)
                        {
                            // For addition/subtraction, variances add
                            variances[binop->result_reg] = compute_addition_variance(left_variance, right_variance);

                            // For means, add or subtract
                            double left_mean = means.count(left_reg) ? means[left_reg] : 0.0;
                            double right_mean = means.count(right_reg) ? means[right_reg] : 0.0;

                            if (binop->opcode == ir::OpCode::ADD)
                            {
                                means[binop->result_reg] = left_mean + right_mean;
                            }
                            else
                            {
                                means[binop->result_reg] = left_mean - right_mean;
                            }
                        }
                        else if (binop->opcode == ir::OpCode::MUL)
                        {
                            // For multiplication, variance computation is more complex
                            double left_mean = means.count(left_reg) ? means[left_reg] : 0.0;
                            double right_mean = means.count(right_reg) ? means[right_reg] : 0.0;

                            variances[binop->result_reg] = compute_multiplication_variance(
                                left_mean, right_mean, left_variance, right_variance);

                            // For means, multiply
                            means[binop->result_reg] = left_mean * right_mean;
                        }
                        else if (binop->opcode == ir::OpCode::DIV)
                        {
                            // Division is complex - for simplicity we'll just put a placeholder
                            // In a real implementation, we'd use the proper error propagation formulas
                            variances[binop->result_reg] = 1.0;

                            // For means, divide
                            double left_mean = means.count(left_reg) ? means[left_reg] : 0.0;
                            double right_mean = means.count(right_reg) ? means[right_reg] : 1.0;

                            if (right_mean != 0.0)
                            {
                                means[binop->result_reg] = left_mean / right_mean;
                            }
                            else
                            {
                                means[binop->result_reg] = 0.0;
                            }
                        }
                    }
                }
                else if (auto *call = dynamic_cast<ir::CallInstr *>(instr.get()))
                {
                    // For function calls, we would need to look up the function's variance properties
                    // For simplicity, we'll assume all function calls have a default variance
                    if (call->result_reg.has_value())
                    {
                        variances[*call->result_reg] = 1.0;
                        means[*call->result_reg] = 0.0;
                    }
                }
            }
        }

        std::cout << "  Propagated variance for " << variances.size() << " registers" << std::endl;
        return variances;
    }

    bool VarianceTrackingPass::insert_variance_tracking(
        ir::FunctionIR &function_ir,
        const std::unordered_map<std::string, double> &variances)
    {

        bool modified = false;

        // For each basic block, insert variance tracking instructions where appropriate
        for (auto &block : function_ir.basic_blocks)
        {
            // We'll need to collect instructions to insert
            std::vector<std::pair<size_t, std::unique_ptr<ir::Instruction>>> insertions;

            for (size_t i = 0; i < block->instructions.size(); ++i)
            {
                const auto &instr = block->instructions[i];

                // If this is a stochastic operation, add a variance tracking instruction after it
                if (dynamic_cast<ir::SampleGaussianInstr *>(instr.get()) ||
                    dynamic_cast<ir::SampleUniformInstr *>(instr.get()) ||
                    dynamic_cast<ir::SampleBernoulliInstr *>(instr.get()))
                {

                    // Get the result register
                    std::string result_reg;
                    if (auto *gaussian = dynamic_cast<ir::SampleGaussianInstr *>(instr.get()))
                    {
                        result_reg = gaussian->result_reg;
                    }
                    else if (auto *uniform = dynamic_cast<ir::SampleUniformInstr *>(instr.get()))
                    {
                        result_reg = uniform->result_reg;
                    }
                    else if (auto *bernoulli = dynamic_cast<ir::SampleBernoulliInstr *>(instr.get()))
                    {
                        result_reg = bernoulli->result_reg;
                    }

                    // If we know the variance, add a tracking instruction
                    if (variances.count(result_reg) > 0)
                    {
                        double variance = variances.at(result_reg);

                        // Add after this instruction (i+1)
                        insertions.emplace_back(i + 1, std::make_unique<ir::VarianceTrackInstr>(
                                                           result_reg, result_reg, variance));

                        modified = true;
                    }
                }
            }

            // Now insert all the tracking instructions
            // Note: Insert from back to front so indexes remain valid
            std::sort(insertions.begin(), insertions.end(),
                      [](const auto &a, const auto &b)
                      { return a.first > b.first; });

            for (const auto &[index, instr] : insertions)
            {
                block->instructions.insert(block->instructions.begin() + index, std::move(const_cast<std::unique_ptr<ir::Instruction> &>(instr)));
            }
        }

        return modified;
    }

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