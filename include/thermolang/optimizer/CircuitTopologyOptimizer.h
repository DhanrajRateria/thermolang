#ifndef THERMOLANG_CIRCUIT_TOPOLOGY_OPTIMIZER_H
#define THERMOLANG_CIRCUIT_TOPOLOGY_OPTIMIZER_H

#include "thermolang/optimizer/Passes.h"
#include <vector>
#include <memory>

namespace thermolang::optimizer
{

    class CircuitTopologyPass : public IRPass
    {
    public:
        bool run(ir::FunctionIR &function_ir) override;

    private:
        bool optimize_circuit_topology(ir::FunctionIR &function_ir, ir::DiscreteEBMInstr *ebm);
        bool optimize_couplings(ir::FunctionIR &function_ir);
        bool partition_computation(ir::FunctionIR &function_ir, int parallel_units = 4);
    };

} // namespace thermolang::optimizer

#endif // THERMOLANG_CIRCUIT_TOPOLOGY_OPTIMIZER_H