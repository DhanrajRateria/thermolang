#ifndef THERMOLANG_ENERGY_FUNCTION_OPTIMIZER_H
#define THERMOLANG_ENERGY_FUNCTION_OPTIMIZER_H

#include "thermolang/optimizer/Passes.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace thermolang::optimizer
{

    class EnergyFunctionPass : public IRPass
    {
    public:
        bool run(ir::FunctionIR &function_ir) override;

    private:
        // Identify patterns in the energy function
        bool identify_quadratic_form(ir::EnergyExpression &expr,
                                     std::vector<std::vector<double>> &matrix);

        // Replace complex energy functions with simplified forms
        bool simplify_quadratic_form(ir::EnergyExpression &expr,
                                     const std::vector<std::vector<double>> &matrix);

        // Extract constants from energy expressions
        std::unordered_map<std::string, double> extract_constants(const ir::EnergyExpression &expr);

        // Check if an energy function has a minimum
        bool has_minimum(const std::vector<std::vector<double>> &matrix);

        // Optimize energy expressions for minimum dissipation
        bool optimize_for_minimum_dissipation(ir::FunctionIR &function_ir);
    };

} // namespace thermolang::optimizer

#endif // THERMOLANG_ENERGY_FUNCTION_OPTIMIZER_H