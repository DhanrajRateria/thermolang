#ifndef THERMOLANG_PASSES_H
#define THERMOLANG_PASSES_H

#include "thermolang/ir/ThermoIR.h"

namespace thermolang::optimizer
{
    // Abstract base class for all IR optimization passes.
    class IRPass
    {
    public:
        virtual ~IRPass() = default;

        // Executes the pass on a given function's IR.
        // Returns true if the IR was modified, false otherwise.
        virtual bool run(ir::FunctionIR &function_ir) = 0;
    };

    // Forward declarations of concrete passes
    class ConstantFoldingPass;
    class EnergyFunctionPass;
    class CircuitTopologyPass;
    class ThermalSchedulingPass;
    class VarianceTrackingPass;
    class NoiseShapingPass;

} // namespace thermolang::optimizer

#endif // THERMOLANG_PASSES_H