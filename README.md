# ThermoLang: A Language & Compiler for Thermodynamic Computing

Welcome to the official repository for ThermoLang, the first domain-specific language and compiler system designed for thermodynamic computing hardware.

## Vision

Our mission is to democratize thermodynamic computing research by providing the critical software infrastructure needed to program stochastic hardware. By abstracting away low-level circuit parameters, ThermoLang enables researchers to focus on high-level probabilistic algorithms, accelerating the field by an estimated 3-5 years.

## Project Status: Month 1 (In Progress)

This project is currently in its first month of development. The primary goals for this phase are:

1.  **Formal Language Specification:** Defining the syntax, semantics, and type system of ThermoLang.
2.  **Compiler Frontend:** Building the lexer, parser, and semantic analyzer to process ThermoLang source code and produce a validated Abstract Syntax Tree (AST).
3.  **Project Infrastructure:** Setting up a robust, scalable, and well-documented C++ project with CMake and Google Test.

## Getting Started

### Prerequisites
*   C++17 Compiler (GCC 9+ or Clang 10+)
*   CMake (3.14+)
*   Git

### Build Instructions
```bash
# Clone the repository
git clone https://github.com/your-org/thermolang.git
cd thermolang

# Configure the project
cmake -B build

# Build the compiler
cmake --build build

# Run the compiler
./build/thermolangc ./examples/hello_world.thermo
```

## Documentation
All technical documentation can be found in the /docs directory:
- Language Specification: The formal grammar and syntax of ThermoLang.
- Type System: A detailed description of the stochastic type system.
- Standard Library: The API for built-in functions and types.


## Contributing
We welcome contributions from the community! Please read our CONTRIBUTING.md to get started.