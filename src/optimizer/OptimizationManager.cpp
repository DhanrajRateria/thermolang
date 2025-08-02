#include "thermolang/optimizer/OptimizationManager.h"
#include <iostream>

namespace thermolang::optimizer
{
    void OptimizationManager::add_pass(std::unique_ptr<IRPass> pass)
    {
        passes_.push_back(std::move(pass));
    }

    void OptimizationManager::run(ir::FunctionIR &function_ir)
    {
        std::cout << "Running optimization passes on function '" << function_ir.name << "'...\n";
        bool changed = true;
        int iteration_count = 0;
        const int MAX_ITERATIONS = 5; // Prevent infinite loops
        // Keep running passes until a full pass makes no changes (fixed-point iteration)
        while (changed && iteration_count < MAX_ITERATIONS)
        {
            changed = false;
            iteration_count++;

            for (const auto &pass : passes_)
            {
                if (pass->run(function_ir))
                {
                    changed = true;
                }
            }
        }

        if (iteration_count >= MAX_ITERATIONS)
        {
            std::cout << "Warning: Reached maximum optimization iterations for "
                      << function_ir.name << std::endl;
        }
    }

} // namespace thermolang::optimizer