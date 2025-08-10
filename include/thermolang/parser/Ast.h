#ifndef THERMOLANG_AST_H
#define THERMOLANG_AST_H

#include "thermolang/lexer/Token.h"
#include "thermolang/types/Type.h"
#include <vector>
#include <memory>
#include <optional>

namespace thermolang
{
    // Forward declarations
    struct Expr;
    struct Stmt;
    struct TypeExpr;
    struct BinaryExpr;
    struct UnaryExpr;
    struct LiteralExpr;
    struct VariableExpr;
    struct CallExpr;
    struct LetStmt;
    struct ReturnStmt;
    struct ExprStmt;
    struct FunctionStmt;
    struct BlockStmt;
    struct StochasticStmt;
    struct EnergyStmt;
    struct ThermalStmt;
    struct ParallelStmt;
    struct TypeStmt;
    struct AnnotationStmt;
    struct IfStmt;
    struct WhileStmt;

    // Type expression nodes
    struct NameTypeExpr;
    struct DistributionTypeExpr;
    struct FunctionTypeExpr;
    struct EnergyTypeExpr;
    struct CircuitTypeExpr;

    // Define visitor interfaces first
    struct TypeExprVisitor
    {
        virtual ~TypeExprVisitor() = default;
        virtual void visit(const NameTypeExpr &expr) = 0;
        virtual void visit(const DistributionTypeExpr &expr) = 0;
        virtual void visit(const FunctionTypeExpr &expr) = 0;
        virtual void visit(const EnergyTypeExpr &expr) = 0;
        virtual void visit(const CircuitTypeExpr &expr) = 0;
    };

    struct ExprVisitor
    {
        virtual ~ExprVisitor() = default;
        virtual void visit(const BinaryExpr &expr) = 0;
        virtual void visit(const UnaryExpr &expr) = 0;
        virtual void visit(const LiteralExpr &expr) = 0;
        virtual void visit(const VariableExpr &expr) = 0;
        virtual void visit(const CallExpr &expr) = 0;
    };

    struct StmtVisitor
    {
        virtual ~StmtVisitor() = default;
        virtual void visit(const LetStmt &stmt) = 0;
        virtual void visit(const ReturnStmt &stmt) = 0;
        virtual void visit(const ExprStmt &stmt) = 0;
        virtual void visit(const FunctionStmt &stmt) = 0;
        virtual void visit(const BlockStmt &stmt) = 0;
        virtual void visit(const StochasticStmt &stmt) = 0;
        virtual void visit(const EnergyStmt &stmt) = 0;
        virtual void visit(const IfStmt &stmt) = 0;
        virtual void visit(const WhileStmt &stmt) = 0;
        virtual void visit(const ThermalStmt &stmt) = 0;
        virtual void visit(const ParallelStmt &stmt) = 0;
        virtual void visit(const TypeStmt &stmt) = 0;
        virtual void visit(const AnnotationStmt &stmt) = 0;
    };

    // Base classes
    struct Expr
    {
        virtual ~Expr() = default;
        virtual void accept(ExprVisitor &visitor) const = 0;

        // Type information to be filled by semantic analysis
        mutable std::shared_ptr<Type> type;
    };

    struct TypeExpr
    {
        virtual ~TypeExpr() = default;
        virtual void accept(TypeExprVisitor &visitor) const = 0;
        virtual std::string to_string() const = 0;
    };

    struct Stmt
    {
        virtual ~Stmt() = default;
        virtual void accept(StmtVisitor &visitor) const = 0;
    };

    // Type expression classes
    struct NameTypeExpr : TypeExpr
    {
        Token name;

        explicit NameTypeExpr(Token name) : name(name) {}

        void accept(TypeExprVisitor &visitor) const override;
        std::string to_string() const override { return name.get_lexeme(); }
    };

    struct DistributionTypeExpr : TypeExpr
    {
        std::unique_ptr<TypeExpr> element_type;
        std::optional<Token> variance;

