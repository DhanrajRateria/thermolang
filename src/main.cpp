#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "thermolang/parser/Parser.h"
#include "thermolang/semantics/SemanticAnalyzer.h"
#include "thermolang/semantics/TypeChecker.h"

void run(const std::string &source)
{
    // Phase 1: Lexing
    thermolang::Lexer lexer(source);

    // Phase 2: Parsing
    thermolang::Parser parser(lexer);
    auto ast = parser.parse();
    bool parser_had_error = parser.had_error();

    // Phase 3: Semantic Analysis (only if parsing succeeded)
    bool semantic_had_error = false;
    bool type_had_error = false;

    if (!parser_had_error && !ast.empty())
    {
        try
        {
            // Create a shared symbol table
            thermolang::SymbolTable symbols;

            // First do scope checking
            thermolang::SemanticAnalyzer analyzer(symbols); // Pass the symbol table
            semantic_had_error = !analyzer.analyze(ast);

            // Then do type checking if scope checking succeeded
            if (!semantic_had_error)
            {
                thermolang::TypeChecker type_checker(symbols); // Pass the same symbol table
                type_had_error = !type_checker.check(ast);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error during semantic analysis: " << e.what() << std::endl;
            semantic_had_error = true;
        }
        catch (...)
        {
            std::cerr << "Unknown error during semantic analysis" << std::endl;
            semantic_had_error = true;
        }
    }

    std::cout << "--- Compilation Result ---" << std::endl;
    if (parser_had_error || semantic_had_error || type_had_error)
    {
        std::cout << "Status: FAILED. Program contains errors." << std::endl;
    }
    else
    {
        std::cout << "Status: SUCCESS. Program is syntactically and semantically valid." << std::endl;
    }
    std::cout << "--------------------------" << std::endl;
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
    std::cout << std::endl; // Add a newline for clean output
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "ThermoLang Compiler v0.1 - Frontend Complete" << std::endl;
        std::cout << "Usage: thermolangc <file1> <file2> ..." << std::endl;
        return 64;
    }

    for (int i = 1; i < argc; ++i)
    {
        run_file(argv[i]);
    }

    return 0;
}