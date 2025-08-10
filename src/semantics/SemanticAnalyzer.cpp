#include "thermolang/semantics/SemanticAnalyzer.h"
#include <iostream>

namespace thermolang
{

    // Default constructor - create our own symbol table
    SemanticAnalyzer::SemanticAnalyzer()
        : owned_symbols_(std::make_unique<SymbolTable>()),
          symbols_(*owned_symbols_),
          had_error_(false) {}

    // Constructor that accepts an external symbol table
    SemanticAnalyzer::SemanticAnalyzer(SymbolTable &symbols)
        : symbols_(symbols),
          had_error_(false) {}

    bool SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<Stmt>> &statements)
    {
        had_error_ = false; // Reset error state

        // Create a global scope
        // Note: No need to enter_scope() as the symbol table is initialized with a global scope

        // Analyze all statements
        for (const auto &statement : statements)
        {
            if (statement)
            { // Add null check here
                analyze(*statement);
            }
        }

        return !had_error_;
    }

    // --- Statement Visitors ---
    void SemanticAnalyzer::visit(const LetStmt &stmt)
    {
        // 1. Analyze the initializer first (if it exists).
        if (stmt.initializer)
        {
            analyze(*stmt.initializer);
        }

        // 2. Then, define the variable in the current scope.
        // Note: Type will be set later by the TypeChecker
        if (!symbols_.define(stmt.name.get_lexeme()))
        {
            std::cerr << "Semantic Error: Variable '" << stmt.name.get_lexeme()
                      << "' already declared in this scope." << std::endl;
            had_error_ = true;
        }
    }

    void SemanticAnalyzer::visit(const FunctionStmt &stmt)
    {
        // Define the function in the current (enclosing) scope.
        // Note: We just register it as defined here; the TypeChecker will add type info
        // if (!symbols_.define(stmt.name.get_lexeme(), nullptr, false, true))
        // {
        //     std::cerr << "Semantic Error: Function '" << stmt.name.get_lexeme()
        //               << "' already declared in this scope." << std::endl;
        //     had_error_ = true;
        //     return;
        // }
        if (symbols_.is_defined_in_current_scope(stmt.name.get_lexeme()))
        {
            std::cerr << "Semantic Error: Identifier '" << stmt.name.get_lexeme()
                      << "' is already defined in this scope." << std::endl;
            had_error_ = true;
            // Do not proceed with analyzing this function if its name is invalid.
            return;
        }

        // Create a new scope for the function body.
        symbols_.enter_scope();

        // Define all parameters in the new scope.
        for (const auto &param : stmt.params)
        {
            if (!symbols_.define(param.name.get_lexeme()))
            {
                std::cerr << "Semantic Error: Parameter '" << param.name.get_lexeme()
                          << "' already declared in function." << std::endl;
                had_error_ = true;
            }
        }

        // Analyze the body within the new scope.
        if (stmt.body)
        {
            analyze(*stmt.body);
        }

        // Exit the function's scope.
        symbols_.exit_scope();
    }

    void SemanticAnalyzer::visit(const IfStmt &stmt)
    {
        // Analyze the condition expression
        stmt.condition->accept(*this);

        // Analyze the then branch
        stmt.then_branch->accept(*this);

        // If there's an else branch, analyze that too
        if (stmt.else_branch)
        {
            stmt.else_branch->accept(*this);
        }
    }

    void SemanticAnalyzer::visit(const WhileStmt &stmt)
    {
        // Analyze the condition expression
        stmt.condition->accept(*this);

        // Analyze the body
        stmt.body->accept(*this);
    }

    void SemanticAnalyzer::visit(const BlockStmt &stmt)
    {
        symbols_.enter_scope();
        for (const auto &statement : stmt.statements)
        {
            analyze(*statement);
        }
        symbols_.exit_scope();
    }

    void SemanticAnalyzer::visit(const ExprStmt &stmt)
    {
        if (stmt.expression)
        {
            analyze(*stmt.expression);
        }
    }

    void SemanticAnalyzer::visit(const ReturnStmt &stmt)
    {
        if (stmt.value)
        {
            analyze(*stmt.value);
        }
    }

    void SemanticAnalyzer::visit(const StochasticStmt &stmt)
    {
        // Analyze the function
        analyze(*stmt.function);
    }

    void SemanticAnalyzer::visit(const EnergyStmt &stmt)
    {
        // Analyze the function
        analyze(*stmt.function);
    }

    void SemanticAnalyzer::visit(const ThermalStmt &stmt)
    {
        // Analyze the block
        analyze(*stmt.block);
    }

    void SemanticAnalyzer::visit(const ParallelStmt &stmt)
    {
        // Analyze the block
        analyze(*stmt.block);
    }

    void SemanticAnalyzer::visit(const TypeStmt &stmt)
    {
        // Define the type name in the current scope
        // Type resolution happens in the TypeChecker
        if (!symbols_.define(stmt.name.get_lexeme()))
        {
            std::cerr << "Semantic Error: Type '" << stmt.name.get_lexeme()
                      << "' already declared in this scope." << std::endl;
            had_error_ = true;
        }
    }

    void SemanticAnalyzer::visit(const AnnotationStmt &stmt)
    {
        // Analyze the annotated statement
        analyze(*stmt.statement);
    }

    // --- Expression Visitors ---
    void SemanticAnalyzer::visit(const BinaryExpr &expr)
    {
        if (expr.left)
            analyze(*expr.left);
        if (expr.right)
            analyze(*expr.right);
    }

    void SemanticAnalyzer::visit(const UnaryExpr &expr)
    {
        if (expr.right)
            analyze(*expr.right);
    }

    void SemanticAnalyzer::visit(const LiteralExpr &expr)
    {
        // Literals are always semantically correct.
    }

    void SemanticAnalyzer::visit(const VariableExpr &expr)
    {
        if (!symbols_.resolve(expr.name.get_lexeme()))
        {
            std::cerr << "Semantic Error: Use of undeclared variable '"
                      << expr.name.get_lexeme() << "'." << std::endl;
            had_error_ = true;
        }
    }

    void SemanticAnalyzer::visit(const CallExpr &expr)
    {
        // if (expr.callee)
        //     analyze(*expr.callee);

        for (const auto &arg : expr.arguments)
        {
            if (arg)
                analyze(*arg);
        }
    }

} // namespace thermolang