<div align="center">
  <h1>🔥 ThermoLang</h1>
  <p><strong>A Complete Language & Compiler System for Thermodynamic Computing</strong></p>
  
  <p>
    <a href="#quick-start">Quick Start</a> •
    <a href="#features">Features</a> •
    <a href="#documentation">Documentation</a> •
    <a href="#examples">Examples</a> •
    <a href="#contributing">Contributing</a>
  </p>

  <p>
    <img src="https://img.shields.io/badge/version-1.0-blue.svg" alt="Version">
    <img src="https://img.shields.io/badge/license-MIT-green.svg" alt="License">
    <img src="https://img.shields.io/badge/C++-17-orange.svg" alt="C++17">
  </p>
</div>

---

## 🎯 Vision

ThermoLang democratizes thermodynamic computing by providing a high-level language and robust compiler toolchain that bridges the gap between probabilistic algorithms and physical stochastic hardware. Write once, deploy everywhere—from software simulation to synthesizable hardware.

## 🌟 What is ThermoLang?

ThermoLang is the **first complete, open-source compiler** for thermodynamic computing. It translates high-level programs written in the `.thermo` language into optimized configurations for multiple backends, enabling researchers to focus on **algorithmic innovation** rather than low-level circuit parameter tuning.

### Why ThermoLang?

- **🚀 Accelerate Research**: Spend time on algorithms, not hardware details
- **🔄 Universal Compatibility**: One codebase, multiple execution targets
- **🎓 Educational**: Learn thermodynamic computing principles with immediate feedback
- **🏗️ Production Ready**: Validated through comprehensive benchmarking
- **🤝 Ecosystem Integration**: Native support for Extropic AI's `thrml` library

---

## ✨ Key Features

### High-Level Language
A declarative, domain-specific language for expressing:
- Energy functions and Hamiltonians
- Annealing schedules and cooling protocols
- Probabilistic models and sampling strategies
- Physical constraints and coupling parameters

### Advanced Multi-Stage Compiler
- **Custom IR**: `ThermoIR` intermediate representation optimized for thermodynamic operations
- **Domain-Specific Optimizations**: Automatic recognition of physical models (Discrete EBMs, Ising models, etc.)
- **Type Safety**: Static analysis prevents physical inconsistencies
- **Dead Code Elimination**: Removes unused circuit components

### Multi-Target Backend System

Generate optimized code for every stage of your development pipeline:

| Target | Description | Use Case |
|--------|-------------|----------|
| `thrml` | Python + Extropic's JAX library | **Recommended** for ecosystem alignment and high-fidelity simulation |
| `sim` | NumPy-based Python simulator | Rapid prototyping and algorithm development |
| `spu` | High-performance C++ executable | Production deployments and large-scale experiments |
| `spice` | Analog circuit netlist | Physical circuit analysis and validation |
| `fpga` | Hardware configuration files | Real hardware implementation on FPGAs |

---

## 🚀 Quick Start

### Prerequisites

```bash
# Required
- C++17 compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.14+
- Python 3.8+
- NumPy: pip install numpy

# Recommended
- Extropic's thrml library: pip install thrml
- For SPICE: LTspice, ngspice, or similar
- For FPGA: Xilinx Vivado or Intel Quartus
```

### Installation

```bash
# Clone the repository
git clone https://github.com/DhanrajRateria/thermolang.git
cd thermolang

# Configure with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build the compiler
cmake --build build --parallel

# Verify installation
./build/thermolangc --version
```

### Your First ThermoLang Program

Create `hello_thermo.thermo`:

```thermo
model IsingSpinSystem {
  spins: 4
  coupling: -1.0
  field: 0.5
  
  schedule {
    temperature: 10.0 -> 0.1
    steps: 1000
  }
}
```

Compile and run:

```bash
# Compile to thrml backend
./build/thermolangc hello_thermo.thermo --target=thrml

# Execute the simulation
python3 hello_thermo_thrml.py
```

---

## 📚 Documentation

### Language Reference

The `.thermo` language supports:

- **Variable Declarations**: `var spin: bool[8]`
- **Energy Functions**: `energy = -J * sum(spin[i] * spin[i+1])`
- **Constraints**: `constraint sum(spin) == 4`
- **Schedules**: `anneal temperature from 10 to 0.1 in 1000 steps`
- **Physical Models**: Built-in primitives for common architectures

### Compilation Pipeline

