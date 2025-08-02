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

// Test fixture for Optimizer Passes
class OptimizerTest : public ::testing::Test
{
protected:
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
    opt_manager.add_pass(std::make_unique<thermolang::optimizer::ConstantFoldingPass>());
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