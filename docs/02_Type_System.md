# ThermoLang Type System v0.1

This document describes the type system of ThermoLang, including its novel features for handling stochastic computations.

## 1. Primitive Types
*   `int`: 64-bit signed integer.
*   `float`: 64-bit floating-point number.
*   `bool`: Boolean value (`true` or `false`).
*   `string`: UTF-8 encoded string.

## 2. Stochastic Types

Stochastic types are the core innovation of ThermoLang. They represent not a single value, but a probability distribution over a set of values.

### `distribution<T, P...>`
The fundamental generic type for all probability distributions.
*   `T`: The underlying value type of the distribution (e.g., `float`, `int`).
*   `P...`: A variadic list of parameter types that define the distribution's shape (e.g., variance, mean).

### Type Aliases for Common Distributions
For convenience, the standard library provides aliases for common distributions:

```thermolang
// A Gaussian (Normal) distribution over floating-point numbers.
// Defined by its mean and variance.

type Gaussian = distribution<float, mean: float, variance: float>;

// A Uniform distribution over a range of floats.

type Uniform = distribution<float, low: float, high: float>;

// A Bernoulli distribution over boolean values.

type Bernoulli = distribution<bool, p: float>;
```
Use code with caution.

## 3. Type Checking Rules
1. Deterministic Operations: Standard arithmetic and logical operations are only defined for primitive types. Applying them to a distribution type is a compile-time error.

    ```thermolang
    let x: Gaussian = sample_gaussian(0.0, 1.0);
    let y = x + 5.0; // COMPILE ERROR: Cannot add 'Gaussian' and 'float'.
    ```
    Use code with caution.

2. Stochastic Operations: Special functions (sample, minimize, anneal) are required to operate on or produce distribution types.

3. Variance Tracking (Future Goal): The compiler will eventually track the propagation of uncertainty (variance) through computations. For Month 1, we will focus on correct type identification.

4. Energy Functions: An energy function is a special function type that maps a configuration of variables to a scalar energy value (float). They are the bridge between probabilistic models and the underlying physics.
E: (T_vars) -> float

## 4. Energy Function Typing

An `energy` function is a special function type with specific constraints to be considered valid for use in thermodynamic operations like `thermal_anneal`.

1.  **Return Type:** An energy function **must** return a `float`. This scalar value represents the total energy of the system for a given configuration.
2.  **Parameters:** The parameters of an energy function represent the variables of the system (e.g., spins in an Ising model). These parameters should be of a type that can be sampled or manipulated by the hardware, typically `bool` or `float`.
3.  **Purity:** An energy function should be "pure" in the sense that for the same inputs, it always produces the same output. It should not depend on external state or have side effects.

## 5. New Built-in Type: `Configuration`

The `Configuration` type is a special struct-like type returned by optimization functions like `thermal_anneal`. It acts as a container for the resulting state of the variables that minimized the energy function.