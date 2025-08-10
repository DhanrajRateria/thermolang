#include "thermolang/optimizer/ConstantFoldingPass.h"
#include <variant>
#include <iostream>

namespace thermolang::optimizer
{
    // Helper variant for holding constant values (int or float)
    using ConstantValue = std::variant<int64_t, double>;

    // Helper to get a constant value from an operand
    std::optional<ConstantValue> get_const_value(const ir::Operand &op, const std::unordered_map<std::string, ConstantValue> &constants)
    {
        if (std::holds_alternative<int64_t>(op))
            return std::get<int64_t>(op);
        if (std::holds_alternative<double>(op))
            return std::get<double>(op);
        if (std::holds_alternative<std::string>(op))
        {
            const auto &reg = std::get<std::string>(op);
            auto it = constants.find(reg);
            if (it != constants.end())
            {
                return it->second;
            }
        }
        return std::nullopt;
    }

    bool ConstantFoldingPass::run(ir::FunctionIR &function_ir)
    {
        bool ir_changed = false;
        std::cout << "Running Constant Folding on '" << function_ir.name << "'" << std::endl;
        int fold_count = 0;

        for (auto &block : function_ir.basic_blocks)
        {
            // This map tracks which registers hold known constant values
            std::unordered_map<std::string, ConstantValue> known_constants;
            std::vector<std::unique_ptr<ir::Instruction>> new_instructions;

            for (auto &instr_ptr : block->instructions)
            {
                bool instruction_folded = false;

                // 1. Propagate Constants from load instructions
                if (auto *load_instr = dynamic_cast<ir::LoadConstInstr *>(instr_ptr.get()))
                {
                    if (std::holds_alternative<int64_t>(load_instr->value))
                    {
                        known_constants[load_instr->result_reg] = std::get<int64_t>(load_instr->value);
                    }
                    else if (std::holds_alternative<double>(load_instr->value))
                    {
                        known_constants[load_instr->result_reg] = std::get<double>(load_instr->value);
                    }
                }

                // 2. Fold Binary Operations
                if (auto *binary_op = dynamic_cast<ir::BinaryOpInstr *>(instr_ptr.get()))
                {
                    auto val1_opt = get_const_value(binary_op->arg1, known_constants);
                    auto val2_opt = get_const_value(binary_op->arg2, known_constants);

                    if (val1_opt && val2_opt)
                    {
                        ConstantValue v1 = *val1_opt;
                        ConstantValue v2 = *val2_opt;
                        std::optional<ConstantValue> result;

                        // Perform the operation based on types
                        try
                        {
                            if (std::holds_alternative<double>(v1) || std::holds_alternative<double>(v2))
                            {
                                // At least one operand is double, so perform floating-point arithmetic
                                double d1 = std::holds_alternative<double>(v1) ? std::get<double>(v1) : static_cast<double>(std::get<int64_t>(v1));
                                double d2 = std::holds_alternative<double>(v2) ? std::get<double>(v2) : static_cast<double>(std::get<int64_t>(v2));

                                switch (binary_op->opcode)
                                {
                                case ir::OpCode::ADD:
                                    result = d1 + d2;
                                    break;
                                case ir::OpCode::SUB:
                                    result = d1 - d2;
                                    break;
                                case ir::OpCode::MUL:
                                    result = d1 * d2;
                                    break;
                                case ir::OpCode::DIV:
                                    if (d2 != 0.0)
                                        result = d1 / d2;
                                    break;
                                default:
                                    break;
                                }
                            }
                            else
                            {
                                // Both operands are integers, perform integer arithmetic
                                int64_t i1 = std::get<int64_t>(v1);
                                int64_t i2 = std::get<int64_t>(v2);

                                switch (binary_op->opcode)
                                {
                                case ir::OpCode::ADD:
                                    result = i1 + i2;
                                    break;
                                case ir::OpCode::SUB:
                                    result = i1 - i2;
                                    break;
                                case ir::OpCode::MUL:
                                    result = i1 * i2;
                                    break;
                                case ir::OpCode::DIV:
                                    if (i2 != 0)
                                        result = i1 / i2;
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                        catch (const std::bad_variant_access &e)
                        {
                            std::cerr << "Type error during constant folding: " << e.what() << std::endl;
                        }

                        if (result.has_value())
                        {
                            // Replace the binary operation with a constant load instruction
                            ir::Operand new_const_op;
                            if (std::holds_alternative<double>(*result))
                            {
                                new_const_op = std::get<double>(*result);
                            }
                            else
                            {
                                new_const_op = std::get<int64_t>(*result);
                            }

                            auto new_instr = std::make_unique<ir::LoadConstInstr>(binary_op->result_reg, new_const_op);
                            new_instructions.push_back(std::move(new_instr));

                            // Update our knowledge of constants
                            known_constants[binary_op->result_reg] = *result;
                            instruction_folded = true;
                            ir_changed = true;
                            fold_count++;
                        }
                    }
                }

                // If we didn't fold this instruction, keep it unchanged
                if (!instruction_folded)
                {
                    new_instructions.push_back(std::move(instr_ptr));
                }
            }

            // Replace the block's instructions with our optimized version
            block->instructions = std::move(new_instructions);
        }

        if (ir_changed)
        {
            std::cout << "  Folded " << fold_count << " constant expressions" << std::endl;
        }

        return ir_changed;
    }

} // namespace thermolang::optimizer