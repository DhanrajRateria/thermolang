#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "thermolang/parser/Parser.h"
#include "thermolang/semantics/SemanticAnalyzer.h"
#include "thermolang/semantics/TypeChecker.h"
#include "thermolang/compiler/IrGenerator.h"
#include "thermolang/optimizer/OptimizationManager.h"
#include "thermolang/optimizer/Passes.h"
#include "thermolang/optimizer/EnergyFunctionOptimizer.h"
#include "thermolang/optimizer/CircuitTopologyOptimizer.h"
#include "thermolang/optimizer/VarianceTrackingPass.h"
#include "thermolang/optimizer/ConstantFoldingPass.h"
#include "thermolang/optimizer/IsingModelPass.h"
#include "thermolang/codegen/CodeGenerator.h"
#include <string>

void run(const std::string &source, const std::string &target)
{
    // Phase 1: Frontend (Lexing, Parsing, Semantic Analysis)
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
    std::cout << "--------------------------" << std::endl
              << std::endl;

    // Phase 2: IR Generation
    std::cout << "--- IR Generation ---" << std::endl;
    thermolang::compiler::IrGenerator ir_gen;
    auto ir_program = ir_gen.generate(ast);

    for (const auto &func_ir : ir_program)
    {
        std::cout << func_ir->toString();
    }
    std::cout << "---------------------" << std::endl;

    // Set up the optimization manager with all our passes
    thermolang::optimizer::OptimizationManager opt_manager;

    // General optimizations
    opt_manager.add_pass(std::unique_ptr<thermolang::optimizer::IRPass>(
        new thermolang::optimizer::ConstantFoldingPass()));

    // Domain-specific optimizations
    opt_manager.add_pass(std::make_unique<thermolang::optimizer::IsingModelPass>());
    opt_manager.add_pass(std::unique_ptr<thermolang::optimizer::IRPass>(
        new thermolang::optimizer::EnergyFunctionPass()));
    opt_manager.add_pass(std::unique_ptr<thermolang::optimizer::IRPass>(
        new thermolang::optimizer::CircuitTopologyPass()));
    opt_manager.add_pass(std::unique_ptr<thermolang::optimizer::IRPass>(
        new thermolang::optimizer::VarianceTrackingPass()));

    // Run optimizations on each function
    for (auto &func_ir : ir_program)
    {
        opt_manager.run(*func_ir);
    }

    std::cout << "--- IR Generation (After Optimization) ---\n";
    for (const auto &func_ir : ir_program)
    {
        std::cout << func_ir->toString();
    }
    std::cout << "------------------------------------------\n";

    if (target == "sim")
    {
        std::cout << "\n--- Code Generation (Python Simulation) ---\n";
        thermolang::codegen::SimulationCodeGenerator code_gen;
        std::string python_code = code_gen.generate(ir_program);
        std::cout << python_code << std::endl;

        // Optionally, write to file
        std::ofstream out_file("program_sim.py");
        out_file << python_code;
        out_file.close();
        std::cout << ">>> Simulation code written to program_sim.py" << std::endl;
    }
}

void run_file(const std::string &path, const std::string &target)
{
    std::cout << "Compiling: " << path << std::endl;
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "ERROR: Could not open file: " << path << std::endl;
        exit(74);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    run(buffer.str(), target);
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "ThermoLang Compiler v0.3" << std::endl;
        std::cerr << "Usage: thermolangc <file> [--target=sim]" << std::endl;
        return 64;
    }

    std::string filename = argv[1];
    std::string target = "ir_only"; // Default target

    if (argc > 2)
    {
        std::string target_arg = argv[2];
        std::string prefix = "--target=";
        if (target_arg.rfind(prefix, 0) == 0)
        {
            target = target_arg.substr(prefix.length());
        }
    }

    run_file(filename, target);

    return 0;
}
