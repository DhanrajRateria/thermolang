#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

// --- Compiler Pipeline Headers ---
#include "thermolang/parser/Parser.h"
#include "thermolang/semantics/SemanticAnalyzer.h"
#include "thermolang/semantics/TypeChecker.h"
#include "thermolang/compiler/IrGenerator.h"

// --- Optimizer Headers ---
#include "thermolang/optimizer/OptimizationManager.h"
#include "thermolang/optimizer/Passes.h"
#include "thermolang/optimizer/ConstantFoldingPass.h"
#include "thermolang/optimizer/DiscreteEBMAnalysisPass.h"
#include "thermolang/optimizer/EnergyFunctionOptimizer.h"
#include "thermolang/optimizer/CircuitTopologyOptimizer.h"
#include "thermolang/optimizer/VarianceTrackingPass.h"
#include "thermolang/optimizer/ThermalSchedulingPass.h"
#include "thermolang/optimizer/GraphColoringPass.h"

// --- Code Generator Headers ---
#include "thermolang/codegen/CodeGenerator.h"
#include "thermolang/codegen/SPUCodeGenerator.h"
#include "thermolang/codegen/SPICECodeGenerator.h"
#include "thermolang/codegen/FPGACodeGenerator.h"
#include "thermolang/codegen/ThrmlCodeGenerator.h"

// The primary compilation pipeline function.
void run(const std::string &source, const std::string &target, bool enable_optimizations, const std::string &base_output_filename)
{
    // === Phase 1: Frontend ===
    thermolang::Lexer lexer(source);
    thermolang::Parser parser(lexer);
    auto ast = parser.parse();
    if (parser.had_error())
    {
        std::cerr << "Compilation failed during parsing." << std::endl;
        return;
    }

    thermolang::SymbolTable symbols;
    thermolang::SemanticAnalyzer semantic_analyzer(symbols);
    if (!semantic_analyzer.analyze(ast))
    {
        std::cerr << "Compilation failed during semantic analysis." << std::endl;
        return;
    }

    thermolang::TypeChecker type_checker(symbols);
    if (!type_checker.check(ast))
    {
        std::cerr << "Compilation failed during type checking." << std::endl;
        return;
    }
    std::cout << "--- Frontend: SUCCESS ---" << std::endl;
    std::cout << "Program is syntactically and semantically valid." << std::endl;
    std::cout << "--------------------------" << std::endl;

    // === Phase 2: IR Generation & Optimization ===
    std::cout << "\n--- IR Generation (Before Optimization) ---" << std::endl;
    thermolang::compiler::IrGenerator ir_gen;
    auto ir_program = ir_gen.generate(ast);

    if (enable_optimizations)
    {
        std::cout << "\n--- Optimizations Enabled ---" << std::endl;
        thermolang::optimizer::OptimizationManager opt_manager;

        // Register all optimizer passes.
        opt_manager.add_pass(std::make_unique<thermolang::optimizer::ConstantFoldingPass>());
        opt_manager.add_pass(std::make_unique<thermolang::optimizer::DiscreteEBMAnalysisPass>());
        opt_manager.add_pass(std::make_unique<thermolang::optimizer::GraphColoringPass>());
        opt_manager.add_pass(std::make_unique<thermolang::optimizer::EnergyFunctionPass>());
        opt_manager.add_pass(std::make_unique<thermolang::optimizer::CircuitTopologyPass>());
        opt_manager.add_pass(std::make_unique<thermolang::optimizer::VarianceTrackingPass>());

        auto thermal_pass = std::make_unique<thermolang::optimizer::ThermalSchedulingPass>();
        thermal_pass->set_program_ir(&ir_program);
        opt_manager.add_pass(std::move(thermal_pass));

        // Run the pipeline on each function.
        for (auto &func_ir : ir_program)
        {
            opt_manager.run(*func_ir);
        }

        std::cout << "\n--- IR Generation (After Optimization) ---\n";
        for (const auto &func_ir : ir_program)
        {
            std::cout << func_ir->toString();
        }
        std::cout << "------------------------------------------\n";
    }
    else
    {
        std::cout << "\n--- Optimizations Disabled ---\n";
    }

    // === Phase 3: Code Generation ===
    std::string generated_code;
    std::string output_filename;
    if (target == "sim")
    {
        std::cout << "\n--- Code Generation (Python Simulation Target) ---\n";
        thermolang::codegen::SimulationCodeGenerator code_gen;
        generated_code = code_gen.generate(ir_program);
        output_filename = base_output_filename + "_sim.py";
    }
    else if (target == "spu")
    {
        std::cout << "\n--- Code Generation (SPU C++ Target) ---\n";
        thermolang::codegen::SPUCodeGenerator code_gen;
        generated_code = code_gen.generate(ir_program);
        output_filename = base_output_filename + "_spu.cpp";
    }
    else if (target == "spice")
    {
        std::cout << "\n--- Code Generation (SPICE Netlist Target) ---\n";
        thermolang::codegen::SPICECodeGenerator code_gen;
        generated_code = code_gen.generate(ir_program);
        output_filename = base_output_filename + ".spice";
    }
    else if (target == "fpga")
    {
        std::cout << "\n--- Code Generation (FPGA Config Target) ---\n";
        thermolang::codegen::FPGACodeGenerator code_gen;
        if (code_gen.generate(ir_program, base_output_filename))
        {
            std::cout << ">>> FPGA output written to " << base_output_filename << "_config.mem and " << base_output_filename << "_schedule.txt" << std::endl;
        }
        else
        {
            std::cerr << "ERROR: Failed to generate FPGA configuration files." << std::endl;
        }
        return; // FPGA generator writes its own files, so we return early.
    }
    else if (target == "thrml")
    {
        std::cout << "\n--- Code Generation (Extropic thrml Target) ---\n";
        thermolang::codegen::ThrmlCodeGenerator code_gen;
        generated_code = code_gen.generate(ir_program);
        output_filename = base_output_filename + "_thrml.py";
    }
    else
    {
        std::cout << "\n--- IR Generation Only (No Backend Target) ---\n";
        std::stringstream ss;
        for (const auto &func_ir : ir_program)
        {
            ss << func_ir->toString();
        }
        generated_code = ss.str();
        output_filename = base_output_filename + ".ir";
    }

    std::cout << generated_code << std::endl;

    std::ofstream out_file(output_filename);
    out_file << generated_code;
    out_file.close();
    std::cout << ">>> Output written to " << output_filename << std::endl;
}

