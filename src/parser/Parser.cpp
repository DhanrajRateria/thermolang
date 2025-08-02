#include "thermolang/parser/Parser.h"
#include <iostream>

namespace thermolang
{

    Parser::Parser(Lexer &lexer)
        : lexer_(lexer),
          current_(lexer_.next_token()),
          previous_(current_)
    {
    }

    std::vector<std::unique_ptr<Stmt>> Parser::parse()
    {
        std::vector<std::unique_ptr<Stmt>> statements;
        while (!is_at_end())
        {
            try
            {
                auto stmt = declaration();
                if (stmt)
                {
                    statements.push_back(std::move(stmt));
                }
            }
            catch (const ParseError &error)
            {
                had_error_ = true;
                synchronize();
            }
        }
        return statements;
    }

    std::unique_ptr<Stmt> Parser::declaration()
    {
        try
        {
            if (match({TokenType::LET}))
            {
                return let_declaration();
            }
            else if (match({TokenType::FN}))
            {
                return function("function");
            }
            else if (match({TokenType::STOCHASTIC}))
            {
                // Make sure we consume the "fn" token after "stochastic"
                Token stochastic_token = previous(); // Store the 'stochastic' token

                if (match({TokenType::FN}))
                {
                    // Pass a flag indicating this is a stochastic function
                    auto fn = function("stochastic function");
                    if (fn)
                    {
                        return std::make_unique<StochasticStmt>(
                            stochastic_token, // Pass the token
                            std::unique_ptr<FunctionStmt>(static_cast<FunctionStmt *>(fn.release())));
                    }
                }
                else
                {
                    throw error("Expected 'fn' after 'stochastic'.");
                }
            }
            else if (match({TokenType::ENERGY}))
            {
                // Make sure we consume the "fn" token after "energy"
                Token energy_token = previous(); // Store the 'energy' token

                if (match({TokenType::FN}))
                {
                    // Pass a flag indicating this is an energy function
                    auto fn = function("energy function");
                    if (fn)
                    {
                        return std::make_unique<EnergyStmt>(
                            energy_token, // Pass the token
                            std::unique_ptr<FunctionStmt>(static_cast<FunctionStmt *>(fn.release())));
                    }
                }
                else
                {
                    throw error("Expected 'fn' after 'energy'.");
                }
            }
            else if (match({TokenType::TYPE}))
            {
                return type_declaration();
            }
            else if (check(TokenType::AT))
            {
                return annotation();
            }

            return statement();
        }
        catch (const ParseError &e)
        {
            synchronize();
            return nullptr;
        }
    }

    // Add this method to fix the "function" undefined identifier issue
    std::unique_ptr<Stmt> Parser::function(const std::string &kind)
    {
        // Simply delegate to the existing function_declaration method
        return function_declaration();
    }

    // Add this method to fix the "annotation" undefined identifier issue
    std::unique_ptr<Stmt> Parser::annotation()
    {
        // Delegate to the existing annotation_statement method
        return annotation_statement();
    }

