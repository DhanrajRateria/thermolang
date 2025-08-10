#ifndef THERMOLANG_CODE_GENERATOR_H
#define THERMOLANG_CODE_GENERATOR_H

#include "thermolang/ir/ThermoIR.h"
#include <string>
#include <vector>
#include <memory>
#include <sstream>

namespace thermolang::codegen
{

    // Abstract base class for all code generators
    class CodeGenerator
    {
    public:
        virtual ~CodeGenerator() = default;
        virtual std::string generate(const std::vector<std::unique_ptr<ir::FunctionIR>> &program) = 0;
    };

    // Concrete generator for Python simulation
    class SimulationCodeGenerator : public CodeGenerator
    {
    public:
        std::string generate(const std::vector<std::unique_ptr<ir::FunctionIR>> &program) override;

    private:
        void generate_function(const ir::FunctionIR &function);
        void generate_instruction(const ir::Instruction &instr);
        std::string operand_to_string(const ir::Operand &op);

        void indent();

        std::stringstream ss_;
        int indent_level_ = 0;
        const std::vector<std::unique_ptr<ir::FunctionIR>> *program_ir_ = nullptr;
    };

} // namespace thermolang::codegen

#endif // THERMOLANG_CODE_GENERATOR_H