# Create library for the semantics module
add_library(ThermoLangSemantics
    SemanticAnalyzer.cpp
    TypeChecker.cpp  # Add this new file
)

target_include_directories(ThermoLangSemantics PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(ThermoLangSemantics PUBLIC
    ThermoLangParser
    ThermoLangTypes  # Add this new dependency
)