add_library(ThermoLangParser
    Parser.cpp
    SymbolTable.cpp
    Ast.cpp
)

target_include_directories(ThermoLangParser PUBLIC
    # We don't need to add include dir here because the top-level one handles it
)

# The Parser depends on the Lexer library for Token definitions
target_link_libraries(ThermoLangParser PUBLIC ThermoLangLexer)