#ifndef THERMOLANG_ISING_MODEL_PASS_H
#define THERMOLANG_ISING_MODEL_PASS_H

#include "thermolang/optimizer/Passes.h"
#include <map>
#include <string>

namespace thermolang::optimizer
{

    class IsingModelPass : public IRPass
    {
    public:
        bool run(ir::FunctionIR &function_ir) override;

    private:
        // Helper to find the instruction that defines a specific register
        ir::Instruction *find_def(ir::FunctionIR &function_ir, const std::string &reg);
    };

} // namespace thermolang::optimizer

#endif // THERMOLANG_ISING_MODEL_PASS_H