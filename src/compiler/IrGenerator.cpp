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
        add_basic_block(std::make_unique<ir::BasicBlock>("entry"));

        // Visit the function body
        analyze(*stmt.body);

        exit_scope();

        ir_functions_.push_back(std::move(function_ir));
        current_function_ = nullptr;
    }

    void IrGenerator::visit(const StochasticStmt &stmt)
    {
        // Mark the function as stochastic
        auto function_ir = std::make_unique<ir::FunctionIR>();
        function_ir->name = stmt.function->name.get_lexeme();
        function_ir->is_stochastic = true;
        current_function_ = function_ir.get();

        enter_scope(); // Scope for parameters

        for (const auto &param : stmt.function->params)
        {
            std::string param_reg = new_register();
            function_ir->parameters.push_back(param_reg);
            define_variable(param.name.get_lexeme(), param_reg);
        }

        // Create the entry block
        add_basic_block(std::make_unique<ir::BasicBlock>("entry"));

        // Visit the function body
        analyze(*stmt.function->body);

        exit_scope();

        ir_functions_.push_back(std::move(function_ir));
        current_function_ = nullptr;
    }

    void IrGenerator::visit(const EnergyStmt &stmt)
    {
        // Mark the function as an energy function
        auto function_ir = std::make_unique<ir::FunctionIR>();
        function_ir->name = stmt.function->name.get_lexeme();
        function_ir->is_energy_function = true;
        current_function_ = function_ir.get();

        enter_scope(); // Scope for parameters

        for (const auto &param : stmt.function->params)
        {
            std::string param_reg = new_register();
            function_ir->parameters.push_back(param_reg);
            define_variable(param.name.get_lexeme(), param_reg);
        }

        // Create the entry block
        add_basic_block(std::make_unique<ir::BasicBlock>("entry"));

        // Generate the energy function body with special handling
        generate_energy_function_body(*stmt.function->body, stmt.function->params);

        exit_scope();

        ir_functions_.push_back(std::move(function_ir));
        current_function_ = nullptr;
    }

    void IrGenerator::visit(const ThermalStmt &stmt)
    {
        // Create a thermal block with temperature management
        std::string temp_reg = new_register();

        // Default temperature if none is explicitly set
        add_instruction(std::make_unique<ir::LoadConstInstr>(temp_reg, 1.0));

        // Push the temperature register to the stack
        temperature_registers_.push(temp_reg);

        // Process the block statements
        analyze(*stmt.block);

        // Pop the temperature register
        temperature_registers_.pop();
    }

    void IrGenerator::visit(const ParallelStmt &stmt)
    {
        // Create entry and exit blocks for the parallel region
        std::string body_label = new_label();
        std::string exit_label = new_label();

        // Add the parallel for instruction
        std::string iterator_reg = new_register();
        std::string collection_reg = new_register();

        // Dummy collection for now - would be calculated from the actual collection
        add_instruction(std::make_unique<ir::LoadConstInstr>(collection_reg, 8)); // Represent 8 items

        add_instruction(std::make_unique<ir::ParallelForInstr>(
            iterator_reg, collection_reg, body_label, exit_label));

        // Add the body block
        add_basic_block(std::make_unique<ir::BasicBlock>(body_label));

        // Process the parallel block
        analyze(*stmt.block);

        // Jump to exit
        add_instruction(std::make_unique<ir::BinaryOpInstr>(
            ir::OpCode::JUMP, "", exit_label, ""));

        // Add the exit block
        add_basic_block(std::make_unique<ir::BasicBlock>(exit_label));
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
        else if (expr.value.get_type() == TokenType::STRING_LITERAL)
        {
            value = ir::Operand{std::get<std::string>(expr.value.get_literal())};
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

    void IrGenerator::visit(const UnaryExpr &expr)
    {
        analyze(*expr.right);
        std::string right_reg = last_expr_result_reg_;
        std::string result_reg = new_register();

        if (expr.op.get_type() == TokenType::MINUS)
        {
            // Negate by multiplying by -1
            std::string neg_one_reg = new_register();
            add_instruction(std::make_unique<ir::LoadConstInstr>(neg_one_reg, -1));
            add_instruction(std::make_unique<ir::BinaryOpInstr>(
                ir::OpCode::MUL, result_reg, right_reg, neg_one_reg));
        }
        else if (expr.op.get_type() == TokenType::BANG)
        {
            // We'd need proper boolean operations here
            // For now, just assign the negation (placeholder)
            add_instruction(std::make_unique<ir::BinaryOpInstr>(
                ir::OpCode::SUB, result_reg, "1", right_reg)); // 1 - value for boolean NOT
        }

        last_expr_result_reg_ = result_reg;
    }

    void IrGenerator::visit(const CallExpr &expr)
    {
        // Check if this is a stochastic function call
        if (auto *var_expr = dynamic_cast<const VariableExpr *>(expr.callee.get()))
        {
            std::string callee_name = var_expr->name.get_lexeme();

            // Handle specific stochastic functions
            if (callee_name == "sample_gaussian" && expr.arguments.size() == 2)
            {
                // Process arguments
                analyze(*expr.arguments[0]);
                std::string mean_reg = last_expr_result_reg_;

                analyze(*expr.arguments[1]);
                std::string variance_reg = last_expr_result_reg_;

                // Create result register
                std::string result_reg = new_register();

                // Generate specialized instruction
                add_instruction(std::make_unique<ir::SampleGaussianInstr>(
                    result_reg, mean_reg, variance_reg));

                last_expr_result_reg_ = result_reg;
                return;
            }
            else if (callee_name == "sample_uniform" && expr.arguments.size() == 2)
            {
                // Process arguments
                analyze(*expr.arguments[0]);
                std::string low_reg = last_expr_result_reg_;

                analyze(*expr.arguments[1]);
                std::string high_reg = last_expr_result_reg_;

                // Create result register
                std::string result_reg = new_register();

                // Generate specialized instruction
                add_instruction(std::make_unique<ir::SampleUniformInstr>(
                    result_reg, low_reg, high_reg));

                last_expr_result_reg_ = result_reg;
                return;
            }
            else if (callee_name == "minimize_energy" && expr.arguments.size() >= 1)
            {
                // Process the energy function argument
                analyze(*expr.arguments[0]);
                std::string energy_func_reg = last_expr_result_reg_;

                // Process initial values if provided
                std::vector<ir::Operand> initial_values;
                for (size_t i = 1; i < expr.arguments.size(); ++i)
                {
                    analyze(*expr.arguments[i]);
                    initial_values.push_back(ir::Operand{last_expr_result_reg_});
                }

                // Create result register
                std::string result_reg = new_register();

                // Generate specialized instruction
                add_instruction(std::make_unique<ir::MinimizeEnergyInstr>(
                    result_reg, energy_func_reg, initial_values));

                last_expr_result_reg_ = result_reg;
                return;
            }
            else if (callee_name == "thermal_anneal" && expr.arguments.size() >= 1)
            {
                // Process the energy function argument
                analyze(*expr.arguments[0]);
                std::string energy_func_reg = last_expr_result_reg_;

                // Default values for optional parameters
                std::string initial_temp_reg = new_register();
                std::string cooling_rate_reg = new_register();
                std::string steps_reg = new_register();

                add_instruction(std::make_unique<ir::LoadConstInstr>(initial_temp_reg, 1.0));
                add_instruction(std::make_unique<ir::LoadConstInstr>(cooling_rate_reg, 0.95));
                add_instruction(std::make_unique<ir::LoadConstInstr>(steps_reg, 1000));

                // Override with provided parameters
                if (expr.arguments.size() > 1)
                {
                    analyze(*expr.arguments[1]);
                    initial_temp_reg = last_expr_result_reg_;
                }

                if (expr.arguments.size() > 2)
                {
                    analyze(*expr.arguments[2]);
                    cooling_rate_reg = last_expr_result_reg_;
                }

                if (expr.arguments.size() > 3)
                {
                    analyze(*expr.arguments[3]);
                    steps_reg = last_expr_result_reg_;
                }

                // Create result register
                std::string result_reg = new_register();

                // Generate specialized instruction
                add_instruction(std::make_unique<ir::ThermalAnnealInstr>(
                    result_reg, energy_func_reg, initial_temp_reg, cooling_rate_reg, steps_reg));

                last_expr_result_reg_ = result_reg;
                return;
            }
        }

        // Standard function call processing
        std::vector<ir::Operand> arg_regs;
        for (const auto &arg : expr.arguments)
        {
            analyze(*arg);
            arg_regs.push_back(ir::Operand{last_expr_result_reg_});
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

    // --- Helper implementations ---

    std::string IrGenerator::new_register()
    {
        return "r" + std::to_string(register_counter_++);
    }

    std::string IrGenerator::new_label()
    {
        return "L" + std::to_string(label_counter_++);
    }

    std::string IrGenerator::new_energy_expr_id()
    {
        return "E" + std::to_string(energy_expr_counter_++);
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

    void IrGenerator::add_basic_block(std::unique_ptr<ir::BasicBlock> block)
    {
        current_function_->basic_blocks.push_back(std::move(block));
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

    void IrGenerator::generate_energy_function_body(const BlockStmt &body, const std::vector<Parameter> &params)
    {
        // Process the body normally first
        analyze(body);

        // Create an energy expression from the function body
        std::vector<std::string> param_regs;
        for (size_t i = 0; i < params.size(); ++i)
        {
            param_regs.push_back(current_function_->parameters[i]);
        }

        // Create an energy function reference
        std::string energy_func_reg = new_register();
        std::string energy_expr_id = new_energy_expr_id();

        std::vector<ir::Operand> param_operands;
        for (const auto &reg : param_regs)
        {
            param_operands.push_back(ir::Operand{reg});
        }

        // The last expression result should be the energy value
        add_instruction(std::make_unique<ir::CreateEnergyFuncInstr>(
            energy_func_reg, param_operands, energy_expr_id));

        // Store the energy expression for later reference
        auto energy_expr = std::make_unique<ir::EnergyExpression>(
            energy_expr_id,
            std::vector<std::string>{}, // Would be parameter names
            std::make_unique<ir::BasicBlock>("energy_expr_" + energy_expr_id),
            last_expr_result_reg_);

        current_function_->energy_expressions[energy_expr_id] = std::move(energy_expr);
    }

    std::string IrGenerator::create_energy_expression(const Expr &expr, const std::vector<std::string> &var_regs)
    {
        // Save current state
        auto temp_block = current_function_->basic_blocks.back().release();
        current_function_->basic_blocks.pop_back();

        // Create a new block for the energy expression
        std::string energy_expr_id = new_energy_expr_id();
        auto expr_block = std::make_unique<ir::BasicBlock>("energy_expr_" + energy_expr_id);
        current_function_->basic_blocks.push_back(std::move(expr_block));

        // Generate code for the expression
        analyze(expr);
        std::string result_reg = last_expr_result_reg_;

        // Save the energy expression
        auto energy_expr = std::make_unique<ir::EnergyExpression>(
            energy_expr_id,
            std::vector<std::string>{}, // Would be variable names
            std::move(current_function_->basic_blocks.back()),
            result_reg);

        current_function_->basic_blocks.pop_back();
        current_function_->energy_expressions[energy_expr_id] = std::move(energy_expr);

        // Restore original block
        current_function_->basic_blocks.push_back(std::unique_ptr<ir::BasicBlock>(temp_block));

        // Create the energy function
        std::vector<ir::Operand> var_operands;
        for (const auto &reg : var_regs)
        {
            var_operands.push_back(ir::Operand{reg});
        }
        std::string energy_func_reg = new_register();
        add_instruction(std::make_unique<ir::CreateEnergyFuncInstr>(
            energy_func_reg, var_operands, energy_expr_id));

        return energy_func_reg;
    }

} // namespace thermolang::compiler