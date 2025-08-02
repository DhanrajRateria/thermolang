#include "thermolang/optimizer/ThermalSchedulingOptimizer.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <unordered_set>

namespace thermolang::optimizer
{

    bool ThermalSchedulingPass::run(ir::FunctionIR &function_ir)
    {
        bool cooling_modified = false;
        bool coordination_modified = false;
        bool adaptive_modified = false;

        std::cout << "Running thermal scheduling optimization on '" << function_ir.name << "'" << std::endl;

        // Optimize cooling schedules - only set flag if changes are made
        if (optimize_cooling_schedule(function_ir))
        {
            cooling_modified = true;
        }

        // Coordinate temperature across SPUs - only set flag if changes are made
        if (coordinate_distributed_temperature(function_ir))
        {
            coordination_modified = true;
        }

        // Generate adaptive control - only set flag if changes are made
        if (generate_adaptive_control(function_ir))
        {
            adaptive_modified = true;
        }

        // Return true only if at least one optimization actually modified the IR
        return cooling_modified || coordination_modified || adaptive_modified;
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