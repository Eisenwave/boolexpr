#ifndef COMPILER_HPP
#define COMPILER_HPP

#include "lexer.hpp"

enum class SymbolOrder { APPEARANCE_ASCENDING, APPEARANCE_DESCENDING, LEX_ASCENDING, LEX_DESCENDING };

[[nodiscard]] bool to_reverse_polish_notation(std::vector<Token> &output, const std::vector<Token> &tokens);

[[nodiscard]] Program compile(const std::vector<Token> &tokens, SymbolOrder order) noexcept;

#endif
