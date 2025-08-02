#include <gtest/gtest.h>
#include "thermolang/lexer/Lexer.h"
#include <vector>
#include "thermolang/parser/AstPrinter.h"
#include "thermolang/parser/Parser.h"
#include "thermolang/semantics/SemanticAnalyzer.h"

// Test fixture for the Lexer
class LexerTest : public ::testing::Test {
protected:
    void check_tokenization(const std::string& source, const std::vector<thermolang::TokenType>& expected_types) {
        thermolang::Lexer lexer(source);
        for (const auto& expected_type : expected_types) {
            thermolang::Token token = lexer.next_token();
            EXPECT_EQ(token.get_type(), expected_type) << "Token lexeme: " << token.get_lexeme();
        }
    }
};

TEST_F(LexerTest, TokenizeLetStatement) {
    std::string source = "let x = 10.5;";
    std::vector<thermolang::TokenType> expected = {
        thermolang::TokenType::LET,
        thermolang::TokenType::IDENTIFIER,
        thermolang::TokenType::EQUAL,
        thermolang::TokenType::FLOAT_LITERAL,
        thermolang::TokenType::SEMICOLON,
        thermolang::TokenType::END_OF_FILE
    };
    check_tokenization(source, expected);
}

TEST_F(LexerTest, TokenizeFunctionDeclaration) {
    std::string source = "fn calculate_energy() -> float {}";
    std::vector<thermolang::TokenType> expected = {
        thermolang::TokenType::FN,
        thermolang::TokenType::IDENTIFIER,
        thermolang::TokenType::LPAREN,
        thermolang::TokenType::RPAREN,
        thermolang::TokenType::ARROW,
        thermolang::TokenType::IDENTIFIER, // "float" is an identifier at this stage
        thermolang::TokenType::LBRACE,
        thermolang::TokenType::RBRACE,
        thermolang::TokenType::END_OF_FILE
    };
    check_tokenization(source, expected);
}

// Test fixture for the Parser
class ParserTest : public ::testing::Test {};

TEST_F(ParserTest, ParseLetStatement) {
    std::string source = "let my_var = 123;";
    thermolang::Lexer lexer(source);
    thermolang::Parser parser(lexer);

    auto statements = parser.parse();

    // We expect one statement
    ASSERT_EQ(statements.size(), 1);

    // Check if it's a LetStmt
    auto* let_stmt = dynamic_cast<thermolang::LetStmt*>(statements[0].get());
    ASSERT_NE(let_stmt, nullptr);

    // Check the variable name
    EXPECT_EQ(let_stmt->name.get_lexeme(), "my_var");
}

TEST_F(ParserTest, ParseExpressionWithPrecedence) {
    std::string source = "let x = -2 * (5 + 10);";
    thermolang::Lexer lexer(source);
    thermolang::Parser parser(lexer);
    auto statements = parser.parse();

    ASSERT_EQ(statements.size(), 1);
    auto* let_stmt = dynamic_cast<thermolang::LetStmt*>(statements[0].get());
    ASSERT_NE(let_stmt, nullptr);
    ASSERT_NE(let_stmt->initializer, nullptr);

    // Verify the structure of the AST using the printer
    thermolang::AstPrinter printer;
    std::string ast_string = printer.print(*let_stmt->initializer);

    // Expected output shows '*' is higher precedence than '+'
    EXPECT_NE(ast_string.find("(*"), std::string::npos);
    EXPECT_NE(ast_string.find("(+ 5 10)"), std::string::npos);
    EXPECT_NE(ast_string.find("(- 2)"), std::string::npos);
}

TEST_F(ParserTest, ParseFunction) {
    std::string source = "fn my_func() -> float { let x = 10; }";
    thermolang::Lexer lexer(source);
    thermolang::Parser parser(lexer);

    auto statements = parser.parse();
    ASSERT_EQ(statements.size(), 1);
    
    auto* func_stmt = dynamic_cast<thermolang::FunctionStmt*>(statements[0].get());
    ASSERT_NE(func_stmt, nullptr);
    EXPECT_EQ(func_stmt->name.get_lexeme(), "my_func");
    ASSERT_NE(func_stmt->body, nullptr);
    EXPECT_EQ(func_stmt->body->statements.size(), 1);
}

class SemanticsTest : public ::testing::Test {};

TEST_F(SemanticsTest, DetectsUndeclaredVariable) {
    std::string source = "let x = y;"; // y is not declared
    thermolang::Lexer lexer(source);
    thermolang::Parser parser(lexer);
    auto ast = parser.parse();
    thermolang::SemanticAnalyzer analyzer;
    
    // We expect the analysis to fail
    EXPECT_FALSE(analyzer.analyze(ast));
}

TEST_F(SemanticsTest, AcceptsValidVariableUsage) {
    std::string source = "let y = 10; let x = y;";
    thermolang::Lexer lexer(source);
    thermolang::Parser parser(lexer);
    auto ast = parser.parse();
    thermolang::SemanticAnalyzer analyzer;

    // We expect the analysis to succeed
    EXPECT_TRUE(analyzer.analyze(ast));
}

TEST_F(SemanticsTest, FunctionAndBlockScoping) {
    // This program should be valid. The inner 'x' shadows the outer one.
    std::string source = R"(
        let x = 10;
        fn my_func() -> float {
            let x = 20;
            let y = x; // Should resolve to 20
        }
    )";
    thermolang::Lexer lexer(source);
    thermolang::Parser parser(lexer);
    auto ast = parser.parse();
    thermolang::SemanticAnalyzer analyzer;
    
    EXPECT_TRUE(analyzer.analyze(ast));
}

TEST_F(SemanticsTest, DetectsVariableOutOfScope) {
    // 'y' is defined inside a block, but used outside. This is an error.
    std::string source = R"(
        fn my_func() -> float {
            let y = 10;
        }
        let x = y; 
    )";
    thermolang::Lexer lexer(source);
    thermolang::Parser parser(lexer);
    auto ast = parser.parse();
    thermolang::SemanticAnalyzer analyzer;

    EXPECT_FALSE(analyzer.analyze(ast));
}