#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <variant>
#include <cstdlib>

#include "thermolang/compiler/IrGenerator.h"
#include "thermolang/optimizer/OptimizationManager.h"
#include "thermolang/optimizer/Passes.h"
#include "thermolang/parser/Parser.h"
#include "thermolang/semantics/SemanticAnalyzer.h"
#include "thermolang/semantics/TypeChecker.h"
#include "thermolang/optimizer/ConstantFoldingPass.h"
#include "thermolang/optimizer/EnergyFunctionOptimizer.h"
#include "thermolang/optimizer/CircuitTopologyOptimizer.h"
#include "thermolang/optimizer/VarianceTrackingPass.h"
#include "thermolang/parser/Parser.h"
#include "thermolang/semantics/SemanticAnalyzer.h"
#include "thermolang/optimizer/ThermalSchedulingPass.h"
#include "thermolang/optimizer/NoiseShapingPass.h"

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
    std::vector<std::unique_ptr<thermolang::ir::FunctionIR>> get_full_ir(const std::string &source)
    {
        thermolang::Lexer lexer(source);
        thermolang::Parser parser(lexer);
        auto ast = parser.parse();
        if (parser.had_error())
            return {};
        thermolang::SymbolTable symbols;
        thermolang::SemanticAnalyzer semantic_analyzer(symbols);
        if (!semantic_analyzer.analyze(ast))
            return {};
        thermolang::TypeChecker type_checker(symbols);
        if (!type_checker.check(ast))
            return {};
        thermolang::compiler::IrGenerator ir_gen;
        return ir_gen.generate(ast);
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

TEST_F(OptimizerTest, ThermalSchedulingPass)
{
    std::string source = R"(
        energy fn complex_energy(s1: float, s2: float, s3: float, s4: float) -> float {
            return s1+s2+s3+s4;
        }

        fn main() -> void {
            let initial_temp = 1.0;
            let cooling_rate = 0.9;
            let steps = 100;
            let result = thermal_anneal(complex_energy, initial_temp, cooling_rate, steps);
        }
    )";

    auto ir_program = get_full_ir(source);
    ASSERT_EQ(ir_program.size(), 2);

    auto *main_func = ir_program[1].get();
    ASSERT_EQ(main_func->name, "main");

    thermolang::optimizer::ThermalSchedulingPass pass;
    pass.set_program_ir(&ir_program);
    bool modified = pass.run(*main_func);

    ASSERT_TRUE(modified);

    // --- Robustly find the modified values by tracing registers from the call site ---
    std::string temp_reg_name;
    std::string steps_reg_name;

    // 1. Find the thermal_anneal call and get the register names for its arguments.
    for (const auto &instr : main_func->basic_blocks[0]->instructions)
    {
        if (auto *call = dynamic_cast<thermolang::ir::CallInstr *>(instr.get()))
        {
            if (call->callee_name == "thermal_anneal")
            {
                temp_reg_name = std::get<std::string>(call->args[1]);  // temp is the 2nd argument (index 1)
                steps_reg_name = std::get<std::string>(call->args[3]); // steps is the 4th argument (index 3)
                break;
            }
        }
    }

    ASSERT_FALSE(temp_reg_name.empty());
    ASSERT_FALSE(steps_reg_name.empty());

    double found_temp = 0.0;
    int64_t found_steps = 0;

    // 2. Now, find the LoadConstInstr that define *those specific registers*.
    for (const auto &instr : main_func->basic_blocks[0]->instructions)
    {
        if (auto *load = dynamic_cast<thermolang::ir::LoadConstInstr *>(instr.get()))
        {
            if (load->result_reg == temp_reg_name)
            {
                found_temp = std::get<double>(load->value);
            }
            if (load->result_reg == steps_reg_name)
            {
                found_steps = std::get<int64_t>(load->value);
            }
        }
    }

    // 3. Verify the results.
    double expected_temp = 2.5 * log(4); // approx 3.4657
    int64_t expected_steps = 4000;

    EXPECT_NEAR(found_temp, expected_temp, 0.001);
    EXPECT_EQ(found_steps, expected_steps);
}

TEST_F(OptimizerTest, NoiseShapingScalesAndEmitsLocalTemperatures)
{
    // Ensure variance shaping is enabled for this test regardless of external env
    setenv("NOISE_SHAPING_MODE", "degree+variance", 1);
    setenv("NOISE_SHAPING_VARIANCE_SHRINK", "0.5", 1);

    using namespace thermolang;
    using namespace thermolang::ir;

    auto func = std::make_unique<FunctionIR>();
    func->name = "energy";
    func->is_energy_function = true;
    func->basic_blocks.emplace_back(std::make_unique<BasicBlock>("entry"));
    auto &instrs = func->basic_blocks[0]->instructions;

    instrs.push_back(std::make_unique<VarianceTrackInstr>("s0", std::string("s0"), 1.0));
    instrs.push_back(std::make_unique<VarianceTrackInstr>("s1", std::string("s1"), 0.0));

    std::vector<Operand> spins = {std::string("s0"), std::string("s1")};
    std::vector<std::vector<double>> J = {{0.0, 1.0}, {1.0, 0.0}};
    std::vector<double> h = {1.0, 1.0};

    instrs.push_back(std::make_unique<DiscreteEBMInstr>("res", spins, J, h));

    optimizer::NoiseShapingPass pass;
    bool modified = pass.run(*func);
    EXPECT_TRUE(modified);

    auto *ebm = dynamic_cast<DiscreteEBMInstr *>(instrs.back().get());
    ASSERT_NE(ebm, nullptr);

    ASSERT_EQ(ebm->local_temperatures.size(), spins.size());
    EXPECT_NEAR(ebm->h_vector[0], 2.4, 1e-9);
    EXPECT_NEAR(ebm->h_vector[1], 1.6, 1e-9);

    double expected_coupling = std::sqrt(2.4 * 1.6);
    EXPECT_NEAR(ebm->J_matrix[0][1], expected_coupling, 1e-9);
    EXPECT_NEAR(ebm->J_matrix[1][0], expected_coupling, 1e-9);

    ASSERT_EQ(ebm->local_temperatures.size(), 2u);
    EXPECT_NEAR(ebm->local_temperatures[0], 0.41666666666666663, 1e-9);
    EXPECT_NEAR(ebm->local_temperatures[1], 0.625, 1e-9);
}