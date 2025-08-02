#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <sstream>

#include "thermolang/lexer/Lexer.h"
#include "thermolang/parser/Parser.h"
#include "thermolang/semantics/SemanticAnalyzer.h"
#include "thermolang/semantics/TypeChecker.h"
#include "thermolang/compiler/IrGenerator.h"

// Test fixture for the IR Generator
class IrGeneratorTest : public ::testing::Test
{
protected:
    // Helper function to run the full pipeline and return the generated IR as a string.
    std::string generate_ir(const std::string &source)
    {
        // Run Frontend
        thermolang::Lexer lexer(source);
        thermolang::Parser parser(lexer);
        auto ast = parser.parse();
        if (parser.had_error())
        {
            return "Parser Error";
        }

        thermolang::SymbolTable symbols;
        thermolang::SemanticAnalyzer semantic_analyzer(symbols);
        if (!semantic_analyzer.analyze(ast))
        {
            return "Semantic Error";
        }

        thermolang::TypeChecker type_checker(symbols);
        if (!type_checker.check(ast))
        {
            return "Type Error";
        }

        // Run IR Generation
        thermolang::compiler::IrGenerator ir_gen;
        auto ir_functions = ir_gen.generate(ast);

        // Convert the IR to a string for easy comparison
        std::stringstream ss;
        for (const auto &func_ir : ir_functions)
        {
            ss << func_ir->toString();
        }
        return ss.str();
    }
};

TEST_F(IrGeneratorTest, SimpleFunctionWithLocals)
{
    std::string source = R"(
        fn calculate_energy(x: float) -> float {
            let base_energy = 10.5;
            let squared_x = x * x;
            return base_energy * squared_x;
        }
    )";

    std::string expected_ir =
        "function calculate_energy(r0) {\n"
        "entry:\n"
        "    r1 = load_const 10.5\n"
        "    r2 = mul r0, r0\n"
        "    r3 = mul r1, r2\n"
        "    return r3\n"
        "}\n";

    std::string actual_ir = generate_ir(source);
    EXPECT_EQ(actual_ir, expected_ir);
}

TEST_F(IrGeneratorTest, FunctionCall)
{
    std::string source = R"(
        fn add(a: int, b: int) -> int {
            return a + b;
        }

        fn main() -> int {
            let x = 10;
            let y = 20;
            let z = add(x, y);
            return z;
        }
    )";

    // Note: The order of function generation can vary based on std::vector behavior,
    // but the content of each function's IR should be consistent.
    // We will check for the presence of both.

    std::string expected_ir_add =
        "function add(r0, r1) {\n"
        "entry:\n"
        "    r2 = add r0, r1\n"
        "    return r2\n"
        "}\n";

    std::string expected_ir_main =
        "function main() {\n"
        "entry:\n"
        "    r3 = load_const 10\n"
        "    r4 = load_const 20\n"
        "    r5 = call add(r3, r4)\n"
        "    return r5\n"
        "}\n";

    std::string actual_ir = generate_ir(source);

    // Check that both functions were generated correctly, regardless of order.
    EXPECT_NE(actual_ir.find(expected_ir_add), std::string::npos);
    EXPECT_NE(actual_ir.find(expected_ir_main), std::string::npos);
}

TEST_F(IrGeneratorTest, StochasticFunctionIsGenerated)
{
    std::string source = R"(
        stochastic fn sample(mean: float) -> float {
            return mean;
        }
    )";

    std::string expected_ir =
        "function sample(r0) {\n"
        "entry:\n"
        "    return r0\n"
        "}\n";

    std::string actual_ir = generate_ir(source);
    EXPECT_EQ(actual_ir, expected_ir);
}