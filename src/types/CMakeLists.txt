# Create library for the Types module
add_library(ThermoLangTypes
    Type.cpp
)

target_include_directories(ThermoLangTypes PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
)

# Link any dependencies
target_link_libraries(ThermoLangTypes PUBLIC
    # No dependencies yet
)