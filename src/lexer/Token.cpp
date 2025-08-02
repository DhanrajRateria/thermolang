#include "thermolang/lexer/Token.h"
#include <sstream>
#include <vector>
#include <string>

namespace thermolang {

Token::Token(TokenType type, std::string lexeme, LiteralValue literal, SourceLocation location)
    : type_(type), lexeme_(std::move(lexeme)), literal_(std::move(literal)), location_(location) {}

std::string Token::to_string() const {
    std::stringstream ss;
    ss << "Token(" << thermolang::to_string(type_) << ", '" << lexeme_ << "', "
       << "L:" << location_.line << " C:" << location_.column << ")";
    return ss.str();
}

std::string to_string(TokenType type) {
    static const std::vector<std::string> token_strings = {
        "TYPE", "DISTRIBUTION", "FUNCTION", "STOCHASTIC", "ENERGY", "THERMAL", "PARALLEL",
        "IF", "ELSE", "WHILE", "RETURN", "LET", "CONST", "FN",
        "IDENTIFIER", "INTEGER_LITERAL", "FLOAT_LITERAL", "STRING_LITERAL", "BOOL_LITERAL",
        "PLUS", "MINUS", "STAR", "SLASH",
        "EQUAL", "EQUAL_EQUAL", "BANG", "BANG_EQUAL",
        "LESS", "LESS_EQUAL", "GREATER", "GREATER_EQUAL",
        "ARROW", "FAT_ARROW",
        "LPAREN", "RPAREN", "LBRACE", "RBRACE", "LBRACKET", "RBRACKET",
        "COMMA", "COLON", "SEMICOLON", "DOT",
        "AT",
        "ILLEGAL",
        "END_OF_FILE"
    };
    int index = static_cast<int>(type);
    if (index >= 0 && index < token_strings.size()) {
        return token_strings[index];
    }
    return "UNKNOWN";
}

} // namespace thermolang