# --- Optimizer Library ---
add_library(ThermoLangOptimizer
    OptimizationManager.cpp
    ConstantFoldingPass.cpp
    EnergyFunctionOptimizer.cpp
    CircuitTopologyOptimizer.cpp
    VarianceTrackingPass.cpp
    IsingModelPass.cpp
    ThermalSchedulingPass.cpp
    ParallelizationPass.cpp
)

# Publicly expose the include directory for this library
target_include_directories(ThermoLangOptimizer PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)

# The optimizer operates on the IR
target_link_libraries(ThermoLangOptimizer PUBLIC ThermoLangIR)