        DistributionTypeExpr(std::unique_ptr<TypeExpr> element_type, std::optional<Token> variance = std::nullopt)
            : element_type(std::move(element_type)), variance(variance) {}

        void accept(TypeExprVisitor &visitor) const override;
        std::string to_string() const override;
    };

    struct FunctionTypeExpr : TypeExpr
    {
        std::vector<std::unique_ptr<TypeExpr>> param_types;
        std::unique_ptr<TypeExpr> return_type;

        FunctionTypeExpr(std::vector<std::unique_ptr<TypeExpr>> param_types, std::unique_ptr<TypeExpr> return_type)
            : param_types(std::move(param_types)), return_type(std::move(return_type)) {}

        void accept(TypeExprVisitor &visitor) const override;
        std::string to_string() const override;
    };

    struct EnergyTypeExpr : TypeExpr
    {
        std::vector<std::unique_ptr<TypeExpr>> var_types;

        explicit EnergyTypeExpr(std::vector<std::unique_ptr<TypeExpr>> var_types)
            : var_types(std::move(var_types)) {}

        void accept(TypeExprVisitor &visitor) const override;
        std::string to_string() const override;
    };

    struct CircuitTypeExpr : TypeExpr
    {
        Token nodes;
        Token couplings;

        CircuitTypeExpr(Token nodes, Token couplings)
            : nodes(nodes), couplings(couplings) {}

        void accept(TypeExprVisitor &visitor) const override;
        std::string to_string() const override;
    };

    // Expression classes
    struct BinaryExpr : Expr
    {
        std::unique_ptr<Expr> left;
        Token op;
        std::unique_ptr<Expr> right;

        BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
            : left(std::move(left)), op(op), right(std::move(right)) {}

        void accept(ExprVisitor &visitor) const override;
    };

    struct UnaryExpr : Expr
    {
        Token op;
        std::unique_ptr<Expr> right;

        UnaryExpr(Token op, std::unique_ptr<Expr> right)
            : op(op), right(std::move(right)) {}

        void accept(ExprVisitor &visitor) const override;
    };

    struct LiteralExpr : Expr
    {
        Token value;

        explicit LiteralExpr(Token value) : value(value) {}

        void accept(ExprVisitor &visitor) const override;
    };

    struct VariableExpr : Expr
    {
        Token name;

        explicit VariableExpr(Token name) : name(name) {}

        void accept(ExprVisitor &visitor) const override;
    };

    struct CallExpr : Expr
    {
        std::unique_ptr<Expr> callee;
        Token paren; // The closing parenthesis, for error reporting
        std::vector<std::unique_ptr<Expr>> arguments;

        CallExpr(std::unique_ptr<Expr> callee, Token paren, std::vector<std::unique_ptr<Expr>> arguments)
            : callee(std::move(callee)), paren(paren), arguments(std::move(arguments)) {}

        void accept(ExprVisitor &visitor) const override;
    };

    // Statement classes
    struct LetStmt : Stmt
    {
        Token name;
        std::unique_ptr<TypeExpr> type_annotation;
        std::unique_ptr<Expr> initializer;

        LetStmt(Token name, std::unique_ptr<TypeExpr> type_annotation, std::unique_ptr<Expr> initializer)
            : name(name), type_annotation(std::move(type_annotation)), initializer(std::move(initializer)) {}

        void accept(StmtVisitor &visitor) const override;
    };

    struct ReturnStmt : Stmt
    {
        Token keyword;
        std::unique_ptr<Expr> value;

        ReturnStmt(Token keyword, std::unique_ptr<Expr> value)
            : keyword(keyword), value(std::move(value)) {}

        void accept(StmtVisitor &visitor) const override;
    };

    struct ExprStmt : Stmt
    {
        std::unique_ptr<Expr> expression;

        explicit ExprStmt(std::unique_ptr<Expr> expression)
            : expression(std::move(expression)) {}

        void accept(StmtVisitor &visitor) const override;
    };

