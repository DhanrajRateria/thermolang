#ifndef THERMOLANG_THRML_CODE_GENERATOR_H
#define THERMOLANG_THRML_CODE_GENERATOR_H

#include "thermolang/codegen/CodeGenerator.h"

namespace thermolang::codegen
{
    // Generates a Python script that uses the 'thrml' library.
    class ThrmlCodeGenerator : public CodeGenerator
    {
    public:
        std::string generate(const std::vector<std::unique_ptr<ir::FunctionIR>> &program) override;
    
    private:
        // Helper to format Python lists and matrices.
        std::string format_python_list(const std::vector<double>& vec);
        std::string format_python_list_int(const std::vector<int>& vec);
        std::string format_python_matrix(const std::vector<std::vector<double>>& mat);
    };

} // namespace thermolang::codegen

#endif // THERMOLANG_THRML_CODE_GENERATOR_H