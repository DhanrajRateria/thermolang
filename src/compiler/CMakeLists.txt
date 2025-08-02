add_library(ThermoLangCompiler
    IrGenerator.cpp
)

# Publicly expose the include directory for this library
target_include_directories(ThermoLangCompiler PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)

# Link dependencies
target_link_libraries(ThermoLangCompiler PUBLIC
    ThermoLangIR
    ThermoLangSemantics
)