cmake_minimum_required(VERSION 3.14)
project(ThermoLang CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# IMPORTANT: Move enable_testing before any subdirectory that uses it
enable_testing()

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type" FORCE)
endif()

# --- Project-wide Include Directories ---
include_directories(
    "${PROJECT_SOURCE_DIR}/include"
)

# --- Subdirectories ---
add_subdirectory(src)
add_subdirectory(tests)