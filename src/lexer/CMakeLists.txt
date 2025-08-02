# --- Lexer Library ---
add_library(ThermoLangLexer
    Token.cpp
    Lexer.cpp
)

# Publicly expose the include directory for this library
target_include_directories(ThermoLangLexer PUBLIC
    "${PROJECT_SOURCE_DIR}/include" # For Token.h
)