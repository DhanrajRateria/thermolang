#ifndef THERMOLANG_TOKEN_H
#define THERMOLANG_TOKEN_H

#include <string>
#include <variant>

namespace thermolang {

// Enum for all possible token types in ThermoLang
enum class TokenType {
    // Keywords
    TYPE, DISTRIBUTION, FUNCTION, STOCHASTIC, ENERGY, THERMAL, PARALLEL,
    IF, ELSE, WHILE, RETURN, LET, CONST, FN,

    CIRCUIT,
    // Identifiers and Literals
    IDENTIFIER,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    BOOL_LITERAL,

    // Operators
    PLUS, MINUS, STAR, SLASH,
    EQUAL, EQUAL_EQUAL, BANG, BANG_EQUAL,
    LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
    ARROW, FAT_ARROW, // ->, =>

    // Punctuation
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, COLON, SEMICOLON, DOT,

    // Special
    AT, // @ for annotations
    ILLEGAL,
    END_OF_FILE,
};

// A struct to hold location info for better error messages
struct SourceLocation {
    int line = 1;
    int column = 0;
};

// The Token class
class Token {
public:
    using LiteralValue = std::variant<std::monostate, int64_t, double, std::string, bool>;

    Token(TokenType type, std::string lexeme, LiteralValue literal, SourceLocation location);

    TokenType get_type() const { return type_; }
    const std::string& get_lexeme() const { return lexeme_; }
    const LiteralValue& get_literal() const { return literal_; }
    const SourceLocation& get_location() const { return location_; }

    std::string to_string() const;

private:
    TokenType type_;
    std::string lexeme_;
    LiteralValue literal_;
    SourceLocation location_;
};

// Function to convert TokenType to string for debugging
std::string to_string(TokenType type);

} // namespace thermolang

#endif // THERMOLANG_TOKEN_H