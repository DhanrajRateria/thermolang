#include "thermolang/compiler/IrGenerator.h"
#include <stdexcept>
#include <iostream>

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
        // This method handles `energy fn ...`
        auto function_ir = std::make_unique<ir::FunctionIR>();
        function_ir->name = stmt.function->name.get_lexeme();
        function_ir->is_energy_function = true;
        current_function_ = function_ir.get();

        enter_scope();

        for (const auto &param : stmt.function->params)
        {
            std::string param_reg = new_register();
            function_ir->parameters.push_back(param_reg);
            define_variable(param.name.get_lexeme(), param_reg);
        }

        add_basic_block(std::make_unique<ir::BasicBlock>("entry"));

        // Visit the function body normally. The instructions for the energy
        // calculation will be added to the function's basic blocks.
        analyze(*stmt.function->body);

        // *** FIX: DO NOT add a `create_energy_func` instruction here. ***
        // The function itself serves as the energy function definition. The IR for its body
        // is the implementation. `create_energy_func` is for dynamically creating
        // energy function objects from expressions, which is a different use case.

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
        std::cout << "  IrGenerator: Found parallel block." << std::endl;
        std::string body_label = new_label();
        std::string exit_label = new_label();

        // The "collection" for parallel is the set of statements inside.
        // For IR, we can represent this as an integer count of parallel tasks.
        int task_count = stmt.block->statements.size();

        std::string collection_reg = new_register();
        add_instruction(std::make_unique<ir::LoadConstInstr>(collection_reg, (int64_t)task_count));

        std::string iterator_reg = new_register(); // Represents the task ID

        add_instruction(std::make_unique<ir::ParallelForInstr>(iterator_reg, collection_reg, body_label, exit_label));

        // The body of the parallel_for contains all the statements from the block.
        add_basic_block(std::make_unique<ir::BasicBlock>(body_label));
        analyze(*stmt.block);
        add_instruction(std::make_unique<ir::JumpInstr>(exit_label)); // Each parallel task jumps to the end.

        // Continue execution after the parallel block.
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

    void IrGenerator::visit(const IfStmt &stmt)
    {
        // Generate code for the condition
        analyze(*stmt.condition);
        std::string cond_reg = last_expr_result_reg_;

        // Create labels for the branches
        std::string then_label = new_label();
        std::string else_label = new_label();
        std::string merge_label = new_label();

        // The 'else' label is used if an else branch exists, otherwise we jump straight to merge.
        std::string false_target = stmt.else_branch ? else_label : merge_label;

        add_instruction(std::make_unique<ir::BranchInstr>(cond_reg, then_label, false_target));

        // Emit the 'then' block
        add_basic_block(std::make_unique<ir::BasicBlock>(then_label));
        analyze(*stmt.then_branch);
        add_instruction(std::make_unique<ir::JumpInstr>(merge_label)); // Jump to merge after 'then'

        // Emit the 'else' block if it exists
        if (stmt.else_branch)
        {
            add_basic_block(std::make_unique<ir::BasicBlock>(else_label));
            analyze(*stmt.else_branch);
            add_instruction(std::make_unique<ir::JumpInstr>(merge_label)); // Jump to merge after 'else'
        }

        // Emit the merge block
        add_basic_block(std::make_unique<ir::BasicBlock>(merge_label));
    }

    void IrGenerator::visit(const WhileStmt &stmt)
    {
        // Use new_label() to create unique labels
        std::string cond_label = "L_cond";
        std::string body_label = "L_body";
        std::string exit_label = "L_exit";

        // Jump to the condition check first
        add_instruction(std::make_unique<ir::JumpInstr>(cond_label));

        // Emit the condition block
        add_basic_block(std::make_unique<ir::BasicBlock>(cond_label));
        analyze(*stmt.condition);
        std::string cond_reg = last_expr_result_reg_;
        add_instruction(std::make_unique<ir::BranchInstr>(cond_reg, body_label, exit_label));

        // Emit the loop body block
        add_basic_block(std::make_unique<ir::BasicBlock>(body_label));
        analyze(*stmt.body);
        add_instruction(std::make_unique<ir::JumpInstr>(cond_label)); // Loop back to the condition

        // Emit the exit block
        add_basic_block(std::make_unique<ir::BasicBlock>(exit_label));
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
        auto register_opt = resolve_variable(expr.name.get_lexeme());

        if (register_opt.has_value())
        {
            last_expr_result_reg_ = *register_opt;
        }
        else
        {
            last_expr_result_reg_ = expr.name.get_lexeme();
        }
    }

    void IrGenerator::visit(const BinaryExpr &expr)
    {

        if (expr.op.get_type() == TokenType::EQUAL)
        {
            // The left side must be a variable. The parser guarantees this.
            auto *var_expr = dynamic_cast<const VariableExpr *>(expr.left.get());

            // First, evaluate the right-hand side to get the new value/register.
            analyze(*expr.right);
            std::string value_reg = last_expr_result_reg_;

            // Update the map so the variable name now points to the new register.
            // Our define_variable helper works for both new definitions and re-assignments.
            define_variable(var_expr->name.get_lexeme(), value_reg);

            // The result of an assignment expression is the assigned value itself.
            last_expr_result_reg_ = value_reg;

            // Return early to skip the general operator switch statement.
            return;
        }

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
        case TokenType::EQUAL_EQUAL:
            opcode = ir::OpCode::EQUAL;
            break;
        case TokenType::BANG_EQUAL:
            opcode = ir::OpCode::NOT_EQUAL;
            break;
        case TokenType::LESS:
            opcode = ir::OpCode::LESS;
            break;
        case TokenType::LESS_EQUAL:
            opcode = ir::OpCode::LESS_EQUAL;
            break;
        case TokenType::GREATER:
            opcode = ir::OpCode::GREATER;
            break;
        case TokenType::GREATER_EQUAL:
            opcode = ir::OpCode::GREATER_EQUAL;
            break;
        default:
            throw std::runtime_error("Unsupported binary operator for IR generation: " + expr.op.get_lexeme());
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
            else if (callee_name == "thermal_anneal" && expr.arguments.size() == 4)
            {
                analyze(*expr.arguments[0]);
                std::string energy_func_reg = last_expr_result_reg_;

                analyze(*expr.arguments[1]);
                std::string temp_reg = last_expr_result_reg_;

                analyze(*expr.arguments[2]);
                std::string rate_reg = last_expr_result_reg_;

                analyze(*expr.arguments[3]);
                std::string steps_reg = last_expr_result_reg_;

                std::string result_reg = new_register();
                // This instruction is now a bit of a misnomer, as it doesn't just take a schedule.
                // For now, we'll pack the args into the existing IR.
                // We'll create a temporary schedule object in the codegen.
                add_instruction(std::make_unique<ir::CallInstr>(result_reg, "thermal_anneal",
                                                                std::vector<ir::Operand>{energy_func_reg, temp_reg, rate_reg, steps_reg}));

                last_expr_result_reg_ = result_reg;
                return;
            }

            else if (callee_name == "thermal_denoise" && expr.arguments.size() == 3)
            {
                analyze(*expr.arguments[0]); 
                std::string target_func_reg = last_expr_result_reg_;

                analyze(*expr.arguments[1]); 
                std::string sigma_reg = last_expr_result_reg_;

                analyze(*expr.arguments[2]); 
                std::string steps_reg = last_expr_result_reg_;

                std::string result_reg = new_register();
                
                add_instruction(std::make_unique<ir::DenoiseInstr>(
                    result_reg, 
                    target_func_reg, 
                    ir::Operand{sigma_reg}, 
                    ir::Operand{steps_reg}
                ));

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

    std::optional<std::string> IrGenerator::resolve_variable(const std::string &name)
    {
        for (auto it = variable_to_register_map_.rbegin(); it != variable_to_register_map_.rend(); ++it)
        {
            if (it->count(name))
            {
                return it->at(name);
            }
        }
        return std::nullopt;
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