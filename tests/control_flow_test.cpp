#include <gtest/gtest.h>
#include <string>
#include "thermolang/lexer/Lexer.h"
#include "thermolang/parser/Parser.h"
#include "thermolang/semantics/SemanticAnalyzer.h"
#include "thermolang/semantics/TypeChecker.h"
#include "thermolang/compiler/IrGenerator.h"

class ControlFlowTest : public ::testing::Test
{
protected:
    std::string generate_ir(const std::string &source)
    {
        thermolang::Lexer lexer(source);
        thermolang::Parser parser(lexer);
        auto ast = parser.parse();
        if (parser.had_error())
        {
            std::cerr << "Parser error while processing: " << source << std::endl;
            return "Parser Error";
        }

        thermolang::SymbolTable symbols;
        thermolang::SemanticAnalyzer semantic_analyzer(symbols);
        if (!semantic_analyzer.analyze(ast))
            return "Semantic Error";

        thermolang::TypeChecker type_checker(symbols);
        if (!type_checker.check(ast))
            return "Type Error";

        thermolang::compiler::IrGenerator ir_gen;
        auto ir_functions = ir_gen.generate(ast);

        std::stringstream ss;
        for (const auto &func : ir_functions)
        {
            ss << func->toString();
        }
        return ss.str();
    }
};

TEST_F(ControlFlowTest, IfElseIRGeneration)
{
    std::string source = R"(
        fn decide(condition: bool) -> int {
            if (condition) {
                return 1;
            } else {
                return 0;
            }
        }
    )";
    std::string ir = generate_ir(source);

    // Check for key components of a CFG
    EXPECT_NE(ir.find("branch"), std::string::npos) << "IR should contain a 'branch' instruction.";
    EXPECT_NE(ir.find("jump"), std::string::npos) << "IR should contain 'jump' instructions.";
    EXPECT_NE(ir.find("L0:"), std::string::npos) << "IR should contain basic block labels (e.g., L0).";
    EXPECT_NE(ir.find("L1:"), std::string::npos) << "IR should contain basic block labels (e.g., L1).";
    EXPECT_NE(ir.find("L2:"), std::string::npos) << "IR should contain basic block labels (e.g., L2).";
}

TEST_F(ControlFlowTest, WhileLoopIRGeneration)
{
    std::string source = R"(
        fn loop_n_times(count: int) -> int {
            let i = 0;
            while (i < count) {
                i = i + 1;
            }
            return i;
        }
    )";
    std::string ir = generate_ir(source);

    // Check for the characteristic loop structure in the IR
    EXPECT_NE(ir.find("L_cond:"), std::string::npos) << "While loop IR should have a condition label.";
    EXPECT_NE(ir.find("L_body:"), std::string::npos) << "While loop IR should have a body label.";
    EXPECT_NE(ir.find("L_exit:"), std::string::npos) << "While loop IR should have an exit label.";
    EXPECT_NE(ir.find("branch"), std::string::npos) << "While loop IR should branch on a condition.";
    // Check for the jump back to the condition to form the loop
    EXPECT_NE(ir.find("jump L_cond"), std::string::npos) << "While loop IR should jump back to the condition.";
}