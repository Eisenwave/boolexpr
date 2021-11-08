#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include <cstdint>
#include <iosfwd>
#include <string_view>
#include <string>
#include <array>
#include <vector>

#include "util.hpp"
#include "operation.hpp"

constexpr char DONT_CARE = 'x';

enum class InstructionSet : std::uint64_t {
    NAND = to_underlying(Op::NOT_A)
          | to_underlying(Op::NAND) << 4,
    NOR = to_underlying(Op::NOT_A)
          | to_underlying(Op::NOR) << 4,
    BASIC = to_underlying(Op::NOT_A)
          | to_underlying(Op::AND) << 4
          | to_underlying(Op::OR) << 8,
    C = BASIC | to_underlying(Op::XOR) << 12,
    X64 = C | to_underlying(Op::A_ANDN_B) << 16,
};

struct TruthTable {
    /// table where all don't cares are false
    std::uint64_t f;
    /// table where all don't cares are true
    std::uint64_t t;
};

TruthTable truth_table_parse(std::string_view str) noexcept;

bool truth_table_is_valid(std::string_view str) noexcept;

struct Instruction {
    /// the truth table of the operation
    std::uint8_t op;
    /// the index of the first operand, where the first six values are reserved for the program inputs
    std::uint8_t a;
    /// the index of the second operand, where the first six values are reserved for the program inputs
    std::uint8_t b;

    constexpr bool operator==(const Instruction &other) const noexcept {
        return this->op == other.op && this->a == other.a && this->b == other.b;
    }

    constexpr bool operator!=(const Instruction &other) const noexcept {
        return not (*this == other);
    }
};

inline constexpr Instruction EOF_INSTRUCTION = {0xff, 0xff, 0xff};

struct Program {
    using instruction_type = Instruction;
    using state_type = bitvec256;
    using size_type = std::size_t;
    static constexpr size_type instruction_count = 250;

private:
    std::array<Instruction, instruction_count> instructions;
    size_type length;
public:
    size_type variables;
    std::array<std::string, 6> symbols;

public:
    explicit Program(const size_type variables = 0) : instructions{}, length{0}, variables{variables} {}

    constexpr size_type size() const noexcept {
        return length;
    }

    constexpr instruction_type operator[](const size_type i) const noexcept {
        return instructions[i];
    }

    constexpr void push(const Instruction ins) noexcept {
        instructions[length++] = ins;
    }

    constexpr void push(const Op op, const unsigned a, const unsigned b) noexcept {
        push({static_cast<std::uint8_t>(op), static_cast<std::uint8_t>(a), static_cast<std::uint8_t>(b)});
    }

    constexpr Instruction &top() noexcept {
        return instructions[length - 1];
    }

    constexpr const Instruction &top() const noexcept {
        return instructions[length - 1];
    }

    constexpr void pop() noexcept {
        --length;
    }

    constexpr void clear() noexcept {
        length = 0;
    }

    std::string symbol(size_type i, bool input_prefix = true) const noexcept;

    bool is_equivalent(TruthTable table) const noexcept;

    TruthTable compute_truth_table() const noexcept;
};

std::vector<Instruction> find_equivalent_programs(const TruthTable table,
                                                  const InstructionSet instructionSet,
                                                  const std::size_t variables,
                                                  const bool exhaustive) noexcept;

std::ostream &print_instruction(std::ostream &out, Instruction ins, const Program &p);

std::ostream &print_program_as_expression(std::ostream &out, const Program &program);

std::ostream& operator<<(std::ostream &out, const Program &p);

#endif // PROGRAM_HPP
