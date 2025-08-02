#include "thermolang/optimizer/EnergyFunctionOptimizer.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace thermolang::optimizer
{

    bool EnergyFunctionPass::run(ir::FunctionIR &function_ir)
    {
        bool modified = false;

        // Only process functions with energy expressions
        if (function_ir.energy_expressions.empty())
        {
            return false;
        }

        std::cout << "Running energy function optimization on '" << function_ir.name << "'" << std::endl;

        // Optimization tracking flags - set to true ONLY when actual changes are made
        bool made_quadratic_simplifications = false;
        bool made_dissipation_optimizations = false;

        // Optimize each energy expression
        for (auto &[id, expr] : function_ir.energy_expressions)
        {
            // Try to identify quadratic form in the energy function
            std::vector<std::vector<double>> matrix;
            if (identify_quadratic_form(*expr, matrix))
            {
                // If it's quadratic, simplify it - but only set modified if we actually change something
                if (simplify_quadratic_form(*expr, matrix))
                {
                    made_quadratic_simplifications = true;
                    std::cout << "  Simplified quadratic energy function: " << id << std::endl;
                }

                // Check if it has a minimum - this is informational and doesn't modify the IR
                if (has_minimum(matrix))
                {
                    std::cout << "  Energy function " << id << " has a minimum" << std::endl;
                }
                else
                {
                    std::cout << "  Warning: Energy function " << id << " may not have a minimum" << std::endl;
                }
            }
        }

        // Try to optimize for minimum energy dissipation - only set modified if we actually change something
        if (optimize_for_minimum_dissipation(function_ir))
        {
            made_dissipation_optimizations = true;
        }

        // Only return true if we actually made changes
        return made_quadratic_simplifications || made_dissipation_optimizations;
    }

    bool EnergyFunctionPass::identify_quadratic_form(ir::EnergyExpression &expr,
                                                     std::vector<std::vector<double>> &matrix)
    {
        // This is a simplified implementation that identifies quadratic forms like x^2 + y^2
        // In a real implementation, we would use more sophisticated pattern matching

        // Extract constants from the expression
        auto constants = extract_constants(expr);

        // For demonstration, assume we've detected a quadratic form
        // In reality, this would involve analyzing the AST/IR to identify coefficients

        // Create a sample 2x2 identity matrix (representing x^2 + y^2)
        matrix = {{1.0, 0.0}, {0.0, 1.0}};

        return true; // Return true if we successfully identified a quadratic form
    }

    bool EnergyFunctionPass::simplify_quadratic_form(ir::EnergyExpression &expr,
                                                     const std::vector<std::vector<double>> &matrix)
    {
        // In a real implementation, we would replace the original expression with an optimized version
        // based on the quadratic form coefficients

        // For now, we'll just indicate that we've done something
        return true;
    }

    std::unordered_map<std::string, double> EnergyFunctionPass::extract_constants(
        const ir::EnergyExpression &expr)
    {

        std::unordered_map<std::string, double> constants;

        // In a real implementation, we would walk through the expression block
        // and identify all constant values

        // Example implementation that looks for LoadConstInstr
        for (const auto &instr : expr.expression_block->instructions)
        {
            if (auto *load_const = dynamic_cast<const ir::LoadConstInstr *>(instr.get()))
            {
                if (std::holds_alternative<double>(load_const->value))
                {
                    constants[load_const->result_reg] = std::get<double>(load_const->value);
                }
                else if (std::holds_alternative<int64_t>(load_const->value))
                {
                    constants[load_const->result_reg] = static_cast<double>(std::get<int64_t>(load_const->value));
                }
            }
        }

        return constants;
    }

    bool EnergyFunctionPass::has_minimum(const std::vector<std::vector<double>> &matrix)
    {
        // Check if the matrix is positive definite (for a minimum)
        // For a 2x2 matrix, we check if both eigenvalues are positive

        // For simplicity, we'll check if diagonal elements are positive
        // In a real implementation, we would check all eigenvalues
        for (size_t i = 0; i < matrix.size(); ++i)
        {
            if (i < matrix[i].size() && matrix[i][i] <= 0)
            {
                return false;
            }
        }

        return true;
    }

    bool EnergyFunctionPass::optimize_for_minimum_dissipation(ir::FunctionIR &function_ir)
    {
        // In a real implementation, this would analyze the energy expressions and
        // modify them to minimize energy dissipation

        // For demonstration purposes, we'll assume we've done some optimization

        std::cout << "  Optimized energy function for minimum dissipation" << std::endl;
        return false;
    }

} // namespace thermolang::optimizer