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
        // Keep running passes until a full pass makes no changes (fixed-point iteration)
        while (changed)
        {
            changed = false;
            for (const auto &pass : passes_)
            {
                if (pass->run(function_ir))
                {
                    changed = true;
                }
            }
        }
    }

} // namespace thermolang::optimizer