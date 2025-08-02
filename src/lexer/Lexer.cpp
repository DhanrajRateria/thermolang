#include "thermolang/lexer/Lexer.h"

namespace thermolang {

const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    {"type", TokenType::TYPE}, {"distribution", TokenType::DISTRIBUTION},
    {"function", TokenType::FUNCTION}, {"stochastic", TokenType::STOCHASTIC},
    {"energy", TokenType::ENERGY}, {"thermal", TokenType::THERMAL},
    {"parallel", TokenType::PARALLEL}, {"if", TokenType::IF},
    {"else", TokenType::ELSE}, {"while", TokenType::WHILE},
    {"return", TokenType::RETURN}, {"let", TokenType::LET},
    {"const", TokenType::CONST}, {"fn", TokenType::FN},
    {"true", TokenType::BOOL_LITERAL}, {"false", TokenType::BOOL_LITERAL},
    {"circuit", TokenType::CIRCUIT},
};

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

Token Lexer::next_token() {
    skip_whitespace();
    start_ = current_;

    if (is_at_end()) return make_token(TokenType::END_OF_FILE);

    char c = advance();

    if (is_alpha(c)) return identifier();
    if (is_digit(c)) return number();

    switch (c) {
        case '(': return make_token(TokenType::LPAREN);
        case ')': return make_token(TokenType::RPAREN);
        case '{': return make_token(TokenType::LBRACE);
        case '}': return make_token(TokenType::RBRACE);
        case '[': return make_token(TokenType::LBRACKET);
        case ']': return make_token(TokenType::RBRACKET);
        case ',': return make_token(TokenType::COMMA);
        case '.': return make_token(TokenType::DOT);
        case '-': return make_token(peek() == '>' ? (advance(), TokenType::ARROW) : TokenType::MINUS);
        case '+': return make_token(TokenType::PLUS);
        case ';': return make_token(TokenType::SEMICOLON);
        case ':': return make_token(TokenType::COLON);
        case '*': return make_token(TokenType::STAR);
        case '!': return make_token(peek() == '=' ? (advance(), TokenType::BANG_EQUAL) : TokenType::BANG);
        case '=': return make_token(peek() == '=' ? (advance(), TokenType::EQUAL_EQUAL) : (peek() == '>' ? (advance(), TokenType::FAT_ARROW) : TokenType::EQUAL));
        case '<': return make_token(peek() == '=' ? (advance(), TokenType::LESS_EQUAL) : TokenType::LESS);
        case '>': return make_token(peek() == '=' ? (advance(), TokenType::GREATER_EQUAL) : TokenType::GREATER);
        case '/':
            if (peek() == '/') {
                while (peek() != '\n' && !is_at_end()) advance();
                return next_token(); // Recurse to get the next token after comment
            } else {
                return make_token(TokenType::SLASH);
            }
        case '"': return string();
    }

    return make_token(TokenType::ILLEGAL);
}

void Lexer::skip_whitespace() {
    while (true) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                location_.line++;
                location_.column = 0;
                advance();
                break;
            default:
                return;
        }
    }
}

char Lexer::advance() {
    if (!is_at_end()) {
        current_++;
        location_.column++;
    }
    return source_[current_ - 1];
}

char Lexer::peek() const {
    if (is_at_end()) return '\0';
    return source_[current_];
}

Token Lexer::make_token(TokenType type) const {
    return Token(type, source_.substr(start_, current_ - start_), {}, location_);
}

Token Lexer::make_token(TokenType type, Token::LiteralValue literal) const {
    return Token(type, source_.substr(start_, current_ - start_), literal, location_);
}

Token Lexer::identifier() {
    while (is_alpha(peek()) || is_digit(peek())) advance();
    std::string text = source_.substr(start_, current_ - start_);
    auto it = keywords_.find(text);
    if (it != keywords_.end()) {
        if (text == "true") return make_token(it->second, true);
        if (text == "false") return make_token(it->second, false);
        return make_token(it->second);
    }
    return make_token(TokenType::IDENTIFIER);
}

Token Lexer::number() {
    while (is_digit(peek())) advance();
    if (peek() == '.' && is_digit(peek_next())) {
        advance(); // Consume the '.'
        while (is_digit(peek())) advance();
        return make_token(TokenType::FLOAT_LITERAL, std::stod(source_.substr(start_, current_ - start_)));
    }
    return make_token(TokenType::INTEGER_LITERAL, std::stoll(source_.substr(start_, current_ - start_)));
}

Token Lexer::string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') {
            location_.line++;
            location_.column = 0;
        }
        advance();
    }
    if (is_at_end()) return make_token(TokenType::ILLEGAL);
    advance(); // Closing quote
    std::string value = source_.substr(start_ + 1, current_ - start_ - 2);
    return make_token(TokenType::STRING_LITERAL, value);
}

bool Lexer::is_at_end() const {
    return current_ >= source_.size();
}

bool Lexer::is_alpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::is_digit(char c) const {
    return c >= '0' && c <= '9';
}

char Lexer::peek_next() const {
    if (current_ + 1 >= source_.size()) return '\0';
    return source_[current_ + 1];
}

} // namespace thermolang
