#ifndef THERMOLANG_SPU_CODE_GENERATOR_H
#define THERMOLANG_SPU_CODE_GENERATOR_H

#include "thermolang/codegen/CodeGenerator.h"
#include "thermolang/hardware/SPUSimulator.h"
#include <map>

namespace thermolang::codegen
{

    // Generates C++ code for the SPU hardware target.
    class SPUCodeGenerator : public CodeGenerator
    {
    public:
        std::string generate(const std::vector<std::unique_ptr<ir::FunctionIR>> &program) override;

    private:
        // Generates the C++ code for the SPUConfig struct initialization.
        void generate_spu_config(const ir::IsingHamiltonianInstr &ising_instr,
                                 const ir::CallInstr &anneal_call);

        // Helper to format a C++ vector literal from IR data.
        std::string format_cpp_vector(const thermolang::hardware::Vector &vec);
        std::string format_cpp_matrix(const thermolang::hardware::Matrix &mat);

        std::stringstream ss_;
    };

} // namespace thermolang::codegen

#endif // THERMOLANG_SPU_CODE_GENERATOR_H