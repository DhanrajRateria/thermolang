#include "thermolang/compiler/IrGenerator.h"
#include <stdexcept>

namespace thermolang::compiler
{

    std::vector<std::unique_ptr<ir::FunctionIR>> IrGenerator::generate(const std::vector<std::unique_ptr<Stmt>> &ast)
    {
        for (const auto &stmt : ast)
        {
            analyze(*stmt);
        }
        return std::move(ir_functions_);
    }

    // --- Visitor Implementations ---

    void IrGenerator::visit(const FunctionStmt &stmt)
    {
        auto function_ir = std::make_unique<ir::FunctionIR>();
        function_ir->name = stmt.name.get_lexeme();
        current_function_ = function_ir.get();

        enter_scope(); // Scope for parameters

        for (const auto &param : stmt.params)
        {
            std::string param_reg = new_register();
            function_ir->parameters.push_back(param_reg);
            define_variable(param.name.get_lexeme(), param_reg);
        }

        // Create the entry block
        function_ir->basic_blocks.push_back(std::make_unique<ir::BasicBlock>("entry"));

        // Visit the function body
        analyze(*stmt.body);

        exit_scope();

        ir_functions_.push_back(std::move(function_ir));
        current_function_ = nullptr;
    }

    void IrGenerator::visit(const BlockStmt &stmt)
    {
        enter_scope();
        for (const auto &s : stmt.statements)
        {
            analyze(*s);
        }
        exit_scope();
    }

    void IrGenerator::visit(const LetStmt &stmt)
    {
        // First, evaluate the initializer to get its result in a register
        analyze(*stmt.initializer);
        std::string initializer_reg = last_expr_result_reg_;

        // The variable 'name' will now be represented by this register
        define_variable(stmt.name.get_lexeme(), initializer_reg);
    }

    void IrGenerator::visit(const ExprStmt &stmt)
    {
        analyze(*stmt.expression);
    }

    void IrGenerator::visit(const ReturnStmt &stmt)
    {
        if (stmt.value)
        {
            analyze(*stmt.value);
            add_instruction(std::make_unique<ir::ReturnInstr>(last_expr_result_reg_));
        }
        else
        {
            add_instruction(std::make_unique<ir::ReturnInstr>(std::nullopt));
        }
    }

    // --- Expression Visitors ---

    void IrGenerator::visit(const LiteralExpr &expr)
    {
        std::string reg = new_register();
        ir::Operand value;

        if (expr.value.get_type() == TokenType::INTEGER_LITERAL)
        {
            value = std::get<int64_t>(expr.value.get_literal());
        }
        else if (expr.value.get_type() == TokenType::FLOAT_LITERAL)
        {
            value = std::get<double>(expr.value.get_literal());
        }
        else if (expr.value.get_type() == TokenType::BOOL_LITERAL)
        {
            value = std::get<bool>(expr.value.get_literal());
        }

        add_instruction(std::make_unique<ir::LoadConstInstr>(reg, value));
        last_expr_result_reg_ = reg;
    }

    void IrGenerator::visit(const VariableExpr &expr)
    {
        // The result is simply the register assigned to this variable
        last_expr_result_reg_ = resolve_variable(expr.name.get_lexeme());
    }

    void IrGenerator::visit(const BinaryExpr &expr)
    {
        analyze(*expr.left);
        std::string left_reg = last_expr_result_reg_;

        analyze(*expr.right);
        std::string right_reg = last_expr_result_reg_;

        std::string result_reg = new_register();
        ir::OpCode opcode;
        switch (expr.op.get_type())
        {
        case TokenType::PLUS:
            opcode = ir::OpCode::ADD;
            break;
        case TokenType::MINUS:
            opcode = ir::OpCode::SUB;
            break;
        case TokenType::STAR:
            opcode = ir::OpCode::MUL;
            break;
        case TokenType::SLASH:
            opcode = ir::OpCode::DIV;
            break;
        default:
            throw std::runtime_error("Unsupported binary operator for IR generation");
        }

        add_instruction(std::make_unique<ir::BinaryOpInstr>(opcode, result_reg, left_reg, right_reg));
        last_expr_result_reg_ = result_reg;
    }

    void IrGenerator::visit(const CallExpr &expr)
    {
        std::vector<ir::Operand> arg_regs;
        for (const auto &arg : expr.arguments)
        {
            analyze(*arg);
            arg_regs.push_back(last_expr_result_reg_);
        }

        std::string callee_name = dynamic_cast<VariableExpr *>(expr.callee.get())->name.get_lexeme();

        if (expr.type && expr.type->to_string() != "void")
        {
            std::string result_reg = new_register();
            add_instruction(std::make_unique<ir::CallInstr>(result_reg, callee_name, arg_regs));
            last_expr_result_reg_ = result_reg;
        }
        else
        {
            add_instruction(std::make_unique<ir::CallInstr>(std::nullopt, callee_name, arg_regs));
            last_expr_result_reg_ = ""; // No result
        }
    }

    // --- Helper Implementations ---

    std::string IrGenerator::new_register()
    {
        return "r" + std::to_string(register_counter_++);
    }

    ir::BasicBlock *IrGenerator::current_block()
    {
        if (!current_function_ || current_function_->basic_blocks.empty())
        {
            throw std::runtime_error("Attempted to add instruction with no active basic block.");
        }
        return current_function_->basic_blocks.back().get();
    }

    void IrGenerator::add_instruction(std::unique_ptr<ir::Instruction> instr)
    {
        current_block()->instructions.push_back(std::move(instr));
    }

    void IrGenerator::enter_scope()
    {
        variable_to_register_map_.emplace_back();
    }

    void IrGenerator::exit_scope()
    {
        variable_to_register_map_.pop_back();
    }

    void IrGenerator::define_variable(const std::string &name, const std::string &reg)
    {
        if (variable_to_register_map_.empty())
            enter_scope();
        variable_to_register_map_.back()[name] = reg;
    }

    std::string IrGenerator::resolve_variable(const std::string &name)
    {
        for (auto it = variable_to_register_map_.rbegin(); it != variable_to_register_map_.rend(); ++it)
        {
            if (it->count(name))
            {
                return it->at(name);
            }
        }
        throw std::runtime_error("Undeclared variable used in IR generation: " + name);
    }

} // namespace thermolang::compiler