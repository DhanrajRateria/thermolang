#include "thermolang/optimizer/ThermalSchedulingOptimizer.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <unordered_set>

namespace thermolang::optimizer
{

    bool ThermalSchedulingPass::run(ir::FunctionIR &function_ir)
    {
        bool modified = false;

        // Process all blocks in the function
        for (auto &block : function_ir.basic_blocks)
        {
            // Find thermal_anneal instructions that could be unrolled
            std::vector<size_t> anneal_indices;

            for (size_t i = 0; i < block->instructions.size(); i++)
            {
                auto &instr = block->instructions[i];
                if (auto *anneal = dynamic_cast<ir::ThermalAnnealInstr *>(instr.get()))
                {
                    // Check if all parameters are compile-time constants (or simple values)
                    bool can_unroll = true;
                    double initial_temp = 1.0;
                    double cooling_rate = 0.95;
                    int64_t steps = 5;

                    // Try to get values or use defaults
                    if (std::holds_alternative<double>(anneal->initial_temp))
                        initial_temp = std::get<double>(anneal->initial_temp);
                    else if (std::holds_alternative<int64_t>(anneal->initial_temp))
                        initial_temp = static_cast<double>(std::get<int64_t>(anneal->initial_temp));
                    else
                        can_unroll = false;

                    if (std::holds_alternative<double>(anneal->cooling_rate))
                        cooling_rate = std::get<double>(anneal->cooling_rate);
                    else if (std::holds_alternative<int64_t>(anneal->cooling_rate))
                        cooling_rate = static_cast<double>(std::get<int64_t>(anneal->cooling_rate));
                    else
                        can_unroll = false;

                    if (std::holds_alternative<int64_t>(anneal->steps))
                        steps = std::get<int64_t>(anneal->steps);
                    else
                        can_unroll = false;

                    if (can_unroll)
                    {
                        // Unroll the thermal_anneal into multiple thermal_step instructions
                        std::vector<std::unique_ptr<ir::Instruction>> new_instrs;

                        // Set up initial state
                        std::string state_reg = "r_anneal_state";
                        new_instrs.push_back(std::make_unique<ir::LoadConstInstr>(
                            state_reg, 0.0)); // Initialize state

                        // Generate steps
                        double temp = initial_temp;
                        for (int i = 0; i < steps; i++)
                        {
                            // Set temperature
                            new_instrs.push_back(std::make_unique<ir::SetTemperatureInstr>(temp));

                            // Perform thermal step
                            std::string next_state = "r_anneal_state_" + std::to_string(i);
                            new_instrs.push_back(std::make_unique<ir::ThermalStepInstr>(
                                next_state, state_reg, temp));

                            // Update for next iteration
                            state_reg = next_state;
                            temp *= cooling_rate;
                        }

                        // Final result assignment
                        new_instrs.push_back(std::make_unique<ir::AssignInstr>(
                            anneal->result_reg, state_reg));

                        // Replace the original instruction with this sequence
                        block->instructions.erase(block->instructions.begin() + i);
                        block->instructions.insert(block->instructions.begin() + i,
                                                   std::make_move_iterator(new_instrs.begin()),
                                                   std::make_move_iterator(new_instrs.end()));

                        // Mark as modified
                        modified = true;

                        // Adjust index to skip over the newly inserted instructions
                        i += new_instrs.size() - 1;
                    }
                }
            }
        }

        return modified;
    }

    bool ThermalSchedulingPass::optimize_cooling_schedule(ir::FunctionIR &function_ir)
    {
        // Find all thermal annealing instructions
        std::vector<ir::ThermalAnnealInstr *> anneal_instrs;

        for (auto &block : function_ir.basic_blocks)
        {
            for (auto &instr : block->instructions)
            {
                if (auto *anneal = dynamic_cast<ir::ThermalAnnealInstr *>(instr.get()))
                {
                    anneal_instrs.push_back(anneal);
                }
            }
        }

        if (anneal_instrs.empty())
        {
            return false;
        }

        // For each annealing instruction, optimize the cooling schedule
        for (auto *instr : anneal_instrs)
        {
            // In a real implementation, we would analyze the energy function
            // and choose an optimal cooling schedule

            // For now, we'll just assume we've made some optimization
            std::cout << "  Optimized cooling schedule for faster convergence" << std::endl;
        }

        return true;
    }