```
┌─────────────┐
│ .thermo     │
│ Source File │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Lexer     │  ← Tokenization
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Parser    │  ← AST Construction
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  ThermoIR   │  ← Intermediate Representation
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ Optimizer   │  ← Domain-Specific Optimizations
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Backends   │  ← Target Code Generation
└─────────────┘
     │
     ├──► thrml (Python + JAX)
     ├──► sim (Python + NumPy)
     ├──► spu (C++)
     ├──► spice (Netlist)
     └──► fpga (Config Files)
```

### Command-Line Interface

```bash
# Basic compilation
thermolangc input.thermo --target=thrml

# With optimizations
thermolangc input.thermo --target=spu -O3

# Generate multiple targets
thermolangc input.thermo --target=sim,thrml,spice

# Verbose output for debugging
thermolangc input.thermo --target=thrml --verbose

# Display IR for inspection
thermolangc input.thermo --emit-ir
```

---

## 💡 Examples


Browse the full example suite:
- `examples/1_language_basic/` - Language fundamentals
- `examples/2_core_concepts/` - The core concepts
- `examples/3_algorithms/` - Common optimization problems
- `examples/4_advanced/` - Physics simulations

---

## 🧪 Validation & Benchmarking

ThermoLang v1.0 includes a comprehensive test suite that validates:
- ✅ Correctness across all backends
- ✅ Consistency with `thrml` reference implementation
- ✅ Performance benchmarks
- ✅ Hardware accuracy

Run the full validation suite:

```bash
# Complete benchmark across all targets
python3 benchmarks/run_benchmarks.py

# Specific backend comparison
python3 benchmarks/compare_backends.py --backends=thrml,sim,spu

# Performance profiling
python3 benchmarks/performance_test.py --target=spu
```

---

## 🏗️ Architecture

### The ThermoLang Ecosystem

```
                    ┌──────────────────┐
                    │  .thermo Source  │
                    └────────┬─────────┘
                             │
                    ┌────────▼─────────┐
                    │  thermolangc     │
                    │    Compiler      │
                    └────────┬─────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
    ┌────▼────┐         ┌────▼────┐        ┌────▼────┐
    │ thrml   │         │   sim   │        │   spu   │
    │ Backend │         │ Backend │        │ Backend │
    └────┬────┘         └────┬────┘        └────┬────┘
         │                   │                   │
    ┌────▼────┐         ┌────▼────┐        ┌────▼────┐
    │  JAX    │         │  NumPy  │        │   C++   │
    │Execution│         │Simulator│        │Simulator│
    └─────────┘         └─────────┘        └─────────┘
         │                   │                   │
         └───────────────────┴───────────────────┘
                             │
                      ┌──────▼──────┐
                      │   Results   │
                      └─────────────┘
```

### Design for the Future

ThermoLang is designed as a **foundational tool** for the emerging thermodynamic computing ecosystem, with first-class support for platforms like **Extropic AI** and extensibility for future hardware innovations.

---

## 🤝 Contributing

We enthusiastically welcome contributions from the research community!

### How to Contribute

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

### Areas for Contribution

- 🐛 Bug fixes and issue reports
- 📝 Documentation improvements
- 🎯 New optimization passes
- 🔧 Additional backend targets
- 📊 Benchmark problems
- 🎓 Tutorial content

See our [Contributing Guide](CONTRIBUTING.md) for detailed guidelines.

---

## 📖 Citation

If you use ThermoLang in your research, please cite:

```bibtex
@software{thermolang2024,
  title = {ThermoLang: A Complete Language and Compiler System for Thermodynamic Computing},
  author = {Rateria, Dhanraj},
  year = {2025},
  version = {1.0},
  url = {https://github.com/DhanrajRateria/thermolang}
}
```

*BibTeX entry will be updated upon arXiv submission.*

---

## 📜 License

ThermoLang is released under the [MIT License](LICENSE).

---

## 🙏 Acknowledgments

- **Extropic AI** for the `thrml` library and ecosystem support
- The thermodynamic computing research community
- All contributors and early adopters

---

## 📬 Contact & Support

- **Issues**: [GitHub Issues](https://github.com/DhanrajRateria/thermolang/issues)
- **Discussions**: [GitHub Discussions](https://github.com/DhanrajRateria/thermolang/discussions)
- **Email**: [maintainer email]

---

<div align="center">
  <p><strong>Built with ❤️ for the thermodynamic computing community</strong></p>
  <p>⭐ Star us on GitHub if ThermoLang helps your research!</p>
</div>