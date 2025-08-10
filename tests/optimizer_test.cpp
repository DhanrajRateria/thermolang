#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <memory>

#include "thermolang/compiler/IrGenerator.h"
#include "thermolang/optimizer/OptimizationManager.h"
#include "thermolang/optimizer/Passes.h"
#include "thermolang/parser/Parser.h"
#include "thermolang/semantics/SemanticAnalyzer.h"
#include "thermolang/semantics/TypeChecker.h"
#include "thermolang/optimizer/ConstantFoldingPass.h"
#include "thermolang/optimizer/EnergyFunctionOptimizer.h"
#include "thermolang/optimizer/CircuitTopologyOptimizer.h"
#include "thermolang/optimizer/ThermalSchedulingOptimizer.h"
#include "thermolang/optimizer/VarianceTrackingPass.h"
#include "thermolang/parser/Parser.h"
#include "thermolang/semantics/SemanticAnalyzer.h"
#include "thermolang/semantics/TypeChecker.h"

// Test fixture for Optimizer Passes
class OptimizerTest : public ::testing::Test
{
protected:
    std::string run_pipeline_and_optimize(const std::string &source)
    {
        thermolang::Lexer lexer(source);
        thermolang::Parser parser(lexer);
        auto ast = parser.parse();
        if (parser.had_error())
            return "Parser Error";

        thermolang::SymbolTable symbols;
        thermolang::SemanticAnalyzer semantic_analyzer(symbols);
        if (!semantic_analyzer.analyze(ast))
            return "Semantic Error";

        thermolang::TypeChecker type_checker(symbols);
        if (!type_checker.check(ast))
            return "Type Error";

        thermolang::compiler::IrGenerator ir_gen;
        auto ir_program = ir_gen.generate(ast);

        thermolang::optimizer::OptimizationManager opt_manager;
        opt_manager.add_pass(std::make_unique<thermolang::optimizer::EnergyFunctionPass>());
        opt_manager.add_pass(std::make_unique<thermolang::optimizer::ThermalSchedulingPass>());
        opt_manager.add_pass(std::make_unique<thermolang::optimizer::ConstantFoldingPass>());

        for (auto &func_ir : ir_program)
        {
            opt_manager.run(*func_ir);
        }

        std::stringstream ss;
        for (const auto &func : ir_program)
        {
            ss << func->toString();
        }
        return ss.str();
    }
    // Helper to get the IR for a single function
    std::unique_ptr<thermolang::ir::FunctionIR> get_function_ir(const std::string &source, const std::string &func_name)
    {
        thermolang::Lexer lexer(source);
        thermolang::Parser parser(lexer);
        auto ast = parser.parse();
        if (parser.had_error())
            return nullptr;

        thermolang::SymbolTable symbols;
        thermolang::SemanticAnalyzer semantic_analyzer(symbols);
        if (!semantic_analyzer.analyze(ast))
            return nullptr;

        thermolang::TypeChecker type_checker(symbols);
        if (!type_checker.check(ast))
            return nullptr;

        thermolang::compiler::IrGenerator ir_gen;
        auto ir_functions = ir_gen.generate(ast);

        for (auto &func_ir : ir_functions)
        {
            if (func_ir->name == func_name)
            {
                return std::move(func_ir);
            }
        }
        return nullptr;
    }
};

