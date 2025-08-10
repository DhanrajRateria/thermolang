add_library(ThermoLangCodeGen
    CodeGenerator.cpp
    SimulationCodeGenerator.cpp
    SPUCodeGenerator.cpp
)

target_include_directories(ThermoLangCodeGen PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(ThermoLangCodeGen PUBLIC
    ThermoLangIR
)