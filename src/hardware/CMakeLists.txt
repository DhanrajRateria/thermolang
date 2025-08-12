add_library(ThermoLangHardware
    SPUSimulator.cpp
)

target_include_directories(ThermoLangHardware PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)