#ifndef THERMOLANG_LEXER_H
#define THERMOLANG_LEXER_H

#include "thermolang/lexer/Token.h"
#include <string>
#include <unordered_map>

namespace thermolang {

class Lexer {
public:
    explicit Lexer(std::string source);

    Token next_token();

private:
    void skip_whitespace();
    char advance();
    char peek() const;
    char peek_next() const;

    Token make_token(TokenType type) const;
    Token make_token(TokenType type, Token::LiteralValue literal) const;
    Token identifier();
    Token number();
    Token string();

    bool is_at_end() const;
    bool is_alpha(char c) const;
    bool is_digit(char c) const;

    std::string source_;
    size_t start_ = 0;
    size_t current_ = 0;
    SourceLocation location_;

    static const std::unordered_map<std::string, TokenType> keywords_;
};

} // namespace thermolang

#endif // THERMOLANG_LEXER_H