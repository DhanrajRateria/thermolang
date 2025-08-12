# ThermoLang: A Complete Language & Compiler System for Thermodynamic Computing

> **Democratizing thermodynamic computing through high-level programming abstractions**

ThermoLang is the first complete domain-specific language (DSL) and compiler toolchain designed specifically for thermodynamic computing hardware. It bridges the critical gap between high-level probabilistic algorithms and low-level stochastic hardware implementations, enabling researchers to focus on algorithm design rather than circuit-level details.

## 🎯 Vision & Mission

Our mission is to accelerate thermodynamic computing research by providing essential software infrastructure that abstracts away hardware complexity. ThermoLang democratizes access to stochastic computing by enabling researchers to express complex energy models and annealing schedules in an intuitive, high-level language.

**Impact**: We estimate ThermoLang will accelerate research in the field by 3-5 years by removing the barriers between algorithmic innovation and hardware implementation.

## 🚀 Project Status: v1.0 Complete

**✅ Full Toolchain Implemented**

ThermoLang v1.0 represents a complete source-to-hardware compilation pipeline. The system successfully compiles high-level `.thermo` programs to multiple target backends, from pure software simulation to synthesizable hardware descriptions.

### Key Achievements
- ✅ Complete language specification with formal grammar
- ✅ Full compiler frontend (lexer, parser, semantic analyzer)
- ✅ Multi-backend code generation
- ✅ End-to-end validation framework
- ✅ Comprehensive benchmarking suite
- ✅ Production-ready toolchain

## 🛠 The ThermoLang Toolchain

The core compiler, `thermolangc`, supports four distinct compilation targets, each optimized for different use cases:

### 1. Python Simulation (`--target=sim`)
**Purpose**: Rapid prototyping and algorithm validation
- Generates self-contained Python scripts using NumPy
- Ideal for research and development workflows
- Fast iteration cycles for algorithm refinement

```bash
./build/thermolangc my_program.thermo --target=sim
```

### 2. C++ SPU Simulation (`--target=spu`)
**Purpose**: High-performance detailed simulation
- Generates optimized C++ code with hardware-accurate SPU simulation
- Detailed modeling of Stochastic Processing Unit behavior
- Performance-critical applications and validation

```bash
./build/thermolangc my_program.thermo --target=spu
```

### 3. SPICE Netlist (`--target=spice`)
**Purpose**: Physical circuit simulation and analysis
- Generates `.spice` netlists for analog circuit simulators
- Compatible with LTspice, ngspice, and other SPICE tools
- Enables circuit-level analysis and optimization

```bash
./build/thermolangc my_program.thermo --target=spice
```

### 4. FPGA Configuration (`--target=fpga`)
**Purpose**: Hardware accelerator deployment
- Generates memory initialization files (`.mem`, `.txt`)
- Contains optimized J and h matrices plus annealing schedules
- Ready for direct hardware deployment

```bash
./build/thermolangc my_program.thermo --target=fpga
```

## 📋 Prerequisites

### Required Dependencies
- **C++17 Compiler**: GCC 9+ or Clang 10+
- **CMake**: Version 3.14 or higher
- **Python**: 3.8+ with NumPy for simulation backends
- **Git**: For version control and dependency management

### Optional Dependencies
- **Xilinx Vivado**: Required for FPGA synthesis and implementation
- **SPICE Simulator**: LTspice or ngspice for circuit simulation
- **Google Test**: Automatically downloaded for testing framework

## 🔧 Installation & Setup

### Quick Start

```bash
# Clone the repository
git clone https://github.com/DhanrajRateria/thermolang.git
cd thermolang

# Configure the build system
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build the complete toolchain
cmake --build build --parallel

# Verify installation
./build/thermolangc --version
```

### Development Build

```bash
# Configure for development (includes debugging symbols and tests)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Build with verbose output
cmake --build build --parallel --verbose

# Run the test suite
cd build && ctest --parallel --verbose
```

## 🧪 Validation & Benchmarking

### End-to-End Validation
Run our comprehensive benchmark suite to validate all backends:

```bash
# Execute full benchmark suite
python3 benchmarks/run_benchmarks.py

# Run specific backend benchmarks
python3 benchmarks/run_benchmarks.py --target=sim
python3 benchmarks/run_benchmarks.py --target=spu --verbose
```

