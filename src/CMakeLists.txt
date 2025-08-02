# Add subdirectories for each compiler module
add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(semantics)
add_subdirectory(types)

# --- Main Compiler Executable ---
add_executable(thermolangc main.cpp)

# Link modules to the main executable
target_link_libraries(thermolangc PRIVATE
    ThermoLangSemantics
    ThermoLangParser
    ThermoLangLexer
    ThermoLangTypes
)

# Set output directory for the executable
set_target_properties(thermolangc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)