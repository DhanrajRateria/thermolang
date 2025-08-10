# Add subdirectories for each compiler module
add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(semantics)
add_subdirectory(types)
add_subdirectory(ir)
add_subdirectory(compiler)
add_subdirectory(optimizer)
add_subdirectory(codegen)

# --- Main Compiler Executable ---
add_executable(thermolangc main.cpp)

# Link modules to the main executable
target_link_libraries(thermolangc PRIVATE
    ThermoLangCodeGen
    ThermoLangOptimizer
    ThermoLangCompiler
    ThermoLangSemantics
    ThermoLangParser
    ThermoLangLexer
    ThermoLangTypes
    ThermoLangIR
)

# Set output directory for the executable
set_target_properties(thermolangc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)