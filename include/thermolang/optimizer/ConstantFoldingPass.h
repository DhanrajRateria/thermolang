#ifndef THERMOLANG_CONSTANT_FOLDING_PASS_H
#define THERMOLANG_CONSTANT_FOLDING_PASS_H

#include "thermolang/optimizer/Passes.h"
#include <unordered_map>

namespace thermolang::optimizer
{
    class ConstantFoldingPass : public IRPass
    {
    public:
        bool run(ir::FunctionIR &function_ir) override;
    };
} // namespace thermolang::optimizer

#endif // THERMOLANG_CONSTANT_FOLDING_PASS_H