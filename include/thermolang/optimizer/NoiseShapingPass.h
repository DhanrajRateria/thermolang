#ifndef THERMOLANG_NOISE_SHAPING_PASS_H
#define THERMOLANG_NOISE_SHAPING_PASS_H

#include "thermolang/optimizer/Passes.h"
#include <vector>

namespace thermolang::optimizer
{

    class NoiseShapingPass : public IRPass
    {
    public:
        // Applies variable-temperature scaling to EBM parameters.
        // Heuristic: Higher degree -> lower temperature (higher beta).
        // Scales h_i and J_ij by beta_i. Also records per-spin temperatures.
        bool run(ir::FunctionIR &function_ir) override;
    };

} // namespace thermolang::optimizer

#endif // THERMOLANG_NOISE_SHAPING_PASS_H
