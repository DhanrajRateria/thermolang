#include "thermolang/hardware/SPUSimulator.h"
#include <random>
#include <cmath>
#include <iostream>
#include <numeric>

namespace thermolang::hardware
{

    SPUSimulator::SPUSimulator(SPUConfig config) : config_(std::move(config)) {}

    double SPUSimulator::calculate_energy(const std::vector<int> &spins) const
    {
        double coupling_energy = 0.0;
        double field_energy = 0.0;
        size_t num_spins = spins.size();

        // Sum of J_ij * s_i * s_j
        for (size_t i = 0; i < num_spins; ++i)
        {
            for (size_t j = i + 1; j < num_spins; ++j)
            {
                coupling_energy += config_.coupling_matrix[i][j] * spins[i] * spins[j];
            }
        }

        // Sum of h_i * s_i
        for (size_t i = 0; i < num_spins; ++i)
        {
            field_energy += config_.local_field[i] * spins[i];
        }

        // Standard Ising Hamiltonian formula
        return -(coupling_energy + field_energy);
    }

    ExecutionResult SPUSimulator::execute()
    {
        std::cout << "[SPU SIM] Starting simulated annealing..." << std::endl;
        size_t num_spins = config_.local_field.size();
        if (num_spins == 0)
        {
            return {0.0, {}, true};
        }

        // --- Setup for Metropolis-Hastings Simulation ---
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<size_t> spin_dist(0, num_spins - 1);
        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

        // 1. Initialize with a random spin state
        std::vector<int> current_spins(num_spins);
        for (size_t i = 0; i < num_spins; ++i)
        {
            current_spins[i] = (prob_dist(rng) < 0.5) ? 1 : -1;
        }

        double current_energy = calculate_energy(current_spins);
        double temperature = config_.initial_temp;

        std::cout << "[SPU SIM] Initial Temp: " << temperature
                  << ", Steps: " << config_.steps
                  << ", Cooling Rate: " << config_.cooling_rate << std::endl;

        // 2. Main annealing loop
        for (int step = 0; step < config_.steps; ++step)
        {
            // Pick a random spin to flip
            size_t spin_to_flip = spin_dist(rng);

            // Calculate the change in energy if this spin were flipped.
            // delta_E = E_new - E_old
            // This is much more efficient than recalculating the whole energy.
            double delta_energy = 0.0;
            double h_term = config_.local_field[spin_to_flip];
            double j_term = 0.0;
            for (size_t i = 0; i < num_spins; ++i)
            {
                if (i == spin_to_flip) continue;
                double j_val = (spin_to_flip < i) ? config_.coupling_matrix[spin_to_flip][i] : config_.coupling_matrix[i][spin_to_flip];
                j_term += j_val * current_spins[i];
            }
            delta_energy = 2.0 * current_spins[spin_to_flip] * (j_term + h_term);

            // Metropolis-Hastings acceptance criterion
            if (delta_energy < 0 || (prob_dist(rng) < std::exp(-delta_energy / (temperature + 1e-9))))
            {
                // Accept the flip
                current_spins[spin_to_flip] *= -1;
                current_energy += delta_energy;
            }

            // Cool the system
            temperature *= config_.cooling_rate;
        }

        std::cout << "[SPU SIM] Annealing complete. Final Energy: " << current_energy << std::endl;

        return {current_energy, current_spins, true};
    }

} // namespace thermolang::hardware