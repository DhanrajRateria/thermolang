#ifndef THERMOLANG_SPU_CONTROLLER_H
#define THERMOLANG_SPU_CONTROLLER_H

#include <vector>
#include <string>

namespace thermolang::hardware
{

    // Type alias for clarity in the API
    using Matrix = std::vector<std::vector<double>>;
    using Vector = std::vector<double>;

    // Configuration structure for an SPU annealing run.
    // This is what the ThermoLang compiler will generate.
    struct SPUConfig
    {
        Matrix coupling_matrix; // J matrix for the Ising model
        Vector local_field;     // h vector for the local magnetic field
        double initial_temp;
        double cooling_rate;
        int steps;
    };

    // Represents the final result from an SPU execution.
    struct ExecutionResult
    {
        double final_energy;
        std::vector<int> final_state; // Vector of spins {-1, 1}
        bool converged;
    };

    // Interface for controlling the Stochastic Processing Unit (SPU).
    class SPUController
    {
    public:
        // Constructor takes the complete problem definition.
        explicit SPUController(SPUConfig config);

        // The primary entry point to run the annealing process.
        ExecutionResult execute();

    private:
        // Calculates the total energy of a given spin configuration.
        double calculate_energy(const std::vector<int> &spins) const;

        // The configuration for the current problem.
        SPUConfig config_;
    };

} // namespace thermolang::hardware

#endif // THERMOLANG_SPU_CONTROLLER_H