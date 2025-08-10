#ifndef THERMOLANG_THERMAL_SCHEDULING_PASS_H
#define THERMOLANG_THERMAL_SCHEDULING_PASS_H

#include "thermolang/optimizer/Passes.h"
#include <string>
#include <vector>

namespace thermolang::optimizer
{
    class ThermalSchedulingPass : public IRPass
    {
    public:
        // Constructor initializes the program view to null.
        ThermalSchedulingPass() : program_ir_(nullptr) {}

        bool run(ir::FunctionIR &function_ir) override;

        // This method provides the pass with the necessary program-wide context.
        void set_program_ir(const std::vector<std::unique_ptr<ir::FunctionIR>> *program_ir)
        {
            program_ir_ = program_ir;
        }

    private:
        ir::FunctionIR *find_function_def(const std::string &name);

        // A pointer to the full program's IR.
        const std::vector<std::unique_ptr<ir::FunctionIR>> *program_ir_;
    };

} // namespace thermolang::optimizer

#endif // THERMOLANG_THERMAL_SCHEDULING_PASS_H