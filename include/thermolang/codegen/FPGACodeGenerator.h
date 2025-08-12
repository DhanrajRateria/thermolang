#ifndef THERMOLANG_FPGA_CODE_GENERATOR_H
#define THERMOLANG_FPGA_CODE_GENERATOR_H

#include "thermolang/codegen/CodeGenerator.h"
#include <string>

namespace thermolang::codegen
{
    // Generates configuration files (.mem, .txt) for the FPGA hardware target.
    // This generator produces multiple files, so its 'generate' method works
    // by writing directly to disk instead of returning a single string.
    class FPGACodeGenerator
    {
    public:
        // The main generation function.
        // It takes the IR and a base filename and writes out multiple config files.
        // Returns true on success, false on failure.
        bool generate(const std::vector<std::unique_ptr<ir::FunctionIR>> &program, const std::string& base_filename);

    private:
        // Converts a double-precision float to a 16-bit signed fixed-point representation.
        int16_t double_to_fixed_point(double value, int fractional_bits);
    };

} // namespace thermolang::codegen

#endif // THERMOLANG_FPGA_CODE_GENERATOR_H