#ifndef THERMOLANG_SEMANTIC_ANALYZER_H
#define THERMOLANG_SEMANTIC_ANALYZER_H

#include "thermolang/parser/Ast.h"
#include "thermolang/parser/SymbolTable.h"
#include <vector>

namespace thermolang
{

    class SemanticAnalyzer : public StmtVisitor, public ExprVisitor
    {
    public:
        // Default constructor that creates its own symbol table
        SemanticAnalyzer();

        // Constructor that takes a reference to a symbol table
        explicit SemanticAnalyzer(SymbolTable &symbols);

        // Takes a list of statements (the AST) and analyzes them.
        // Returns true if no errors were found.
        bool analyze(const std::vector<std::unique_ptr<Stmt>> &statements);

        // Statement visitor implementations
        void visit(const LetStmt &stmt) override;
        void visit(const ExprStmt &stmt) override;
        void visit(const FunctionStmt &stmt) override;
        void visit(const BlockStmt &stmt) override;
        void visit(const ReturnStmt &stmt) override;
        void visit(const IfStmt &stmt) override;
        void visit(const WhileStmt &stmt) override;

        // Domain-specific constructs
        void visit(const StochasticStmt &stmt) override;
        void visit(const EnergyStmt &stmt) override;
        void visit(const ThermalStmt &stmt) override;
        void visit(const ParallelStmt &stmt) override;
        void visit(const TypeStmt &stmt) override;
        void visit(const AnnotationStmt &stmt) override;

        // Expression visitor implementations
        void visit(const BinaryExpr &expr) override;
        void visit(const UnaryExpr &expr) override;
        void visit(const LiteralExpr &expr) override;
        void visit(const VariableExpr &expr) override;
        void visit(const CallExpr &expr) override;

    private:
        void analyze(const Stmt &stmt) { const_cast<Stmt &>(stmt).accept(*this); }
        void analyze(const Expr &expr) { const_cast<Expr &>(expr).accept(*this); }


        // If we created our own symbol table, store it here
        std::unique_ptr<SymbolTable> owned_symbols_;
        SymbolTable &symbols_;

        bool had_error_ = false;
    };

} // namespace thermolang

#endif // THERMOLANG_SEMANTIC_ANALYZER_H