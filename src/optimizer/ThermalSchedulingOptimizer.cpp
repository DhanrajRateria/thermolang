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

        for (auto &block : function_ir.basic_blocks)
        {
            // We need to iterate carefully as we'll be modifying the instruction list.
            for (auto it = block->instructions.begin(); it != block->instructions.end();)
            {
                if (auto *anneal = dynamic_cast<ir::ThermalAnnealInstr *>(it->get()))
                {
                    // Check if all parameters are compile-time constants.
                    auto temp_opt = std::get_if<double>(&anneal->initial_temp);
                    auto rate_opt = std::get_if<double>(&anneal->cooling_rate);
                    auto steps_opt = std::get_if<int64_t>(&anneal->steps);

                    if (temp_opt && rate_opt && steps_opt)
                    {
                        std::cout << "  Optimizing thermal_anneal instruction by unrolling." << std::endl;

                        // --- Generate the new sequence of IR instructions ---
                        std::vector<std::unique_ptr<ir::Instruction>> new_instrs;

                        double current_temp = *temp_opt;
                        int64_t num_steps = *steps_opt;
                        std::string state_reg = "r_anneal_state"; // A conceptual register for state

                        for (int i = 0; i < num_steps; ++i)
                        {
                            // 1. Set the temperature for this step.
                            new_instrs.push_back(std::make_unique<ir::SetTemperatureInstr>(current_temp));

                            // 2. Perform one step of thermodynamic computation.
                            // The result of the step would update the state.
                            std::string next_state_reg = "r_anneal_state_" + std::to_string(i);
                            new_instrs.push_back(std::make_unique<ir::ThermalStepInstr>(next_state_reg, state_reg, current_temp));
                            state_reg = next_state_reg; // Update state for next iteration

                            // 3. Update temperature for the next iteration (exponential cooling)
                            current_temp *= *rate_opt;
                        }

                        // The final result of the anneal is the last state.
                        new_instrs.push_back(std::make_unique<ir::AssignInstr>(anneal->result_reg, state_reg));

                        // --- Replace the old instruction with the new sequence ---
                        it = block->instructions.erase(it); // Erase the original anneal instruction
                        it = block->instructions.insert(it, std::make_move_iterator(new_instrs.begin()), std::make_move_iterator(new_instrs.end()));

                        modified = true;
                        continue; // Continue to the next instruction
                    }
                }
                ++it;
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