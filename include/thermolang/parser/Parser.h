#ifndef THERMOLANG_PARSER_H
#define THERMOLANG_PARSER_H

#include "thermolang/lexer/Lexer.h"
#include "thermolang/parser/Ast.h"
#include <vector>
#include <memory>
#include <stdexcept>

namespace thermolang
{

    class Parser
    {
    public:
        explicit Parser(Lexer &lexer);

        // The main entry point for the parser.
        std::vector<std::unique_ptr<Stmt>> parse();

        // Accessor for error state
        bool had_error() const;

        // Custom exception for parser errors
        class ParseError : public std::runtime_error
        {
        public:
            explicit ParseError(const char *message) : std::runtime_error(message) {}
        };

    private:
        // --- Parsing methods for different grammar rules ---
        std::unique_ptr<Stmt> declaration(); // Top-level rule
        std::unique_ptr<Stmt> function_declaration();
        std::unique_ptr<Stmt> stochastic_declaration();
        std::unique_ptr<Stmt> energy_declaration();
        std::unique_ptr<Stmt> type_declaration();
        std::unique_ptr<Stmt> statement();
        std::unique_ptr<Stmt> let_declaration();
        std::unique_ptr<Stmt> expression_statement();
        std::unique_ptr<Stmt> block_statement();
        std::unique_ptr<Stmt> return_statement();
        std::unique_ptr<Stmt> thermal_statement();
        std::unique_ptr<Stmt> parallel_statement();
        std::unique_ptr<Stmt> annotation_statement();
        std::unique_ptr<Stmt> function(const std::string &kind);
        std::unique_ptr<Stmt> annotation();

        // Expression-level rules
        std::unique_ptr<Expr> expression();
        std::unique_ptr<Expr> assignment();
        std::unique_ptr<Expr> term();
        std::unique_ptr<Expr> factor();
        std::unique_ptr<Expr> unary();
        std::unique_ptr<Expr> call();
        std::unique_ptr<Expr> primary();

        // Type expression rules
        std::unique_ptr<TypeExpr> type_expression();
        std::unique_ptr<TypeExpr> basic_type();
        std::unique_ptr<TypeExpr> distribution_type();
        std::unique_ptr<TypeExpr> function_type();
        std::unique_ptr<TypeExpr> energy_type();
        std::unique_ptr<TypeExpr> circuit_type();

        // Parameter parsing helper
        Parameter parse_parameter();
        std::vector<Parameter> parse_parameters();

        // Annotation parsing helper
        std::vector<AnnotationValue> parse_annotation_values();

        // --- Helper methods for token handling and error reporting ---
        // Consumes the current token if it matches one of the given types.
        bool match(const std::vector<TokenType> &types);

        // Checks the type of the current token without consuming it.
        bool check(TokenType type) const;

        // Consumes and returns the current token, advancing the stream.
        Token advance();

        // Checks if the current token is of the expected type and consumes it.
        // Reports an error if it's not the correct type.
        Token consume(TokenType type, const char *message);

        // Reports an error at the current token
        ParseError error(const char *message);

        // Synchronizes the parser state after an error
        void synchronize();

        // Peeking and state-checking methods
        Token peek() const;
        Token previous() const;
        bool is_at_end() const;

        // --- Parser State ---
        Lexer &lexer_;
        Token current_;
        Token previous_;
        bool had_error_ = false;
    };

} // namespace thermolang

#endif // THERMOLANG_PARSER_H