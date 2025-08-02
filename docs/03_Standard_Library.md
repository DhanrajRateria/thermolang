# ThermoLang Standard Library API v0.1

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

## 2. Thermal Functions

These functions interact with the thermodynamic properties of the hardware.

---
`thermal fn anneal(energy_func: E, schedule: CoolingSchedule) -> Config`
*   **Description:** Performs simulated thermal annealing to find the minimum energy configuration of a system.
*   `energy_func`: An `energy` function that defines the problem landscape.
*   `schedule`: A configuration for the cooling process.
*   `Config`: The resulting state of variables that minimizes the energy.

---

## 3. Hardware Annotation (Future Feature)

Annotations will provide hints to the compiler for hardware-specific optimizations.

```thermolang
@hardware(spu_count=16, coupling_strength=0.7)
energy fn solve_my_problem(...) -> float { ... }
```