    std::unique_ptr<Stmt> Parser::function_declaration()
    {
        try
        {
            Token name = consume(TokenType::IDENTIFIER, "Expect function name.");
            consume(TokenType::LPAREN, "Expect '(' after function name.");

            // Parse parameters
            std::vector<Parameter> parameters;
            if (!check(TokenType::RPAREN))
            {
                do
                {
                    parameters.push_back(parse_parameter());
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expect ')' after parameters.");

            // Parse return type
            consume(TokenType::ARROW, "Expect '->' for return type.");
            auto return_type = type_expression();

            // Handle the block
            consume(TokenType::LBRACE, "Expect '{' before function body.");

            std::vector<std::unique_ptr<Stmt>> statements;
            while (!check(TokenType::RBRACE) && !is_at_end())
            {
                auto stmt = declaration();
                if (stmt)
                    statements.push_back(std::move(stmt));
            }

            consume(TokenType::RBRACE, "Expect '}' to end function body.");

            auto body = std::make_unique<BlockStmt>(std::move(statements));
            return std::make_unique<FunctionStmt>(name, std::move(parameters), std::move(return_type), std::move(body));
        }
        catch (const ParseError &error)
        {
            // Let the error propagate but ensure it's properly tracked
            had_error_ = true;
            throw;
        }
    }

    Parameter Parser::parse_parameter()
    {
        Token name = consume(TokenType::IDENTIFIER, "Expect parameter name.");
        consume(TokenType::COLON, "Expect ':' after parameter name.");
        auto type = type_expression();
        return Parameter(name, std::move(type));
    }

    std::vector<Parameter> Parser::parse_parameters()
    {
        std::vector<Parameter> parameters;
        if (!check(TokenType::RPAREN))
        {
            do
            {
                parameters.push_back(parse_parameter());
            } while (match({TokenType::COMMA}));
        }
        return parameters;
    }

    std::unique_ptr<Stmt> Parser::stochastic_declaration()
    {
        Token keyword = previous();
        auto function = function_declaration();
        auto *func_stmt = dynamic_cast<FunctionStmt *>(function.get());
        if (!func_stmt)
        {
            throw error("Expected function declaration after 'stochastic' keyword.");
        }
        return std::make_unique<StochasticStmt>(keyword, std::unique_ptr<FunctionStmt>(func_stmt));
    }

    std::unique_ptr<Stmt> Parser::energy_declaration()
    {
        Token keyword = previous();
        auto function = function_declaration();
        auto *func_stmt = dynamic_cast<FunctionStmt *>(function.get());
        if (!func_stmt)
        {
            throw error("Expected function declaration after 'energy' keyword.");
        }
        return std::make_unique<EnergyStmt>(keyword, std::unique_ptr<FunctionStmt>(func_stmt));
    }

    std::unique_ptr<Stmt> Parser::thermal_statement()
    {
        Token keyword = previous();

        // Make sure we have an opening brace
        consume(TokenType::LBRACE, "Expect '{' after 'thermal' keyword.");

        try
        {
            // Parse statements within the thermal block
            std::vector<std::unique_ptr<Stmt>> statements;
            while (!check(TokenType::RBRACE) && !is_at_end())
            {
                auto stmt = declaration();
                if (stmt)
                    statements.push_back(std::move(stmt));
            }

            // Make sure we have a closing brace
            consume(TokenType::RBRACE, "Expect '}' to end thermal block.");

            // Create and return the thermal statement with its own block
            return std::make_unique<ThermalStmt>(
                keyword,
                std::make_unique<BlockStmt>(std::move(statements)));
        }
        catch (const ParseError &error)
        {
            // Improve error handling to prevent segfaults
            had_error_ = true;
            synchronize();

            // Return an empty thermal statement to allow parsing to continue
            return std::make_unique<ThermalStmt>(
                keyword,
                std::make_unique<BlockStmt>(std::vector<std::unique_ptr<Stmt>>()));
        }
    }

    std::unique_ptr<Stmt> Parser::parallel_statement()
    {
        Token keyword = previous();

        // Make sure we have an opening brace
        consume(TokenType::LBRACE, "Expect '{' after 'parallel' keyword.");

        try
        {
            // Parse statements within the parallel block
            std::vector<std::unique_ptr<Stmt>> statements;
            while (!check(TokenType::RBRACE) && !is_at_end())
            {
                auto stmt = declaration();
                if (stmt)
                    statements.push_back(std::move(stmt));
            }

            // Make sure we have a closing brace
            consume(TokenType::RBRACE, "Expect '}' to end parallel block.");

            // Create and return the parallel statement with its own block
            return std::make_unique<ParallelStmt>(
                keyword,
                std::make_unique<BlockStmt>(std::move(statements)));
        }
        catch (const ParseError &error)
        {
            // Improve error handling to prevent segfaults
            had_error_ = true;
            synchronize();

            // Return an empty parallel statement to allow parsing to continue
            return std::make_unique<ParallelStmt>(
                keyword,
                std::make_unique<BlockStmt>(std::vector<std::unique_ptr<Stmt>>()));
        }
    }

    std::unique_ptr<Stmt> Parser::type_declaration()
    {
        Token keyword = previous(); // Get the 'type' token
        Token name = consume(TokenType::IDENTIFIER, "Expect type name.");

        consume(TokenType::EQUAL, "Expect '=' after type name.");

        std::unique_ptr<TypeExpr> type_expr = type_expression();

        consume(TokenType::SEMICOLON, "Expect ';' after type declaration.");

        return std::make_unique<TypeStmt>(keyword, name, std::move(type_expr));
    }

    std::vector<AnnotationValue> Parser::parse_annotation_values()
    {
        std::vector<AnnotationValue> values;
        consume(TokenType::LPAREN, "Expect '(' after annotation name.");
        if (!check(TokenType::RPAREN))
        {
            do
            {
                Token name = consume(TokenType::IDENTIFIER, "Expect parameter name.");
                consume(TokenType::EQUAL, "Expect '=' after parameter name.");
                Token value = current_;
                if (check(TokenType::INTEGER_LITERAL) || check(TokenType::FLOAT_LITERAL) ||
                    check(TokenType::STRING_LITERAL) || check(TokenType::BOOL_LITERAL))
                {
                    value = advance();
                }
                else
                {
                    throw error("Expect literal value for annotation parameter.");
                }
                values.emplace_back(name, value);
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RPAREN, "Expect ')' after annotation parameters.");
        return values;
    }

    std::unique_ptr<Stmt> Parser::annotation_statement()
    {
        Token at = previous();
        Token name = consume(TokenType::IDENTIFIER, "Expect annotation name.");
        std::vector<AnnotationValue> values;

        if (check(TokenType::LPAREN))
        {
            values = parse_annotation_values();
        }

        auto stmt = declaration(); // Parse the statement that follows the annotation
        if (!stmt)
        {
            throw error("Expect statement after annotation.");
        }

        return std::make_unique<AnnotationStmt>(at, name, std::move(values), std::move(stmt));
    }

    std::unique_ptr<Stmt> Parser::let_declaration()
    {
        Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");

        std::unique_ptr<TypeExpr> type_ann = nullptr;
        if (match({TokenType::COLON}))
        {
            type_ann = type_expression();
        }

        std::unique_ptr<Expr> initializer = nullptr;
        if (match({TokenType::EQUAL}))
        {
            initializer = expression();
        }

        consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
        return std::make_unique<LetStmt>(name, std::move(type_ann), std::move(initializer));
    }

    std::unique_ptr<TypeExpr> Parser::type_expression()
    {
        if (match({TokenType::IDENTIFIER}))
        {
            return std::make_unique<NameTypeExpr>(previous());
        }
        else if (match({TokenType::DISTRIBUTION}))
        {
            consume(TokenType::LESS, "Expect '<' after 'distribution'.");
            auto element_type = type_expression();
            std::optional<Token> variance;

            if (match({TokenType::COMMA}))
            {
                Token keyword = consume(TokenType::IDENTIFIER, "Expect 'variance' keyword in distribution type.");
                if (keyword.get_lexeme() != "variance")
                {
                    throw error("Expect 'variance' keyword.");
                }

                consume(TokenType::EQUAL, "Expect '=' after 'variance'.");

                if (match({TokenType::FLOAT_LITERAL, TokenType::INTEGER_LITERAL}))
                {
                    variance = previous();
                }
                else
                {
                    throw error("Expect numeric literal for variance value.");
                }
            }

            consume(TokenType::GREATER, "Expect '>' to close distribution type.");
            return std::make_unique<DistributionTypeExpr>(std::move(element_type), variance);
        }
        else if (match({TokenType::FUNCTION}))
        {
            consume(TokenType::LESS, "Expect '<' after 'function'.");
            std::vector<std::unique_ptr<TypeExpr>> param_types;

            if (!check(TokenType::ARROW) && !check(TokenType::GREATER))
            {
                do
                {
                    param_types.push_back(type_expression());
                } while (match({TokenType::COMMA}) && !check(TokenType::ARROW));
            }

            consume(TokenType::ARROW, "Expect '->' in function type.");
            auto return_type = type_expression();
            consume(TokenType::GREATER, "Expect '>' to close function type.");

            return std::make_unique<FunctionTypeExpr>(std::move(param_types), std::move(return_type));
        }
        else if (match({TokenType::ENERGY}))
        {
            consume(TokenType::LESS, "Expect '<' after 'energy'.");
            std::vector<std::unique_ptr<TypeExpr>> var_types;

            if (!check(TokenType::GREATER))
            {
                do
                {
                    var_types.push_back(type_expression());
                } while (match({TokenType::COMMA}));
            }

            consume(TokenType::GREATER, "Expect '>' to close energy type.");
            return std::make_unique<EnergyTypeExpr>(std::move(var_types));
        }
        else if (match({TokenType::CIRCUIT}))
        {
            // Check for the circuit keyword and parse circuit type
            consume(TokenType::LESS, "Expect '<' after 'circuit'.");

            // Expected format: circuit<nodes=8, coupling="full">
            consume(TokenType::IDENTIFIER, "Expect 'nodes' in circuit type.");
            consume(TokenType::EQUAL, "Expect '=' after 'nodes'.");
            Token nodes = consume(TokenType::INTEGER_LITERAL, "Expect node count.");

            consume(TokenType::COMMA, "Expect ',' after node count.");
            consume(TokenType::IDENTIFIER, "Expect 'coupling' in circuit type.");
            consume(TokenType::EQUAL, "Expect '=' after 'coupling'.");
            Token couplings = consume(TokenType::STRING_LITERAL, "Expect coupling type.");

            consume(TokenType::GREATER, "Expect '>' to close circuit type.");

            return std::make_unique<CircuitTypeExpr>(nodes, couplings);
        }

        throw error("Expect type expression.");
    }

    std::unique_ptr<TypeExpr> Parser::basic_type()
    {
        Token name = consume(TokenType::IDENTIFIER, "Expect type name.");
        return std::make_unique<NameTypeExpr>(name);
    }

    std::unique_ptr<TypeExpr> Parser::distribution_type()
    {
        consume(TokenType::LESS, "Expect '<' after 'distribution'.");
        auto element_type = type_expression();

        std::optional<Token> variance;
        if (match({TokenType::COMMA}))
        {
            consume(TokenType::IDENTIFIER, "Expect 'variance'.");
            consume(TokenType::EQUAL, "Expect '=' after 'variance'.");
            variance = consume(TokenType::FLOAT_LITERAL, "Expect float literal for variance.");
        }

        consume(TokenType::GREATER, "Expect '>' to close distribution type.");
        return std::make_unique<DistributionTypeExpr>(std::move(element_type), variance);
    }

    std::unique_ptr<TypeExpr> Parser::function_type()
    {
        consume(TokenType::LESS, "Expect '<' after 'function'.");

        // Parse parameter types
        std::vector<std::unique_ptr<TypeExpr>> param_types;
        if (!check(TokenType::ARROW))
        {
            do
            {
                param_types.push_back(type_expression());
            } while (match({TokenType::COMMA}) && !check(TokenType::ARROW));
        }

        consume(TokenType::ARROW, "Expect '->' in function type.");
        auto return_type = type_expression();

        consume(TokenType::GREATER, "Expect '>' to close function type.");
        return std::make_unique<FunctionTypeExpr>(std::move(param_types), std::move(return_type));
    }

    std::unique_ptr<TypeExpr> Parser::energy_type()
    {
        consume(TokenType::LESS, "Expect '<' after 'energy'.");

        std::vector<std::unique_ptr<TypeExpr>> var_types;
        if (!check(TokenType::GREATER))
        {
            do
            {
                var_types.push_back(type_expression());
            } while (match({TokenType::COMMA}));
        }

        consume(TokenType::GREATER, "Expect '>' to close energy type.");
        return std::make_unique<EnergyTypeExpr>(std::move(var_types));
    }

    std::unique_ptr<TypeExpr> Parser::circuit_type()
    {
        consume(TokenType::LESS, "Expect '<' after 'circuit'.");

        consume(TokenType::IDENTIFIER, "Expect 'nodes'.");
        consume(TokenType::EQUAL, "Expect '=' after 'nodes'.");
        Token nodes = consume(TokenType::INTEGER_LITERAL, "Expect integer literal for node count.");

        consume(TokenType::COMMA, "Expect ',' after nodes specification.");

        consume(TokenType::IDENTIFIER, "Expect 'couplings'.");
        consume(TokenType::EQUAL, "Expect '=' after 'couplings'.");
        Token couplings = consume(TokenType::IDENTIFIER, "Expect coupling type.");

        consume(TokenType::GREATER, "Expect '>' to close circuit type.");
        return std::make_unique<CircuitTypeExpr>(nodes, couplings);
    }

    std::unique_ptr<Stmt> Parser::statement()
    {
        if (match({TokenType::LBRACE}))
            return block_statement();
        if (match({TokenType::RETURN}))
            return return_statement();
        if (match({TokenType::THERMAL}))
            return thermal_statement();
        if (match({TokenType::PARALLEL}))
            return parallel_statement();
        // In the future, add if_statement, while_statement, etc. here
        return expression_statement();
    }

    std::unique_ptr<Stmt> Parser::return_statement()
    {
        Token keyword = previous();
        std::unique_ptr<Expr> value = nullptr;

        if (!check(TokenType::SEMICOLON))
        {
            value = expression();
        }

        consume(TokenType::SEMICOLON, "Expect ';' after return value.");
        return std::make_unique<ReturnStmt>(keyword, std::move(value));
    }

    std::unique_ptr<Stmt> Parser::block_statement()
    {
        try
        {
            std::vector<std::unique_ptr<Stmt>> statements;
            while (!check(TokenType::RBRACE) && !is_at_end())
            {
                auto stmt = declaration();
                if (stmt)
                    statements.push_back(std::move(stmt));
            }

            Token closeBrace = consume(TokenType::RBRACE, "Expect '}' to end a block.");
            return std::make_unique<BlockStmt>(std::move(statements));
        }
        catch (const ParseError &error)
        {
            // Let the error propagate but ensure it's properly tracked
            had_error_ = true;
            throw;
        }
    }

    std::unique_ptr<Stmt> Parser::expression_statement()
    {
        auto expr = expression();
        consume(TokenType::SEMICOLON, "Expect ';' after expression.");
        return std::make_unique<ExprStmt>(std::move(expr));
    }

    // --- Expression Parsing with Precedence ---
    std::unique_ptr<Expr> Parser::expression() { return assignment(); }

    std::unique_ptr<Expr> Parser::assignment()
    {
        auto expr = term();

        if (match({TokenType::EQUAL}))
        {
            Token equals = previous();
            auto value = assignment();

            if (auto *var_expr = dynamic_cast<VariableExpr *>(expr.get()))
            {
                Token name = var_expr->name;
                return std::make_unique<BinaryExpr>(
                    std::make_unique<VariableExpr>(name),
                    equals,
                    std::move(value));
            }

            error("Invalid assignment target.");
        }

        return expr;
    }

    std::unique_ptr<Expr> Parser::term()
    {
        auto expr = factor();
        while (match({TokenType::MINUS, TokenType::PLUS}))
        {
            Token op = previous();
            auto right = factor();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::factor()
    {
        auto expr = unary();
        while (match({TokenType::SLASH, TokenType::STAR}))
        {
            Token op = previous();
            auto right = unary();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::unary()
    {
        if (match({TokenType::BANG, TokenType::MINUS}))
        {
            Token op = previous();
            auto right = unary();
            return std::make_unique<UnaryExpr>(op, std::move(right));
        }
        return call();
    }

    std::unique_ptr<Expr> Parser::call()
    {
        auto expr = primary();

        while (true)
        {
            if (match({TokenType::LPAREN}))
            {
                // It's a call expression
                std::vector<std::unique_ptr<Expr>> arguments;
                if (!check(TokenType::RPAREN))
                {
                    do
                    {
                        arguments.push_back(expression());
                    } while (match({TokenType::COMMA}));
                }
                Token paren = consume(TokenType::RPAREN, "Expect ')' after arguments.");
                expr = std::make_unique<CallExpr>(std::move(expr), paren, std::move(arguments));
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::primary()
    {
        if (match({TokenType::BOOL_LITERAL, TokenType::FLOAT_LITERAL, TokenType::INTEGER_LITERAL, TokenType::STRING_LITERAL}))
        {
            return std::make_unique<LiteralExpr>(previous());
        }

        if (match({TokenType::IDENTIFIER}))
        {
            return std::make_unique<VariableExpr>(previous());
        }

        if (match({TokenType::LPAREN}))
        {
            auto expr = expression();
            consume(TokenType::RPAREN, "Expect ')' after expression.");
            return expr;
        }

        throw error("Expect expression.");
    }

    // --- Helper Methods ---
    Token Parser::consume(TokenType type, const char *message)
    {
        if (check(type))
            return advance();
        throw error(message);
    }

    Parser::ParseError Parser::error(const char *message)
    {
        std::cerr << "Parser Error: " << message << std::endl;
        had_error_ = true;
        return ParseError(message);
    }

    void Parser::synchronize()
    {
        advance();

        while (!is_at_end())
        {
            if (previous().get_type() == TokenType::SEMICOLON)
                return;

            switch (current_.get_type())
            {
            case TokenType::FN:
            case TokenType::LET:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
            case TokenType::TYPE:
            case TokenType::STOCHASTIC:
            case TokenType::ENERGY:
            case TokenType::THERMAL:
            case TokenType::PARALLEL:
                return;
            default:
                break;
            }

            advance();
        }
    }

    Token Parser::advance()
    {
        if (!is_at_end())
            previous_ = current_;
        current_ = lexer_.next_token();
        return previous_;
    }

    bool Parser::match(const std::vector<TokenType> &types)
    {
        for (TokenType type : types)
        {
            if (check(type))
            {
                advance();
                return true;
            }
        }
        return false;
    }

    bool Parser::check(TokenType type) const { return peek().get_type() == type; }
    Token Parser::peek() const { return current_; }
    Token Parser::previous() const { return previous_; }
    bool Parser::is_at_end() const { return peek().get_type() == TokenType::END_OF_FILE; }

    // Accessor for error state
    bool Parser::had_error() const { return had_error_; }

} // namespace thermolang