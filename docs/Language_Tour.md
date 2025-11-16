# A Tour of ThermoLang

Welcome to ThermoLang! This document provides a high-level tour of the language's core concepts and philosophy. For a formal definition, please see the [Language Specification](01_Language_Specification.md).

## Philosophy

ThermoLang is designed to map high-level descriptions of physical and probabilistic problems directly onto hardware that computes using thermodynamics. Its syntax is built around three core concepts: **Energy**, **Stochastics**, and **Thermal Dynamics**.

---

## 1. The `energy` Keyword: Defining the Problem

The most fundamental concept is the **energy function**. In physics and optimization, many problems can be framed as finding the state of a system that minimizes an energy value.

The `energy` keyword flags a function as a physical objective function for the compiler to analyze and optimize.

```thermo
// An energy function MUST take variables and return a single 'float'.
energy fn quadratic_bowl(x: float, y: float) -> float {
    // This defines a simple landscape. The goal of a thermodynamic
    // computer is to find the values of x and y that make this
    // return value as small as possible (in this case, x=0, y=0).
    return x*x + y*y;
}
```
The compiler's `DiscreteEBMAnalysisPass` is specifically designed to analyze the mathematical structure of these functions and convert them into an optimized hardware representation.

---

## 2. The `stochastic` Keyword: Working with Probability

Thermodynamic computers are inherently probabilistic. The `stochastic` keyword is used for functions that define or operate on probability distributions.

```thermo
// The standard library provides built-in stochastic primitives.
stochastic fn sample_gaussian(mean: float, variance: float) -> distribution<float>;
stochastic fn sample_bernoulli(probability: float) -> bool;

// You can also define your own sampling procedures.
stochastic fn biased_coin_flip(bias: float) -> bool {
    // sample_uniform is another built-in.
    if (sample_uniform(0.0, 1.0) < bias) {
        return true;
    } else {
        return false;
    }
}
```

## 3. The thermal Keyword and thermal_anneal: Solving the Problem
Once you have an energy function, you need a way to tell the computer to find its minimum. This is done through thermal processes.
The thermal_anneal Function
This is the primary "solver" function in ThermoLang. It instructs the hardware or simulator to perform a thermal annealing process to find the ground state of an energy function.

```thermo
energy fn ising_model(...) -> float { ... }

fn main() -> void {
    // Define the annealing parameters.
    let initial_temp = 10.0;
    let cooling_rate = 0.95;
    let steps = 5000;

    // This single call is the core of the program. It tells the system:
    // "Find the minimum of ising_model by annealing according to this schedule."
    let solution = thermal_anneal(ising_model, initial_temp, cooling_rate, steps);
}
```
The compiler's ThermalSchedulingPass can automatically optimize these schedule parameters based on the complexity of the energy function.

## 4. The thermal Block
For more fine-grained control, the thermal block allows you to specify a sequence of operations that should occur within a specific thermal context. This is an advanced feature for designing custom thermodynamic cycles.
```thermo
thermal {
    // Operations inside this block are assumed to happen at a
    // specific, controlled temperature.
    let temp = 1.5;
    let state1 = thermal_step(initial_state, temp);
    let state2 = thermal_step(state1, temp);
}
```
Putting It All Together: A Complete Program
This example combines all the concepts to solve a MAX-CUT problem.
```thermo
// 1. Define the problem as an energy function.
// We want to find the spin assignments (s0, s1, ...) that minimize this.
energy fn max_cut_k4(s0: float, s1: float, s2: float, s3: float) -> float {
    let J = -1.0; // Anti-ferromagnetic coupling
    return J*s0*s1 + J*s0*s2 + J*s0*s3 + J*s1*s2 + J*s1*s3 + J*s2*s3;
}

// 2. The main function orchestrates the solution.
fn main() -> void {
    // 3. Use the thermal_anneal solver.
    let optimal_partition = thermal_anneal(max_cut_k4, 10.0, 0.95, 2000);
    
    // The `optimal_partition` variable will hold the resulting spin configuration
    // that the annealer found.
}
```
The ThermoLang compiler takes this high-level description, recognizes the max_cut_k4 function as a discrete EBM, optimizes it into a compact hardware representation, and generates the code for the specified backend to execute the annealing process.