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

void run(const std::string &source)
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

    thermolang::optimizer::OptimizationManager opt_manager;
    opt_manager.add_pass(std::make_unique<thermolang::optimizer::ConstantFoldingPass>());

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
}

void run_file(const std::string &path)
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
    run(buffer.str());
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "ThermoLang Compiler v0.2 - IR Generation" << std::endl;
        std::cout << "Usage: thermolangc <file>" << std::endl;
        return 64;
    }

    // For now, let's just run one file
    run_file(argv[1]);

    return 0;
}