# --- IR Library ---
add_library(ThermoLangIR
    ThermoIR.cpp
)

# Publicly expose the include directory for this library
target_include_directories(ThermoLangIR PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)

# The IR library may depend on Types for certain definitions
target_link_libraries(ThermoLangIR PUBLIC ThermoLangTypes)