### Example Usage

```bash
# Compile a simple Ising model
./build/thermolangc examples/ising_2d.thermo --target=sim --output=ising_sim.py

# Generate FPGA configuration for quantum annealing
./build/thermolangc examples/qaoa.thermo --target=fpga --output-dir=fpga_configs/

# Create SPICE netlist for circuit analysis
./build/thermolangc examples/boltzmann_machine.thermo --target=spice --optimize=2
```

## 📚 Language Overview

### Core Language Features

```thermolang
// Define energy function for 2D Ising model
model IsingModel {
    lattice: Grid2D<8, 8>
    coupling: Real = -1.0
    field: Real = 0.5
    
    energy(spins: SpinArray) -> Real {
        return -coupling * sum_neighbors(spins) - field * sum(spins)
    }
}

// Specify annealing schedule
schedule LinearAnnealing {
    temperature: 10.0 -> 0.1
    steps: 1000
    method: "exponential"
}

// Main computation
solve IsingModel with LinearAnnealing {
    initial_state: random_spins()
    measurements: ["energy", "magnetization"]
    output_format: "json"
}
```

### Type System Highlights
- **Stochastic Types**: Native support for probability distributions
- **Tensor Operations**: Built-in multidimensional array operations
- **Energy Expressions**: Domain-specific constructs for Hamiltonians
- **Schedule Types**: Temporal evolution specifications

## 🔬 Research Contributions

This work advances the state-of-the-art in non-von Neumann computing through four key contributions:

### 1. Novel Domain-Specific Language
- **First-of-its-kind DSL** for thermodynamic computing
- **Intuitive syntax** for energy-based and probabilistic algorithms
- **Strong type system** preventing common stochastic computing errors

### 2. Probabilistic-to-Physical Compilation
- **Pioneering compiler** that translates abstract energy functions into physical implementations
- **Multi-level optimization** from algorithmic to circuit level
- **Verified correctness** across all compilation stages

### 3. Complete End-to-End Toolchain
- **Seamless workflow** from algorithm specification to hardware deployment
- **Industrial-strength infrastructure** suitable for production use
- **Extensive documentation** and examples for rapid adoption

### 4. Multi-Backend Validation Framework
- **Cross-platform verification** ensuring correctness across all targets
- **Performance benchmarking** suite with detailed analytics
- **Regression testing** to maintain quality across development cycles

## 📖 Documentation

Comprehensive documentation is available in the `/docs` directory:

| Document | Description |
|----------|-------------|
| [Language Specification](docs/01_Language_Specification.md) | Complete formal grammar and syntax reference |
| [Type System Guide](docs/02_Type_System.md) | Detailed stochastic type system documentation |
| [Compiler Architecture](docs/compiler_architecture.md) | Internal design and implementation details |
| [Backend Guide](docs/backends.md) | Target-specific compilation options and optimizations |
| [Standard Library](docs/03_Standard_Library.md) | Built-in functions and types API reference |
| [Tutorial Series](docs/tutorials/) | Step-by-step learning materials |
| [Examples Collection](examples/) | Real-world use cases and sample programs |

## 🤝 Contributing

We enthusiastically welcome contributions from the research community! Whether you're interested in language design, compiler optimization, or applications development, there are many ways to get involved.

### Getting Started
1. **Read our [Contributing Guide](CONTRIBUTING.md)** for detailed instructions
2. **Check the [Issues](https://github.com/DhanrajRateria/thermolang/issues)** for open tasks
3. **Review the [Architecture Documentation](docs/compiler_architecture.md)** to understand the codebase
4. **Join our [Discussions](https://github.com/DhanrajRateria/thermolang/discussions)** for community support

### Development Areas
- **Language Extensions**: New syntax features and type system enhancements
- **Backend Development**: Additional compilation targets and optimizations
- **Standard Library**: Built-in algorithms and utility functions
- **Tooling**: IDE integration, debuggers, and profiling tools
- **Documentation**: Tutorials, examples, and API improvements

## 📄 License

ThermoLang is released under the [MIT License](LICENSE), encouraging both academic research and commercial applications.

---

**ThermoLang**: Enabling the future of probabilistic and thermodynamic computing through accessible, high-level programming abstractions.