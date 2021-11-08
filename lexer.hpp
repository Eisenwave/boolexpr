#ifndef PARSE_HPP
#define PARSE_HPP

#include <iosfwd>
#include <string>
#include <vector>

#include "program.hpp"

#define BOOLEXPR_ENUM_LIST_TOKEN_TYPE \
    BOOLEXPR_ENUM_ACTION(EMPTY)       \
    BOOLEXPR_ENUM_ACTION(LITERAL)     \
    BOOLEXPR_ENUM_ACTION(NOT)         \
    BOOLEXPR_ENUM_ACTION(AND)         \
    BOOLEXPR_ENUM_ACTION(NAND)        \
    BOOLEXPR_ENUM_ACTION(OR)          \
    BOOLEXPR_ENUM_ACTION(NOR)         \
    BOOLEXPR_ENUM_ACTION(XOR)         \
    BOOLEXPR_ENUM_ACTION(NXOR)        \
    BOOLEXPR_ENUM_ACTION(CONS)        \
    BOOLEXPR_ENUM_ACTION(ANDN)        \
    BOOLEXPR_ENUM_ACTION(PARENS_OPEN) \
    BOOLEXPR_ENUM_ACTION(PARENS_CLOSE)

#define BOOLEXPR_ENUM_ACTION(e) e,
enum class TokenType : unsigned { BOOLEXPR_ENUM_LIST_TOKEN_TYPE };
#undef BOOLEXPR_ENUM_ACTION

constexpr const char *token_type_label(TokenType type) noexcept
{
#define BOOLEXPR_ENUM_ACTION(e) \
    case TokenType::e: return #e;
    switch (type) {
        BOOLEXPR_ENUM_LIST_TOKEN_TYPE
    }
    __builtin_unreachable();
}
#undef BOOLEXPR_ENUM_ACTION

struct Token {
    TokenType type;
    std::string value;
};

std::ostream &operator<<(std::ostream &out, const Token &token);

std::vector<Token> tokenize(std::string_view expr);

#endif  // PARSE_HPP
