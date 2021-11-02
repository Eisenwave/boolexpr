#ifndef COMPILER_HPP
#define COMPILER_HPP

#include "lexer.hpp"

bool to_reverse_polish_notation(std::vector<Token> &output, const std::vector<Token> &tokens);

Program compile(const std::vector<Token> &) noexcept;

#endif
