#include <gtest/gtest.h>
#include "thermolang/lexer/Lexer.h"
#include <vector>
#include "thermolang/parser/Parser.h"
#include "thermolang/semantics/SemanticAnalyzer.h"
#include "thermolang/semantics/TypeChecker.h" 

// Test fixture for the Lexer
class LexerTest : public ::testing::Test
{
protected:
    void check_tokenization(const std::string &source, const std::vector<thermolang::TokenType> &expected_types)
    {
        thermolang::Lexer lexer(source);
        for (const auto &expected_type : expected_types)
        {
            thermolang::Token token = lexer.next_token();
            EXPECT_EQ(token.get_type(), expected_type) << "Token lexeme: " << token.get_lexeme();
        }
    }
};

TEST_F(LexerTest, TokenizeLetStatement)
{
    std::string source = "let x = 10.5;";
    std::vector<thermolang::TokenType> expected = {
        thermolang::TokenType::LET, thermolang::TokenType::IDENTIFIER, thermolang::TokenType::EQUAL,
        thermolang::TokenType::FLOAT_LITERAL, thermolang::TokenType::SEMICOLON, thermolang::TokenType::END_OF_FILE};
    check_tokenization(source, expected);
}

// Test fixture for the Parser (Unchanged, for brevity)
class ParserTest : public ::testing::Test
{
};

TEST_F(ParserTest, ParseLetStatement)
{
    std::string source = "let my_var = 123;";
    thermolang::Lexer lexer(source);
    thermolang::Parser parser(lexer);
    auto statements = parser.parse();
    ASSERT_EQ(statements.size(), 1);
    auto *let_stmt = dynamic_cast<thermolang::LetStmt *>(statements[0].get());
    ASSERT_NE(let_stmt, nullptr);
    EXPECT_EQ(let_stmt->name.get_lexeme(), "my_var");
}

// This test fixture now runs the full analysis pipeline.
class AnalysisTest : public ::testing::Test
{
protected:
    // This helper function now runs both semantic analysis and type checking.
    // It returns true if BOTH passes succeed.
    bool analyze_and_check(const std::string &source)
    {
        thermolang::Lexer lexer(source);
        thermolang::Parser parser(lexer);
        auto ast = parser.parse();
        if (parser.had_error())
            return false;

        thermolang::SymbolTable symbols;
        thermolang::SemanticAnalyzer semantic_analyzer(symbols);
        if (!semantic_analyzer.analyze(ast))
            return false; // This can still catch re-declarations

        thermolang::TypeChecker type_checker(symbols);
        return type_checker.check(ast); // The final authority
    }
};

TEST_F(AnalysisTest, DetectsUndeclaredVariable)
{
    std::string source = "let x = y;"; // y is not declared
    // We expect the full analysis pipeline to fail
    EXPECT_FALSE(analyze_and_check(source));
}

TEST_F(AnalysisTest, AcceptsValidVariableUsage)
{
    std::string source = "let y = 10; let x = y;";
    // We expect the analysis to succeed
    EXPECT_TRUE(analyze_and_check(source));
}

TEST_F(AnalysisTest, FunctionAndBlockScoping)
{
    std::string source = R"(
        let x = 10;
        fn my_func() -> float {
            let x = 20;
            let y = x;
            return 0.0;
        }
    )";
    EXPECT_TRUE(analyze_and_check(source));
}

TEST_F(AnalysisTest, DetectsVariableOutOfScope)
{
    std::string source = R"(
        fn my_func() -> float {
            let y = 10;
            return 0.0;
        }
        let x = y; 
    )";
    EXPECT_FALSE(analyze_and_check(source));
}