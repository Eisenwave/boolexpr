#include <iostream>

#include "lexer.hpp"

namespace {

constexpr bool is_digit(const char c) noexcept
{
    return c >= '0' && c <= '9';
}

constexpr bool is_alpha(const char c) noexcept
{
    return (c | 32) >= 'a' && (c | 32) <= 'z';
}

constexpr bool is_alphanum(const char c) noexcept
{
    return is_digit(c) || is_alpha(c);
}

constexpr char to_lower(const char c) noexcept
{
    return c | 32;
}

constexpr std::uint64_t tiny_string(std::string_view str) noexcept
{
    std::uint64_t result = 0;
    if (str.length() > 8) {
        return result;
    }
    for (const char c : str) {
        result <<= 8;
        result |= static_cast<std::uint8_t>(to_lower(c));
    }
    return result;
}

static_assert(tiny_string("nOR") == ('n' << 16 | 'o' << 8 | 'r'));

constexpr TokenType token_of_word(std::string_view word) noexcept
{
    switch (tiny_string(word)) {
    case tiny_string("and"): return TokenType::AND;

    case tiny_string("nand"):
    case tiny_string("notand"): return TokenType::NAND;

    case tiny_string("or"): return TokenType::OR;

    case tiny_string("nor"):
    case tiny_string("notor"): return TokenType::NOR;

    case tiny_string("xor"): return TokenType::XOR;

    case tiny_string("nxor"):
    case tiny_string("notxor"): return TokenType::NXOR;

    case tiny_string("andn"):
    case tiny_string("andnot"): return TokenType::ANDN;

    case tiny_string("not"): return TokenType::NOT;
    default: return TokenType::EMPTY;
    }
}

constexpr TokenType token_type_of_char(char c) noexcept
{
    switch (c) {
    case '~': return TokenType::NOT;
    case '+': return TokenType::OR;
    case '*': return TokenType::AND;
    case '(': return TokenType::PARENS_OPEN;
    case ')': return TokenType::PARENS_CLOSE;
    default: return TokenType::EMPTY;
    }
}

struct ExpressionTokenizer {
    std::vector<Token> tokens;
    std::string literal;
    std::size_t i = 0;
    std::string_view expr;

    explicit ExpressionTokenizer(std::string_view expr) : expr{expr} {}

    void tokenize();

private:
    [[noreturn]] void error(std::size_t i, std::string_view msg) noexcept;

    [[noreturn]] void unexpected_token_error();

    char tokenize_after_whitespace(char c);
    char tokenize_in_literal(char c);
    char tokenize_after_exclamation(char c);
    char tokenize_after_equals(char c);

    template <char Start>
    char tokenize_after_double_op(char c);

    void push(TokenType type, std::string value);

