#include <iostream>
#include <unordered_map>

#include "lexer.hpp"

namespace {

constexpr bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

constexpr bool is_alpha(char c) noexcept {
    return (c | 32) >= 'a' && (c | 32) <= 'z';
}

constexpr bool is_alphanum(char c) noexcept {
    return is_digit(c) || is_alpha(c);
}


struct ExpressionTokenizer {
    std::vector<Token> tokens;
    std::string literal;
    std::size_t i = 0;
    std::string_view expr;

    explicit ExpressionTokenizer(std::string_view expr) : expr{expr} {}

    void tokenize();

private:
    [[noreturn]]
    void error(std::size_t i, std::string_view msg) noexcept;

    [[noreturn]]
    void unexpected_token_error();

    char tokenize_after_whitespace(const char c);
    char tokenize_in_literal(const char c);
    char tokenize_after_exclamation(const char c);
    char tokenize_after_equals(const char c);

    template <char Start>
    char tokenize_after_double_op(const char c);

    void push(TokenType type, std::string value);

    void push(TokenType type, char c) {
        tokens.push_back({type, {c}});
    }
};

void ExpressionTokenizer::push(TokenType type, std::string value) {
    static const std::unordered_map<std::string_view, TokenType> reservedWords{
        {"and", TokenType::AND},
        {"nand", TokenType::NAND},
        {"or", TokenType::OR},
        {"nor", TokenType::NOR},
        {"xor", TokenType::XOR},
        {"nxor", TokenType::NXOR},
        {"andn", TokenType::ANDN},
        {"andnot", TokenType::ANDN},
        {"not", TokenType::NOT}
    };

    if (type != TokenType::LITERAL) {
        tokens.push_back({type, std::move(value)});
        return;
    }

    std::string lower = value;
    for (char &c : lower) {
        c |= 32;
    }
    if (auto it = reservedWords.find(lower); it != reservedWords.end()) {
        tokens.push_back({it->second, std::move(value)});
    }
    else {
        tokens.push_back({TokenType::LITERAL, std::move(value)});
    }
}

[[noreturn]]
void ExpressionTokenizer::error(std::size_t i, std::string_view msg) noexcept {
    constexpr const char *indent = "        ";
    std::cout << "Parse error at index " << i << ": " << msg << '\n';
    std::cout << indent << '"' << expr << "\"\n";
    std::cout << indent << std::string(i + 1, ' ') << "^\n";
    std::exit(1);
}

[[noreturn]]
void ExpressionTokenizer::unexpected_token_error() {
    error(i, std::string("Unexpected token '") + expr[i] + '\'');
}

void ExpressionTokenizer::tokenize() {
    char state = ' ';
    for (i = 0; i <= expr.length(); ++i) {
        const char c = i == expr.length() ? ' ' : expr[i];

        switch (state) {
        case ' ':
            state = tokenize_after_whitespace(c);
            break;
        case 'a':
            state = tokenize_in_literal(c);
            break;
        case '!':
            state = tokenize_after_exclamation(c);
            break;
        case '=':
            state = tokenize_after_equals(c);
            break;
        case '&':
            state = tokenize_after_double_op<'&'>(c);
            break;
        case '|':
            state = tokenize_after_double_op<'|'>(c);
            break;
        }
    }
}

char ExpressionTokenizer::tokenize_after_whitespace(const char c) {
    if (is_alphanum(c)) {
        literal = {c};
        return 'a';
    }
    switch (c) {
    case '~':
        push(TokenType::NOT, "~");
        return ' ';
    case ' ':
    case '!':
    case '|':
    case '&':
    case '=':
        return c;
    case '(':
        push(TokenType::PARENS_OPEN, '(');
        return ' ';
    case ')':
        push(TokenType::PARENS_CLOSE, ')');
        return ' ';
    default:
        unexpected_token_error();
    }
}

char ExpressionTokenizer::tokenize_in_literal(const char c) {
    if (is_alphanum(c)) {
        literal.push_back(c);
        return 'a';
    }
    switch (c) {
    case '~':
        push(TokenType::LITERAL, std::move(literal));
        push(TokenType::NOT, '~');
        return ' ';
    case ' ':
    case '!':
    case '|':
    case '&':
    case '=':
        push(TokenType::LITERAL, std::move(literal));
        return c;
    case '(':
        push(TokenType::LITERAL, std::move(literal));
        push(TokenType::PARENS_OPEN, '(');
        return ' ';
    case ')':
        push(TokenType::LITERAL, std::move(literal));
        push(TokenType::PARENS_CLOSE, ')');
        return ' ';
    default:
        unexpected_token_error();
    }
}

char ExpressionTokenizer::tokenize_after_exclamation(const char c) {
    if (c == ' ' || is_alphanum(c)) {
        push(TokenType::NOT, '!');
        literal = {c};
        return c == ' ' ? ' ' : 'a';
    }
    switch (c) {
    case '~':
        push(TokenType::NOT, '!');
        push(TokenType::NOT, '~');
        return ' ';
    case '!':
        push(TokenType::NOT, '!');
        return '!';
    case '=':
        push(TokenType::XOR, "!=");
        return ' ';
    case '(':
        push(TokenType::NOT, '!');
        push(TokenType::PARENS_OPEN, '(');
        return ' ';
    default: unexpected_token_error();
    }
}

char ExpressionTokenizer::tokenize_after_equals(const char c) {
    if (c == ' ' || is_alphanum(c)) {
        push(TokenType::NXOR, "=");
        literal = {c};
        return c == ' ' ? ' ' : 'a';
    }
    switch (c) {
    case '~':
        push(TokenType::NXOR, '=');
        push(TokenType::NOT, '~');
        return ' ';
    case '=':
        push(TokenType::NXOR, "==");
        return ' ';
    case '>':
        push(TokenType::CONS, "=>");
        return ' ';
    case '!':
    case '|':
    case '&':
        push(TokenType::NXOR, '=');
        return c;
    case '(':
        push(TokenType::NXOR, '=');
        push(TokenType::PARENS_OPEN, '(');
        return ' ';
    default: unexpected_token_error();
    }
}

template <char Start>
char ExpressionTokenizer::tokenize_after_double_op(const char c) {
    constexpr TokenType type = Start == '&' ? TokenType::AND
                             : Start == '|' ? TokenType::OR
                             : TokenType::LITERAL;
    static_assert (type != TokenType::LITERAL);

    if (c == ' ' || is_alphanum(c)) {
        push(type, Start);
        literal = {c};
        return c == ' ' ? ' ' : 'a';
    }
    switch (c) {
    case '~':
        push(type, Start);
        push(TokenType::NOT, '~');
        return ' ';
    case Start:
        push(type, std::string(2, Start));
        return ' ';
    case Start == '&' ? '|' : '&':
        push(type, Start);
        return c;
    case '(':
        push(type, Start);
        push(TokenType::PARENS_OPEN, '(');
        return ' ';
    default: unexpected_token_error();
    }
}

}

std::ostream &operator<<(std::ostream &out, const Token &token) {
    return out << token_type_label(token.type) << ":\"" << token.value << '"';
}

std::vector<Token> tokenize(std::string_view expr) {
    ExpressionTokenizer tokenizer{expr};
    tokenizer.tokenize();
    return tokenizer.tokens;
}