    struct BlockStmt : Stmt
    {
        std::vector<std::unique_ptr<Stmt>> statements;

        explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements)
            : statements(std::move(statements)) {}

        void accept(StmtVisitor &visitor) const override;
    };

    struct IfStmt : Stmt
    {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> then_branch;
        std::unique_ptr<Stmt> else_branch; // Can be nullptr

        IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then_b, std::unique_ptr<Stmt> else_b)
            : condition(std::move(cond)), then_branch(std::move(then_b)), else_branch(std::move(else_b)) {}

        void accept(StmtVisitor &visitor) const override;
    };

    struct WhileStmt : Stmt
    {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> body;

        WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> b)
            : condition(std::move(cond)), body(std::move(b)) {}

        void accept(StmtVisitor &visitor) const override;
    };
    
    struct Parameter
    {
        Token name;
        std::unique_ptr<TypeExpr> type;

        Parameter(Token name, std::unique_ptr<TypeExpr> type)
            : name(name), type(std::move(type)) {}
    };

    struct FunctionStmt : Stmt
    {
        Token name;
        std::vector<Parameter> params;
        std::unique_ptr<TypeExpr> return_type;
        std::unique_ptr<BlockStmt> body;

        FunctionStmt(Token name, std::vector<Parameter> params,
                     std::unique_ptr<TypeExpr> return_type, std::unique_ptr<BlockStmt> body)
            : name(name), params(std::move(params)), return_type(std::move(return_type)), body(std::move(body)) {}

        void accept(StmtVisitor &visitor) const override;
    };

    // Domain-specific statement classes
    struct StochasticStmt : Stmt
    {
        Token keyword;
        std::unique_ptr<FunctionStmt> function;

        StochasticStmt(Token keyword, std::unique_ptr<FunctionStmt> function)
            : keyword(keyword), function(std::move(function)) {}

        void accept(StmtVisitor &visitor) const override;
    };

    struct EnergyStmt : Stmt
    {
        Token keyword;
        std::unique_ptr<FunctionStmt> function;

        EnergyStmt(Token keyword, std::unique_ptr<FunctionStmt> function)
            : keyword(keyword), function(std::move(function)) {}

        void accept(StmtVisitor &visitor) const override;
    };

    struct ThermalStmt : Stmt
    {
        Token keyword;
        std::unique_ptr<BlockStmt> block;

        ThermalStmt(Token keyword, std::unique_ptr<BlockStmt> block)
            : keyword(keyword), block(std::move(block)) {}

        void accept(StmtVisitor &visitor) const override;
    };

    struct ParallelStmt : Stmt
    {
        Token keyword;
        std::unique_ptr<BlockStmt> block;

        ParallelStmt(Token keyword, std::unique_ptr<BlockStmt> block)
            : keyword(keyword), block(std::move(block)) {}

        void accept(StmtVisitor &visitor) const override;
    };

    struct AnnotationValue
    {
        Token name;
        Token value;

        AnnotationValue(Token name, Token value)
            : name(name), value(value) {}
    };

    struct AnnotationStmt : Stmt
    {
        Token at; // '@' token
        Token name;
        std::vector<AnnotationValue> values;
        std::unique_ptr<Stmt> statement;

        AnnotationStmt(Token at, Token name, std::vector<AnnotationValue> values, std::unique_ptr<Stmt> statement)
            : at(at), name(name), values(std::move(values)), statement(std::move(statement)) {}

        void accept(StmtVisitor &visitor) const override;
    };

    struct TypeStmt : Stmt
    {
        Token keyword; // "type" token
        Token name;
        std::unique_ptr<TypeExpr> type_expr;

        TypeStmt(Token keyword, Token name, std::unique_ptr<TypeExpr> type_expr)
            : keyword(keyword), name(name), type_expr(std::move(type_expr)) {}

        void accept(StmtVisitor &visitor) const override;
    };

} // namespace thermolang
#endif // THERMOLANG_AST_H