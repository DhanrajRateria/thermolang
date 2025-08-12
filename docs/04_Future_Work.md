# Future Work and Enhancements

With the core v1.0 toolchain complete, several exciting avenues for future research and development are now possible.

## 1. Thermox Simulator Integration

The current Python backend (`--target=sim`) generates a self-contained NumPy-based simulation. A valuable next step would be to create a new backend, `--target=thermox`, that generates code to interface directly with the Thermox open-source library.

### Implementation Plan
1.  **New Code Generator:** Create `ThermoxCodeGenerator.cpp`.
2.  **IR-to-API Mapping:** This generator would map the `IsingHamiltonianInstr` in the ThermoIR to the appropriate `thermox.RLCNetwork` or `thermox.IsingModel` setup calls.
3.  **Dependency Management:** The benchmarking script would need to be updated to `pip install thermox` and import it when running this new backend.

### Benefits
*   **Standardization:** Allows ThermoLang programs to be tested against a standard, community-vetted simulator.
*   **Richer Physics:** Could leverage more complex physical models available in `thermox` that are not present in our simple NumPy simulator (e.g., detailed RLC dynamics).

## 2. Advanced Hardware Optimizations

*   **Topology-Aware Mapping:** Enhance the `CircuitTopologyPass` to intelligently map a problem's connectivity graph onto the fixed 2D grid of the FPGA, minimizing communication overhead.
*   **Thermal Schedule Optimization:** Improve the `ThermalSchedulingPass` with more sophisticated heuristics based on the problem's eigenvalue spread or other graph-theoretic properties.

## 3. Support for Higher-Order Models

Extend the language and compiler to support Polynomial Unconstrained Binary Optimization (PUBO) models, which involve interactions between more than two spins (e.g., `s1*s2*s3`). This would require significant extensions to the `IsingModelPass` and hardware architecture.