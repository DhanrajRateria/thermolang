#ifndef THERMOLANG_OPTIMIZATION_MANAGER_H
#define THERMOLANG_OPTIMIZATION_MANAGER_H

#include "thermolang/ir/ThermoIR.h"
#include "thermolang/optimizer/Passes.h"
#include <vector>
#include <memory>

namespace thermolang::optimizer
{
    class OptimizationManager
    {
    public:
        // Simpler non-template version that takes a base class pointer
        void add_pass(std::unique_ptr<IRPass> pass);

        // Runs all registered passes over a single function's IR.
        void run(ir::FunctionIR &function_ir);

    private:
        std::vector<std::unique_ptr<IRPass>> passes_;
    };

} // namespace thermolang::optimizer

#endif // THERMOLANG_OPTIMIZATION_MANAGER_H