// Helper to get the base name for output files
std::string get_output_base_name(const std::string &path)
{
    size_t last_slash = path.find_last_of("/\\");
    std::string filename = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
    size_t last_dot = filename.find_last_of('.');
    return (last_dot == std::string::npos) ? filename : filename.substr(0, last_dot);
}

// Helper to run a file and pass its contents to the run function.
void run_file(const std::string &path, const std::string &target, bool enable_optimizations)
{
    std::cout << "Compiling: " << path << " for target: " << target << std::endl;
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "ERROR: Could not open file: " << path << std::endl;
        exit(74);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string base_name = get_output_base_name(path);
    // std::string extension = ".ir";
    // if (target == "sim")
    //     extension = "_sim.py";
    // if (target == "spu")
    //     extension = "_spu.cpp";
    // if (target == "spice")
    //     extension = ".spice";

    run(buffer.str(), target, enable_optimizations, base_name);
    std::cout << std::endl;
}

// The main entry point for the thermolangc executable.
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "ThermoLang Compiler v1.0" << std::endl;
        std::cerr << "Usage: thermolangc <file> [--target=sim|spu|spice|fpga|thrml] [--no-opts]" << std::endl;
        return 64;
    }

    std::string filename = argv[1];
    std::string target = "ir_only";
    bool optimizations_enabled = true; // Optimizations are ON by default.

    for (int i = 2; i < argc; ++i)
    {
        std::string arg = argv[i];
        std::string prefix = "--target=";
        if (arg.rfind(prefix, 0) == 0)
        {
            target = arg.substr(prefix.length());
        }
        else if (arg == "--no-opts")
        {
            optimizations_enabled = false;
        }
    }

    run_file(filename, target, optimizations_enabled);

    return 0;
}