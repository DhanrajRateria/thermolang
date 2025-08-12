# ThermoLang Standard Library API v0.2

This document outlines the core functions and types available in the ThermoLang standard library.

## 1. Stochastic Functions

These are the primary functions for working with probability distributions.

---
`stochastic fn sample(dist: D) -> T`
*   **Description:** Draws a single sample from the given distribution.
*   `D`: A type that conforms to `distribution<T, ...>`.
*   `T`: The underlying type of the distribution.
*   **Example:** `let x: float = sample(my_gaussian);`

---
`stochastic fn sample_gaussian(mean: float, variance: float) -> Gaussian`
*   **Description:** Creates and returns a Gaussian distribution object. This does not sample from it.
*   **Returns:** A `Gaussian` type representing the distribution.

---

## 2. Thermodynamic Functions

These functions interact with the thermodynamic properties of the hardware or simulator.

---
`thermal fn anneal(energy_func: E, initial_temp: float, cooling_rate: float, steps: int) -> Configuration`
*   **Description:** Performs simulated thermal annealing to find the minimum energy configuration of a system defined by `energy_func`. This is a blocking call that executes the full annealing schedule.
*   `energy_func`: An `energy` function that defines the problem landscape. The function signature determines the variables to be optimized.
*   `initial_temp`: The starting temperature for the annealing process.
*   `cooling_rate`: The multiplicative factor to reduce the temperature at each step (e.g., 0.95).
*   `steps`: The total number of annealing steps to perform.
*   `Config`: Returns a `Configuration` object containing the state of the variables that minimized the energy.

---

## 3. Utility Functions

---
`fn print(value: T) -> void`
*   **Description:** A utility function to print values during simulation. The compiler will often ignore this for hardware targets.
*   `T`: Any primitive type (`int`, `float`, `bool`, `string`).
---

## 4. Hardware Annotation (Future Feature)

Annotations will provide hints to the compiler for hardware-specific optimizations.

```thermolang
@hardware(spu_count=16, coupling_strength=0.7)
energy fn solve_my_problem(...) -> float { ... }
```