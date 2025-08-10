add_library(ThermoLangHardware
    SPUController.cpp
)

target_include_directories(ThermoLangHardware PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)