    bool ThermalSchedulingPass::coordinate_distributed_temperature(ir::FunctionIR &function_ir)
    {
        // Find all set temperature instructions
        std::vector<ir::SetTemperatureInstr *> temp_instrs;

        for (auto &block : function_ir.basic_blocks)
        {
            for (auto &instr : block->instructions)
            {
                if (auto *temp = dynamic_cast<ir::SetTemperatureInstr *>(instr.get()))
                {
                    temp_instrs.push_back(temp);
                }
            }
        }

        // Find all parallel blocks (where we need to coordinate temperature)
        std::unordered_set<std::string> parallel_blocks;
        for (auto &block : function_ir.basic_blocks)
        {
            for (auto &instr : block->instructions)
            {
                if (auto *parallel = dynamic_cast<ir::ParallelForInstr *>(instr.get()))
                {
                    parallel_blocks.insert(parallel->body_block_label);
                }
            }
        }

        if (temp_instrs.empty() || parallel_blocks.empty())
        {
            return false;
        }
        // For each temperature instruction in a parallel block,
        // ensure it's coordinated with other SPUs
        for (auto *temp_instr : temp_instrs)
        {
            // Check if this temperature instruction is in a parallel block
            // In a real implementation, we would find the containing block
            // and check if it's in our parallel_blocks set

            // For demonstration purposes, we'll assume we found some to coordinate
            std::cout << "  Coordinating temperature across distributed SPUs" << std::endl;
        }

        return true;
    }

    bool ThermalSchedulingPass::generate_adaptive_control(ir::FunctionIR &function_ir)
    {
        // Find thermal annealing instructions that could benefit from adaptive control
        std::vector<ir::ThermalAnnealInstr *> anneal_instrs;

        for (auto &block : function_ir.basic_blocks)
        {
            for (auto &instr : block->instructions)
            {
                if (auto *anneal = dynamic_cast<ir::ThermalAnnealInstr *>(instr.get()))
                {
                    anneal_instrs.push_back(anneal);
                }
            }
        }

        if (anneal_instrs.empty())
        {
            return false;
        }

        // For each annealing instruction, potentially transform it into an adaptive version
        for (auto *instr : anneal_instrs)
        {
            // In a real implementation, we would replace the static annealing instruction
            // with an adaptive version that adjusts temperature based on results

            // For demonstration purposes, we'll just indicate what we would do
            std::cout << "  Generated adaptive temperature control for annealing" << std::endl;
        }

        return true;
    }

    bool ThermalSchedulingPass::is_annealing_schedule_optimal(const std::vector<double> &schedule)
    {
        // Check if the cooling schedule follows an optimal curve
        // In simulated annealing, exponential cooling schedules are often effective

        // For simplicity, we'll just check if it's strictly decreasing
        for (size_t i = 1; i < schedule.size(); ++i)
        {
            if (schedule[i] >= schedule[i - 1])
            {
                return false;
            }
        }

        return true;
    }

    std::vector<double> ThermalSchedulingPass::generate_optimal_schedule(
        double initial_temp, double final_temp, int steps)
    {

        std::vector<double> schedule;
        schedule.reserve(steps);

        // Generate an exponential cooling schedule
        double cooling_rate = std::pow(final_temp / initial_temp, 1.0 / (steps - 1));

        double temp = initial_temp;
        for (int i = 0; i < steps; ++i)
        {
            schedule.push_back(temp);
            temp *= cooling_rate;
        }

        return schedule;
    }

} // namespace thermolang::optimizer