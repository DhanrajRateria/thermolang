#ifndef THERMOLANG_SPICE_CODE_GENERATOR_H
#define THERMOLANG_SPICE_CODE_GENERATOR_H

#include "thermolang/codegen/CodeGenerator.h"
#include <map>

namespace thermolang::codegen
{
    // Generates a SPICE netlist for circuit simulation.
    class SPICECodeGenerator : public CodeGenerator
    {
    public:
        std::string generate(const std::vector<std::unique_ptr<ir::FunctionIR>> &program) override;

    private:
        // Generates the netlist components for the Ising model.
        void generate_ising_netlist(const ir::IsingHamiltonianInstr &ising_instr, const ir::CallInstr &anneal_call);

        std::stringstream ss_;
    };
} // namespace thermolang::codegen

#endif // THERMOLANG_SPICE_CODE_GENERATOR_H