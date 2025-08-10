#ifndef THERMOLANG_PARALLELIZATION_PASS_H
#define THERMOLANG_PARALLELIZATION_PASS_H

#include "thermolang/optimizer/Passes.h"

namespace thermolang::optimizer
{
    // Placeholder for future parallel optimizations (e.g., load balancing).
    // The main translation is now handled by the IR Generator.
    class ParallelizationPass : public IRPass
    {
    public:
        bool run(ir::FunctionIR &function_ir) override;
    };
}

#endif