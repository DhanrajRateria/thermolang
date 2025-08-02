#include "thermolang/optimizer/Passes.h"
#include <unordered_map>
#include <variant>
#include "thermolang/optimizer/ConstantFoldingPass.h"

namespace thermolang::optimizer
{

    // Helper to extract a constant value from an operand if possible.
    std::optional<int64_t> get_int_value(const ir::Operand &op, const std::unordered_map<std::string, int64_t> &constants)
    {
        if (std::holds_alternative<int64_t>(op))
        {
            return std::get<int64_t>(op);
        }
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

        for (auto &block : function_ir.basic_blocks)
        {
            // This map tracks which registers hold a known constant value.
            // For a more advanced compiler, this would be a more complex data structure.
            std::unordered_map<std::string, int64_t> known_constants;

            for (auto &instr_ptr : block->instructions)
            {
                // 1. Propagate Constants
                if (auto *load_instr = dynamic_cast<ir::LoadConstInstr *>(instr_ptr.get()))
                {
                    if (std::holds_alternative<int64_t>(load_instr->value))
                    {
                        known_constants[load_instr->result_reg] = std::get<int64_t>(load_instr->value);
                    }
                    // Note: Could add float handling here as well.
                }

                // 2. Fold Expressions
                if (auto *binary_op = dynamic_cast<ir::BinaryOpInstr *>(instr_ptr.get()))
                {
                    // Check if both operands are known constants
                    auto val1 = get_int_value(binary_op->arg1, known_constants);
                    auto val2 = get_int_value(binary_op->arg2, known_constants);

                    if (val1 && val2)
                    {
                        // Both operands are constants, we can fold!
                        int64_t result;
                        switch (binary_op->opcode)
                        {
                        case ir::OpCode::ADD:
                            result = *val1 + *val2;
                            break;
                        case ir::OpCode::SUB:
                            result = *val1 - *val2;
                            break;
                        case ir::OpCode::MUL:
                            result = *val1 * *val2;
                            break;
                        case ir::OpCode::DIV:
                            if (*val2 == 0)
                                continue; // Avoid division by zero
                            result = *val1 / *val2;
                            break;
                        default:
                            continue; // Not a foldable operator
                        }

                        // Replace the binary operation with a simple load_const
                        auto new_instr = std::make_unique<ir::LoadConstInstr>(binary_op->result_reg, result);
                        instr_ptr.reset(new_instr.release());

                        // Update our knowledge: the result register now holds a new constant
                        known_constants[binary_op->result_reg] = result;

                        ir_changed = true;
                    }
                }
            }
        }
        return ir_changed;
    }

} // namespace thermolang::optimizer