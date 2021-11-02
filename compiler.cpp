#include "compiler.hpp"

#include <algorithm>
#include <iostream>
#include <queue>

constexpr unsigned VARIABLE_LIMIT = 6;

namespace  {

struct ParserToken {
    TokenType type;
    unsigned operand;
};

unsigned init_symbol_table(std::string(&symbols)[VARIABLE_LIMIT],
                           std::vector<ParserToken> &parserTokens,
                           const std::vector<Token> &tokens) {
    unsigned symbol_count = 0;
    parserTokens.reserve(tokens.size());

    for (const auto &token : tokens) {
        if (token.type != TokenType::LITERAL) {
            parserTokens.push_back({token.type, 0});
            continue;
        }

        const auto pos = std::find(symbols, std::end(symbols), token.value);
        const auto index = static_cast<unsigned>(pos - symbols);
        if (pos != std::end(symbols)) {
            parserTokens.push_back({token.type, index});
            continue;
        }
        if (symbol_count == 6) {
            std::cout << "Too many variables! (at most " << VARIABLE_LIMIT << " allowed)\n";
            std::exit(1);
        }
        parserTokens.push_back({token.type, symbol_count});
        symbols[symbol_count++] = token.value;
    }

    if (symbol_count == 0) {
        std::cout << "Expression does not contain any variables\n";
        std::exit(1);
    }

    return symbol_count;
}

constexpr unsigned token_precedence(TokenType type) noexcept {
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

constexpr Op token_operation(TokenType type) noexcept {
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

#if 0
constexpr TokenType PRECEDENCE[] {
    TokenType::PARENS_OPEN,
    TokenType::NOT,
    TokenType::NXOR,
    TokenType::AND,
    TokenType::NAND,
    TokenType::ANDN,
    TokenType::XOR,
    TokenType::OR,
    TokenType::NOR,
    TokenType::CONS
};

void do_compile(Program &program, std::vector<ParserToken> &parserTokens) noexcept {
    // make it safe to create a three-element view window
    parserTokens.push_back({TokenType::EMPTY, 0});
    parserTokens.push_back({TokenType::EMPTY, 0});

    full_restart: for (unsigned i = 0; i < std::size(PRECEDENCE); ++i) {
        const TokenType current_type = PRECEDENCE[i];
        ParserToken window[3] {parserTokens[0], parserTokens[1], parserTokens[2]};
        for (unsigned i = 0; i < parserTokens.size() - 2; ++i) {
            switch (current_type) {

            case TokenType::PARENS_OPEN:
                if (window[0].type == TokenType::PARENS_OPEN && window[2].type == TokenType::PARENS_CLOSE) {
                    parserTokens[i] = window[1];
                    parserTokens.erase(parserTokens.begin() + i + 1, parserTokens.begin() + i + 3);
                    goto full_restart;
                }
                break;

            case TokenType::NOT:
                if (window[0].type == TokenType::NOT && window[1].type == TokenType::LITERAL) {
                    program.push({static_cast<std::uint8_t>(Op::NOT_A), static_cast<std::uint8_t>(window[1].operand), 0, 0});

                    const unsigned operand = program.length + 6;
                    parserTokens[i] = {TokenType::LITERAL, operand};
                    parserTokens.erase(parserTokens.begin() + i + 1);
                    goto full_restart;
                }
                break;

            case TokenType::NXOR:
            case TokenType::AND:
            case TokenType::NAND:
            case TokenType::ANDN:
            case TokenType::XOR:
            case TokenType::OR:
            case TokenType::NOR:
            case TokenType::CONS:
                if (window[0].type == TokenType::LITERAL
                 && window[1].type == current_type
                 && window[2].type == TokenType::LITERAL) {
                    program.push({static_cast<std::uint8_t>(Op::NOT_A),
                            static_cast<std::uint8_t>(window[0].operand),
                            static_cast<std::uint8_t>(window[2].operand), 0});

                    const unsigned operand = program.length + 6;
                    parserTokens[i] = {current_type, operand};
                    parserTokens.erase(parserTokens.begin() + i + 1, parserTokens.begin() + i + 3);
                    goto full_restart;
                }
                break;

            default:
                std::cout << "Internal error: Invalid parser token\n";
                std::exit(1);
            }

            // shift view window downward
            window[0] = window[1];
            window[1] = window[2];
            window[2] = parserTokens[i + 2];
        }
    }
}
#endif

template <typename T>
bool to_reverse_polish_notation_impl(std::vector<T> &output, const std::vector<T> &tokens) {
    std::vector<T> op_stack;

    const auto pop_stack_push_output = [&output,  &op_stack] {
        output.push_back(std::move(op_stack.back()));
        op_stack.pop_back();
    };

    for (const auto &token : tokens) {
        switch (token.type) {
        case TokenType::LITERAL:
            output.push_back(token);
            break;

        case TokenType::NOT:
            op_stack.push_back(token);
            break;

        case TokenType::PARENS_OPEN:
            op_stack.push_back(token);
            break;

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
            op_stack.pop_back(); // discard opening parenthesis
            if (op_stack.back().type == TokenType::NOT) {
                pop_stack_push_output();
            }
            break;

        default:
            while (not op_stack.empty() &&
                   token_precedence(op_stack.back().type) >= 2 &&
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

bool compile_from_polish(Program &p, const std::vector<ParserToken> &polish_tokens) noexcept {
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
        const auto next_operand = static_cast<std::uint8_t>(p.length + VARIABLE_LIMIT);
        if (op_is_unary(op)) {
            auto top_op = std::exchange(stack.back(), next_operand);
            p.push({static_cast<std::uint8_t>(op), top_op, 0, 0});
        }
        else {
            auto top_b = stack.back();
            stack.pop_back();
            auto top_a = std::exchange(stack.back(), next_operand);
            p.push({static_cast<std::uint8_t>(op), top_a, top_b, 0});
        }
    }
    return true;
}

bool do_compile(Program &program, const std::vector<ParserToken> &tokens) {
    std::vector<ParserToken> reverse_polish;
    if (not to_reverse_polish_notation_impl(reverse_polish, tokens)) {
        return false;
    }
    return compile_from_polish(program, reverse_polish);
}

} // namespace

bool to_reverse_polish_notation(std::vector<Token> &output, const std::vector<Token> &tokens) {
    return to_reverse_polish_notation_impl(output, tokens);
}


Program compile(const std::vector<Token> &tokens) noexcept {
    Program p{};
    std::vector<ParserToken> parser_tokens;
    p.variables = init_symbol_table(p.symbols, parser_tokens, tokens);

    do_compile(p, parser_tokens);
    return p;
}
