#ifndef THERMOLANG_TYPE_CHECKER_H
#define THERMOLANG_TYPE_CHECKER_H

#include "thermolang/parser/Ast.h"
#include "thermolang/parser/SymbolTable.h"
#include "thermolang/types/Type.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <iostream>

namespace thermolang
{

    // Class to resolve TypeExpr to actual Type objects
    class TypeResolver : public TypeExprVisitor
    {
    public:
        TypeResolver(SymbolTable &symbols);

        // Resolve a type expression to a concrete type
        std::shared_ptr<Type> resolve(const TypeExpr &type_expr);

        // TypeExprVisitor implementation
        void visit(const NameTypeExpr &expr) override;
        void visit(const DistributionTypeExpr &expr) override;
        void visit(const FunctionTypeExpr &expr) override;
        void visit(const EnergyTypeExpr &expr) override;
        void visit(const CircuitTypeExpr &expr) override;

    private:
        SymbolTable &symbols_;
        std::shared_ptr<Type> result_;

        // Helper methods for resolving primitive types
        std::shared_ptr<Type> resolve_primitive(const std::string &name);
    };

    // Main type checker class
    class TypeChecker : public StmtVisitor, public ExprVisitor
    {
    public:
        TypeChecker(SymbolTable &symbols);

        // Takes a list of statements (the AST) and type checks them.
        // Returns true if no errors were found.
        bool check(const std::vector<std::unique_ptr<Stmt>> &statements);

        // StmtVisitor implementation
        void visit(const LetStmt &stmt) override;
        void visit(const ExprStmt &stmt) override;
        void visit(const FunctionStmt &stmt) override;
        void visit(const BlockStmt &stmt) override;
        void visit(const ReturnStmt &stmt) override;
        void visit(const StochasticStmt &stmt) override;
        void visit(const EnergyStmt &stmt) override;
        void visit(const ThermalStmt &stmt) override;
        void visit(const ParallelStmt &stmt) override;
        void visit(const TypeStmt &stmt) override;
        void visit(const AnnotationStmt &stmt) override;

        // ExprVisitor implementation
        void visit(const BinaryExpr &expr) override;
        void visit(const UnaryExpr &expr) override;
        void visit(const LiteralExpr &expr) override;
        void visit(const VariableExpr &expr) override;
        void visit(const CallExpr &expr) override;

    private:
        // Helper methods for checking
        void check(const Stmt &stmt);
        void check(const Expr &expr);

        // Check if two types are compatible
        bool check_compatibility(const Type &expected, const Type &actual, const std::string &context);

        // Type inference for literals
        std::shared_ptr<Type> infer_literal_type(const Token &token);

        // Handle function registration in symbol table
        void register_function(const FunctionStmt &stmt, bool is_stochastic = false, bool is_energy = false);

        // Current function return type (for checking return statements)
        std::shared_ptr<Type> current_function_return_type_;

        // Symbol table reference
        SymbolTable &symbols_;

        // Type resolver
        TypeResolver resolver_;

        // Error tracking
        bool had_error_ = false;

        // Pre-analysis phase to register all functions before checking
        void pre_register_functions(const std::vector<std::unique_ptr<Stmt>> &statements);
        void pre_register_function_stmt(const Stmt &stmt);
        void pre_register_types(const std::vector<std::unique_ptr<Stmt>> &statements);
    };

} // namespace thermolang

#endif // THERMOLANG_TYPE_CHECKER_H