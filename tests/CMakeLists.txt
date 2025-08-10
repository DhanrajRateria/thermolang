# --- Google Test ---
# Download and configure Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)

set(EXAMPLES_DIR "${CMAKE_SOURCE_DIR}/examples")
add_definitions(-DEXAMPLES_DIR="${EXAMPLES_DIR}")

# --- Test Executable ---
add_executable(run_tests 
    main_test.cpp
    type_system_test.cpp
    ir_generator_test.cpp
    optimizer_test.cpp
    domain_specific_test.cpp
    control_flow_test.cpp
    e2e_test.cpp
)

target_compile_definitions(run_tests PRIVATE
    "COMPILER_EXECUTABLE_PATH=\"${CMAKE_BINARY_DIR}/thermolangc\""
)

# Link the test executable against Google Test and our compiler libraries
target_link_libraries(run_tests PRIVATE
    gtest_main
    ThermoLangOptimizer
    ThermoLangParser
    ThermoLangLexer
    ThermoLangSemantics
    ThermoLangTypes
    ThermoLangIR
    ThermoLangCompiler
)

# --- Add tests to CTest ---
include(GoogleTest)

# Method 1: Use discover_tests with explicit discovery mode
gtest_discover_tests(run_tests 
    DISCOVERY_MODE PRE_TEST
    PROPERTIES TIMEOUT 10)