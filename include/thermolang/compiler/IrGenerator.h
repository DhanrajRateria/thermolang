#ifndef THERMOLANG_IR_GENERATOR_H
#define THERMOLANG_IR_GENERATOR_H

#include "thermolang/parser/Ast.h"
#include "thermolang/ir/ThermoIR.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <stack>

namespace thermolang::compiler
{
    class IrGenerator : public StmtVisitor, public ExprVisitor
    {
    public:
        // Takes an AST and generates a list of IR functions
        std::vector<std::unique_ptr<ir::FunctionIR>> generate(const std::vector<std::unique_ptr<Stmt>> &ast);

    private:
        // Visitor implementations
        void visit(const FunctionStmt &stmt) override;
        void visit(const LetStmt &stmt) override;
        void visit(const ExprStmt &stmt) override;
        void visit(const ReturnStmt &stmt) override;
        void visit(const BlockStmt &stmt) override;
        void visit(const StochasticStmt &stmt) override;
        void visit(const EnergyStmt &stmt) override;
        void visit(const ThermalStmt &stmt) override;
        void visit(const ParallelStmt &stmt) override;

        // Expressions
        void visit(const BinaryExpr &expr) override;
        void visit(const LiteralExpr &expr) override;
        void visit(const VariableExpr &expr) override;
        void visit(const CallExpr &expr) override;
        void visit(const UnaryExpr &expr) override;

        // Unimplemented visitors
        void visit(const TypeStmt &stmt) override {}
        void visit(const AnnotationStmt &stmt) override { stmt.statement->accept(*this); }

        // Helper methods
        void analyze(const Stmt &stmt) { const_cast<Stmt &>(stmt).accept(*this); }
        void analyze(const Expr &expr) { const_cast<Expr &>(expr).accept(*this); }

        std::string new_register();
        std::string new_label();
        std::string new_energy_expr_id();
        ir::BasicBlock *current_block();
        void add_instruction(std::unique_ptr<ir::Instruction> instr);
        void add_basic_block(std::unique_ptr<ir::BasicBlock> block);

        // Specific domain helpers
        void generate_stochastic_call(const CallExpr &expr);
        void generate_energy_function_body(const BlockStmt &body, const std::vector<Parameter> &params);
        std::string create_energy_expression(const Expr &expr, const std::vector<std::string> &var_regs);

        // State
        std::vector<std::unique_ptr<ir::FunctionIR>> ir_functions_;
        ir::FunctionIR *current_function_ = nullptr;

        // Scoped mapping of ThermoLang variable names to IR register names
        std::vector<std::unordered_map<std::string, std::string>> variable_to_register_map_;
        void enter_scope();
        void exit_scope();
        void define_variable(const std::string &name, const std::string &reg);
        std::string resolve_variable(const std::string &name);

        // State for expression evaluation
        std::string last_expr_result_reg_;

        // State for thermal blocks
        std::stack<std::string> temperature_registers_;

        int register_counter_ = 0;
        int label_counter_ = 0;
        int energy_expr_counter_ = 0;
    };
} // namespace thermolang::compiler

#endif // THERMOLANG_IR_GENERATOR_H