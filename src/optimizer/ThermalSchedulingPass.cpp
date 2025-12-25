#include "thermolang/optimizer/ThermalSchedulingPass.h"
#include <iostream>
#include <cmath>
#include <variant>

namespace thermolang::optimizer
{
    ir::FunctionIR *ThermalSchedulingPass::find_function_def(const std::string &name)
    {
        if (!program_ir_)
            return nullptr;
        for (const auto &func : *program_ir_)
        {
            if (func->name == name)
            {
                return func.get();
            }
        }
        return nullptr;
    }

    bool ThermalSchedulingPass::run(ir::FunctionIR &function_ir)
    {
        // This pass needs the context of the whole program to do its job.
        if (!program_ir_)
            return false;

        bool modified = false;

        for (auto &block : function_ir.basic_blocks)
        {
            for (auto &instr_ptr : block->instructions)
            {
                if (auto *call = dynamic_cast<ir::CallInstr *>(instr_ptr.get()))
                {
                    if (call->callee_name != "thermal_anneal" || call->args.size() != 4)
                        continue;

                    // Fix: Explicitly define the variant type for energy_func_op
                    const auto &energy_func_op = call->args[0];

                    // Check if it's a string type in the variant
                    if (!std::holds_alternative<std::string>(energy_func_op))
                        continue;

                    std::string energy_func_name = std::get<std::string>(energy_func_op);
                    auto *energy_func_ir = find_function_def(energy_func_name);
                    if (!energy_func_ir)
                        continue;

                    int num_variables = energy_func_ir->parameters.size();
                    if (num_variables == 0)
                        continue;

                    double new_initial_temp = 2.5 * log(num_variables);
                    int64_t new_steps = 1000 * num_variables;

                    std::cout << "  ThermalSchedulingPass: Optimizing anneal for '" << energy_func_name
                              << "' (" << num_variables << " vars)." << std::endl;
                    std::cout << "    - New Initial Temp: " << new_initial_temp << " (was user-defined)" << std::endl;
                    std::cout << "    - New Steps: " << new_steps << " (was user-defined)" << std::endl;

                    // Fix: Explicitly check if these are string types
                    if (!std::holds_alternative<std::string>(call->args[1]) ||
                        !std::holds_alternative<std::string>(call->args[3]))
                        continue;

                    auto temp_reg = std::get<std::string>(call->args[1]);
                    auto steps_reg = std::get<std::string>(call->args[3]);

                    // Search the entire function for the definitions of these registers and update them.
                    for (auto &b : function_ir.basic_blocks)
                    {
                        for (auto &i : b->instructions)
                        {
                            if (auto *load = dynamic_cast<ir::LoadConstInstr *>(i.get()))
                            {
                                if (load->result_reg == temp_reg)
                                {
                                    bool needs_update = true;
                                    // Check if value is already close enough
                                    if (std::holds_alternative<double>(load->value))
                                    {
                                        if (std::abs(std::get<double>(load->value) - new_initial_temp) < 1e-6)
                                            needs_update = false;
                                    }
                                    if (needs_update)
                                    {
                                        load->value = new_initial_temp;
                                        modified = true;
                                    }
                                }
                                if (load->result_reg == steps_reg)
                                {
                                    bool needs_update = true;
                                    if (std::holds_alternative<int64_t>(load->value))
                                    {
                                        if (std::get<int64_t>(load->value) == new_steps)
                                            needs_update = false;
                                    }
                                    if (needs_update)
                                    {
                                        load->value = new_steps;
                                        modified = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return modified;
    }
} // namespace thermolang::optimizer