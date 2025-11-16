#ifndef THERMOLANG_DISCRETE_EBM_ANALYSIS_PASS_H
#define THERMOLANG_DISCRETE_EBM_ANALYSIS_PASS_H

#include "thermolang/optimizer/Passes.h"
#include <map>
#include <string>

namespace thermolang::optimizer
{

    class DiscreteEBMAnalysisPass : public IRPass
    {
    public:
        bool run(ir::FunctionIR &function_ir) override;

    private:
        // Helper to find the instruction that defines a specific register
        ir::Instruction *find_def(ir::FunctionIR &function_ir, const std::string &reg);
    };

} // namespace thermolang::optimizer

#endif // THERMOLANG_DISCRETE_EBM_ANALYSIS_PASS_H