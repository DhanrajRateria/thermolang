#ifndef THERMOLANG_THERMAL_SCHEDULING_OPTIMIZER_H
#define THERMOLANG_THERMAL_SCHEDULING_OPTIMIZER_H

#include "thermolang/optimizer/Passes.h"
#include <vector>
#include <memory>

namespace thermolang::optimizer
{

    class ThermalSchedulingPass : public IRPass
    {
    public:
        bool run(ir::FunctionIR &function_ir) override;

    private:
        // Optimize cooling schedules for convergence
        bool optimize_cooling_schedule(ir::FunctionIR &function_ir);

        // Coordinate temperature across distributed SPUs
        bool coordinate_distributed_temperature(ir::FunctionIR &function_ir);

        // Generate adaptive temperature control
        bool generate_adaptive_control(ir::FunctionIR &function_ir);

        // Helper methods
        bool is_annealing_schedule_optimal(const std::vector<double> &schedule);
        std::vector<double> generate_optimal_schedule(double initial_temp,
                                                      double final_temp,
                                                      int steps);
    };

} // namespace thermolang::optimizer

#endif // THERMOLANG_THERMAL_SCHEDULING_OPTIMIZER_H