    void push(const TokenType type, const char c)
    {
        tokens.push_back({type, {c}});
    }
};

void ExpressionTokenizer::push(TokenType type, std::string value)
{
    if (type != TokenType::LITERAL) {
        tokens.push_back({type, std::move(value)});
    }
    else {
        if (auto actual_type = token_of_word(value); actual_type != TokenType::EMPTY) {
            type = actual_type;
        }
        tokens.push_back({type, std::move(value)});
    }
}

[[noreturn]] void ExpressionTokenizer::error(std::size_t i, std::string_view msg) noexcept
{
    constexpr const char *indent = "        ";
    std::cout << "Parse error at index " << i << ": " << msg << '\n';
    std::cout << indent << '"' << expr << "\"\n";
    std::cout << indent << std::string(i + 1, ' ') << "^\n";
    std::exit(1);
}

[[noreturn]] void ExpressionTokenizer::unexpected_token_error()
{
    error(i, std::string("Unexpected token '") + expr[i] + '\'');
}

void ExpressionTokenizer::tokenize()
{
    char state = ' ';
    for (i = 0; i <= expr.length(); ++i) {
        const char c = i == expr.length() ? ' ' : expr[i];

        switch (state) {
        case ' ': state = tokenize_after_whitespace(c); break;
        case 'a': state = tokenize_in_literal(c); break;
        case '!': state = tokenize_after_exclamation(c); break;
        case '=': state = tokenize_after_equals(c); break;
        case '&': state = tokenize_after_double_op<'&'>(c); break;
        case '|': state = tokenize_after_double_op<'|'>(c); break;
        }
    }
}

char ExpressionTokenizer::tokenize_after_whitespace(const char c)
{
    if (is_alphanum(c)) {
        literal = {c};
        return 'a';
    }
    switch (c) {
    case '~':
    case '+':
    case '*':
    case '(':
    case ')': push(token_type_of_char(c), c); return ' ';
    case ' ':
    case '!':
    case '|':
    case '&':
    case '=': return c;
    default: unexpected_token_error();
    }
}

char ExpressionTokenizer::tokenize_in_literal(const char c)
{
    if (is_alphanum(c)) {
        literal.push_back(c);
        return 'a';
    }
    switch (c) {
    case '~':
    case '+':
    case '*':
    case '(':
    case ')':
        push(TokenType::LITERAL, std::move(literal));
        push(token_type_of_char(c), c);
        return ' ';
    case ' ':
    case '!':
    case '|':
    case '&':
    case '=': push(TokenType::LITERAL, std::move(literal)); return c;
    default: unexpected_token_error();
    }
}

char ExpressionTokenizer::tokenize_after_exclamation(const char c)
{
    if (c == ' ' || is_alphanum(c)) {
        push(TokenType::NOT, '!');
        literal = {c};
        return c == ' ' ? ' ' : 'a';
    }
    switch (c) {
    case '~':
    case '+':
    case '*':
    case '(':
    case ')':
        push(TokenType::NOT, '!');
        push(token_type_of_char(c), c);
        return ' ';
    case '!': push(TokenType::NOT, '!'); return '!';
    case '=': push(TokenType::XOR, "!="); return ' ';
    default: unexpected_token_error();
    }
}

char ExpressionTokenizer::tokenize_after_equals(const char c)
{
    if (c == ' ' || is_alphanum(c)) {
        push(TokenType::NXOR, "=");
        literal = {c};
        return c == ' ' ? ' ' : 'a';
    }
    switch (c) {
    case '~':
    case '+':
    case '*':
    case '(':
    case ')':
        push(TokenType::NXOR, '=');
        push(token_type_of_char(c), c);
        return ' ';
    case '=': push(TokenType::NXOR, "=="); return ' ';
    case '>': push(TokenType::CONS, "=>"); return ' ';
    case '!':
    case '|':
    case '&': push(TokenType::NXOR, '='); return c;
    default: unexpected_token_error();
    }
}

template <char Start>
char ExpressionTokenizer::tokenize_after_double_op(const char c)
{
    constexpr TokenType type = Start == '&' ? TokenType::AND : Start == '|' ? TokenType::OR : TokenType::LITERAL;
    static_assert(type != TokenType::LITERAL);

    if (c == ' ' || is_alphanum(c)) {
        push(type, Start);
        literal = {c};
        return c == ' ' ? ' ' : 'a';
    }
    switch (c) {
    case '~':
    case '+':
    case '*':
    case '(':
    case ')':
        push(type, Start);
        push(token_type_of_char(c), c);
        return ' ';
    case Start: push(type, std::string(2, Start)); return ' ';
    case Start == '&' ? '|': '&' : push(type, Start); return c;
    default: unexpected_token_error();
    }
}

}  // namespace

std::ostream &operator<<(std::ostream &out, const Token &token)
{
    return out << token_type_label(token.type) << ":\"" << token.value << '"';
}

std::vector<Token> tokenize(std::string_view expr)
{
    ExpressionTokenizer tokenizer{expr};
    tokenizer.tokenize();
    return tokenizer.tokens;
}
