#ifndef THERMOLANG_SPU_SIMULATOR_H
#define THERMOLANG_SPU_SIMULATOR_H

#include "thermolang/hardware/SPUController.h"

namespace thermolang::hardware
{
    // A concrete implementation of the SPUController interface that runs
    // a Metropolis-Hastings simulation of the annealing process.
    class SPUSimulator : public SPUController
    {
    public:
        // Constructor takes the complete problem definition.
        explicit SPUSimulator(SPUConfig config);

        // The primary entry point to run the annealing process.
        ExecutionResult execute() override;

    private:
        // Calculates the total energy of a given spin configuration.
        double calculate_energy(const std::vector<int> &spins) const;

        // The configuration for the current problem.
        SPUConfig config_;
    };
} // namespace thermolang::hardware

#endif // THERMOLANG_SPU_SIMULATOR_H