#ifndef COMPILER_HPP
#define COMPILER_HPP

#include "lexer.hpp"

[[nodiscard]] bool to_reverse_polish_notation(std::vector<Token> &output, const std::vector<Token> &tokens);

[[nodiscard]] Program compile(const std::vector<Token> &) noexcept;

#endif
