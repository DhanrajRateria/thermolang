// include/thermolang/hardware/SPUController.h (MODIFIED)
#ifndef THERMOLANG_SPU_CONTROLLER_H
#define THERMOLANG_SPU_CONTROLLER_H

#include <vector>
#include <string>

namespace thermolang::hardware
{
    // Type aliases remain the same
    using Matrix = std::vector<std::vector<double>>;
    using Vector = std::vector<double>;

    // SPUConfig remains the same
    struct SPUConfig
    {
        Matrix coupling_matrix;
        Vector local_field;
        double initial_temp;
        double cooling_rate;
        int steps;
    };

    // ExecutionResult remains the same
    struct ExecutionResult
    {
        double final_energy;
        std::vector<int> final_state;
        bool converged;
    };

    // SPUController is now a pure virtual interface
    class SPUController
    {
    public:
        virtual ~SPUController() = default;

        // Pure virtual execute method. Any implementation (simulator or real driver) must provide this.
        virtual ExecutionResult execute() = 0;
    };

} // namespace thermolang::hardware

#endif // THERMOLANG_SPU_CONTROLLER_H