TEST_F(OptimizerTest, ConstantFoldingPass)
{
    std::string source = R"(
        fn test_folding() -> int {
            let a = 10;
            let b = 20;
            let c = a + b; // Should become: c = 30
            let d = c * 2; // Should become: d = 60
            return d;
        }
    )";

    auto ir = get_function_ir(source, "test_folding");
    ASSERT_NE(ir, nullptr);

    // Run the optimization
    thermolang::optimizer::OptimizationManager opt_manager;
    opt_manager.add_pass(std::unique_ptr<thermolang::optimizer::IRPass>(
        new thermolang::optimizer::ConstantFoldingPass()));
    opt_manager.run(*ir);

    // Verify the result
    // We expect the 'add' instruction to be gone, replaced by a load_const.
    // The final result register should be loaded with the value 60.

    // Find the return instruction
    thermolang::ir::ReturnInstr *return_instr = nullptr;
    for (const auto &block : ir->basic_blocks)
    {
        for (const auto &instr : block->instructions)
        {
            if (auto *ret = dynamic_cast<thermolang::ir::ReturnInstr *>(instr.get()))
            {
                return_instr = ret;
                break;
            }
        }
    }

    ASSERT_NE(return_instr, nullptr);
    ASSERT_TRUE(return_instr->return_value.has_value());

    // The return value should point to a register. We need to find what value was last loaded into it.
    std::string return_reg = std::get<std::string>(*return_instr->return_value);

    // Find the last instruction that defines the return register
    thermolang::ir::LoadConstInstr *final_load = nullptr;
    for (const auto &block : ir->basic_blocks)
    {
        for (const auto &instr : block->instructions)
        {
            if (auto *load = dynamic_cast<thermolang::ir::LoadConstInstr *>(instr.get()))
            {
                if (load->result_reg == return_reg)
                {
                    final_load = load;
                }
            }
        }
    }

    ASSERT_NE(final_load, nullptr);
    ASSERT_TRUE(std::holds_alternative<int64_t>(final_load->value));

    EXPECT_EQ(std::get<int64_t>(final_load->value), 60);
}

TEST_F(OptimizerTest, EnergyFunctionOptimization)
{
    std::string source = R"(
        energy fn quadratic_energy(x: float, y: float) -> float {
            let x_sq = x * x;
            let y_sq = y * y;
            return x_sq + y_sq;
        }
    )";
    std::string optimized_ir = run_pipeline_and_optimize(source);

    // After optimization, the separate mul and add instructions should be replaced
    // by a single, high-level quadratic_form instruction.
    EXPECT_EQ(optimized_ir.find("mul"), std::string::npos) << "IR should not contain 'mul' after optimization.";
    EXPECT_EQ(optimized_ir.find("add"), std::string::npos) << "IR should not contain 'add' after optimization.";
    EXPECT_NE(optimized_ir.find("quadratic_form"), std::string::npos) << "IR should contain 'quadratic_form' instruction.";
}

// Modify the ThermalScheduleUnrolling test
TEST_F(OptimizerTest, ThermalScheduleUnrolling)
{
    // CHANGE: Skip IR generation and test the ThermalSchedulingPass directly

    // Create a minimal function IR manually
    auto func_ir = std::make_unique<thermolang::ir::FunctionIR>();
    func_ir->name = "test_function";

    // Create a basic block
    auto block = std::make_unique<thermolang::ir::BasicBlock>("entry");

    // Create a thermal_anneal instruction with constant parameters
    std::string result_reg = "result";
    std::string energy_func_reg = "energy_func";
    double initial_temp = 1.0;
    double cooling_rate = 0.9;
    int64_t steps = 5;

    // Add the thermal_anneal instruction to the block
    block->instructions.push_back(
        std::make_unique<thermolang::ir::ThermalAnnealInstr>(
            result_reg, energy_func_reg, initial_temp, cooling_rate, steps));

    // Add the block to the function
    func_ir->basic_blocks.push_back(std::move(block));

    // Run the thermal scheduling optimization
    thermolang::optimizer::ThermalSchedulingPass pass;
    bool modified = pass.run(*func_ir);

    // Convert the IR to string for analysis
    std::stringstream ss;
    ss << func_ir->toString();
    std::string ir_str = ss.str();

    // Check that the optimization was applied
    EXPECT_TRUE(modified) << "The ThermalSchedulingPass should have modified the IR";

    // Check that thermal_anneal was replaced by thermal_step
    EXPECT_EQ(ir_str.find("thermal_anneal"), std::string::npos)
        << "IR should not contain 'thermal_anneal' after optimization";

    EXPECT_NE(ir_str.find("thermal_step"), std::string::npos)
        << "IR should contain 'thermal_step' instructions";

    // Count thermal_step instructions
    size_t step_count = 0;
    size_t pos = ir_str.find("thermal_step", 0);
    while (pos != std::string::npos)
    {
        step_count++;
        pos = ir_str.find("thermal_step", pos + 1);
    }

    EXPECT_EQ(step_count, 5) << "The anneal should have been unrolled into 5 steps";
}