#ifndef THERMOLANG_VARIANCE_TRACKING_PASS_H
#define THERMOLANG_VARIANCE_TRACKING_PASS_H

#include "thermolang/optimizer/Passes.h"
#include <unordered_map>
#include <string>
#include <memory>

namespace thermolang::optimizer
{

    class VarianceTrackingPass : public IRPass
    {
    public:
        bool run(ir::FunctionIR &function_ir) override;

    private:
        // Track the variance of each register through the program
        std::unordered_map<std::string, double> propagate_variance(ir::FunctionIR &function_ir);

        // Insert variance tracking instructions where needed
        bool insert_variance_tracking(ir::FunctionIR &function_ir,
                                      const std::unordered_map<std::string, double> &variances);

        // Compute output variance for specific operations
        double compute_addition_variance(double var1, double var2);
        double compute_multiplication_variance(double mean1, double mean2, double var1, double var2);
        double compute_sampling_variance(const ir::Instruction *instr);
    };

} // namespace thermolang::optimizer

#endif // THERMOLANG_VARIANCE_TRACKING_PASS_H