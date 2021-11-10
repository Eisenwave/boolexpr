#include "compiler.hpp"

#include <algorithm>
#include <iostream>
#include <queue>

constexpr unsigned VARIABLE_LIMIT = 6;

namespace {

struct ParserToken {
    TokenType type;
    unsigned operand;
};

[[nodiscard]] constexpr bool order_is_lexicographical(SymbolOrder order) noexcept
{
    return order == SymbolOrder::LEX_ASCENDING || order == SymbolOrder::LEX_DESCENDING;
}

[[nodiscard]] constexpr bool order_is_descending(SymbolOrder order) noexcept
{
    return order == SymbolOrder::APPEARANCE_DESCENDING || order == SymbolOrder::LEX_DESCENDING;
}

unsigned find_symbols(std::string *const symbols, const std::vector<Token> &tokens)
{
    unsigned symbol_count = 0;

    for (const auto &token : tokens) {
        if (token.type != TokenType::LITERAL) {
            continue;
        }

        const auto pos = std::find(symbols, symbols + VARIABLE_LIMIT, token.value);
        if (pos != symbols + VARIABLE_LIMIT) {
            continue;
        }
        if (symbol_count == 6) {
            std::cout << "Too many variables! (at most " << VARIABLE_LIMIT << " allowed)\n";
            std::exit(1);
        }
        symbols[symbol_count++] = token.value;
    }

    return symbol_count;
}

void sort_symbol_table(std::string *const symbols, const unsigned count, const SymbolOrder order) noexcept
{
    if (order_is_lexicographical(order)) {
        std::sort(symbols, symbols + count);
    }
    if (order_is_descending(order)) {
        std::reverse(symbols, symbols + count);
    }
}

void apply_symbol_table(const std::string *const symbols,
                        const unsigned count,
                        std::vector<ParserToken> &parserTokens,
                        const std::vector<Token> &tokens)
{
    parserTokens.reserve(tokens.size());

    for (const auto &token : tokens) {
        if (token.type != TokenType::LITERAL) {
            parserTokens.push_back({token.type, 0});
            continue;
        }

        const auto pos = std::find(symbols, symbols + count, token.value);
        const auto index = static_cast<unsigned>(pos - symbols);
        parserTokens.push_back({token.type, index});
    }
}

unsigned init_symbol_table(std::string *const symbols,
                           std::vector<ParserToken> &parserTokens,
                           const std::vector<Token> &tokens,
                           const SymbolOrder order)
{
    const unsigned count = find_symbols(symbols, tokens);
    if (count == 0) {
        std::cout << "Expression does not contain any variables\n";
        std::exit(1);
    }

    sort_symbol_table(symbols, count, order);
    apply_symbol_table(symbols, count, parserTokens, tokens);
    return count;
}

constexpr unsigned token_precedence(const TokenType type) noexcept
{
    switch (type) {
    case TokenType::EMPTY:
    case TokenType::LITERAL:
    case TokenType::PARENS_OPEN:
    case TokenType::PARENS_CLOSE: return 0;
    case TokenType::NOT: return 1;
    case TokenType::NXOR: return 2;
    case TokenType::AND: return 3;
    case TokenType::NAND: return 4;
    case TokenType::ANDN: return 5;
    case TokenType::XOR: return 6;
    case TokenType::OR: return 7;
    case TokenType::NOR: return 8;
    case TokenType::CONS: return 9;
    }
    __builtin_unreachable();
}

constexpr Op token_operation(const TokenType type) noexcept
{
    switch (type) {
    case TokenType::EMPTY:
    case TokenType::LITERAL:
    case TokenType::PARENS_CLOSE:
    case TokenType::PARENS_OPEN: return Op::FALSE;

    case TokenType::NOT: return Op::NOT_A;
    case TokenType::AND: return Op::AND;
    case TokenType::NAND: return Op::NAND;
    case TokenType::OR: return Op::OR;
    case TokenType::NOR: return Op::NOR;
    case TokenType::XOR: return Op::XOR;
    case TokenType::NXOR: return Op::NXOR;
    case TokenType::CONS: return Op::A_CONS_B;
    case TokenType::ANDN: return Op::A_ANDN_B;
    }
    __builtin_unreachable();
}

template <typename T>
bool to_reverse_polish_notation_impl(std::vector<T> &output, const std::vector<T> &tokens)
{
    std::vector<T> op_stack;

    const auto pop_stack_push_output = [&output, &op_stack] {
        output.push_back(std::move(op_stack.back()));
        op_stack.pop_back();
    };

    for (const auto &token : tokens) {
        switch (token.type) {
        case TokenType::LITERAL: output.push_back(token); break;

        case TokenType::NOT: op_stack.push_back(token); break;

        case TokenType::PARENS_OPEN: op_stack.push_back(token); break;

        case TokenType::PARENS_CLOSE:
            while (op_stack.back().type != TokenType::PARENS_OPEN) {
                if (op_stack.empty()) {
                    std::cout << "Syntax error: mismatched parentheses\n";
                    return false;
                }
                pop_stack_push_output();
            }
            if (op_stack.back().type != TokenType::PARENS_OPEN) {
                std::cout << "Syntax error: mismatched parentheses\n";
                return false;
            }
            op_stack.pop_back();  // discard opening parenthesis
            if (op_stack.back().type == TokenType::NOT) {
                pop_stack_push_output();
            }
            break;

        default:
            while (not op_stack.empty() && token_precedence(op_stack.back().type) >= 1 &&
                   token_precedence(op_stack.back().type) <= token_precedence(token.type)) {
                pop_stack_push_output();
            }
            op_stack.push_back(token);
        }
    }

    while (not op_stack.empty()) {
        if (op_stack.back().type == TokenType::PARENS_OPEN) {
            std::cout << "Syntax error: mismatched parentheses\n";
            return false;
        }
        pop_stack_push_output();
    }
    return true;
}

bool compile_from_polish(Program &p, const std::vector<ParserToken> &polish_tokens) noexcept
{
    std::vector<std::uint8_t> stack;
    for (const auto token : polish_tokens) {
        if (token.type == TokenType::LITERAL) {
            stack.push_back(static_cast<std::uint8_t>(token.operand));
            continue;
        }
        Op op = token_operation(token.type);
        if (op_is_trivial(op)) {
            std::cout << "Internal error";
            return false;
        }
        const auto next_operand = static_cast<std::uint8_t>(p.size() + VARIABLE_LIMIT);
        if (op_is_unary(op)) {
            auto top_op = std::exchange(stack.back(), next_operand);
            p.push({static_cast<std::uint8_t>(op), top_op, 0});
        }
        else {
            auto top_b = stack.back();
            stack.pop_back();
            auto top_a = std::exchange(stack.back(), next_operand);
            p.push({static_cast<std::uint8_t>(op), top_a, top_b});
        }
    }
    return true;
}

bool do_compile(Program &program, const std::vector<ParserToken> &tokens)
{
    std::vector<ParserToken> reverse_polish;
    if (not to_reverse_polish_notation_impl(reverse_polish, tokens)) {
        return false;
    }
    return compile_from_polish(program, reverse_polish);
}

}  // namespace

bool to_reverse_polish_notation(std::vector<Token> &output, const std::vector<Token> &tokens)
{
    return to_reverse_polish_notation_impl(output, tokens);
}

Program compile(const std::vector<Token> &tokens, const SymbolOrder order) noexcept
{
    Program p{};
    std::vector<ParserToken> parser_tokens;
    p.variables = init_symbol_table(p.symbols.data(), parser_tokens, tokens, order);

    do_compile(p, parser_tokens);
    return p;
}
