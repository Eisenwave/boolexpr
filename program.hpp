#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include <cstdint>
#include <iosfwd>
#include <string_view>
#include <string>

#include "util.hpp"
#include "operation.hpp"

constexpr char DONT_CARE = 'x';

struct Instruction {
    /// the truth table of the operation
    std::uint8_t op;
    /// the index of the first operand, where the first six values are reserved for the program inputs
    std::uint8_t a;
    /// the index of the second operand, where the first six values are reserved for the program inputs
    std::uint8_t b;
    /// the distance in the DAG of value dependencies from the program inputs
    std::uint8_t distance;
};

enum class InstructionSet : std::uint64_t {
    NAND = to_underlying(Op::NAND),
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

struct Program {
    Instruction instructions[256];
    unsigned length;
    unsigned variables;
    std::string symbols[6];

    constexpr void push(Instruction ins) noexcept {
        instructions[length++] = ins;
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

    std::string symbol(std::size_t i) const noexcept;
};

TruthTable truth_table_parse(std::string_view str) noexcept;

bool truth_table_is_valid(std::string_view str) noexcept;

bool program_is_equivalent(const Program &program, const TruthTable table) noexcept;

TruthTable program_compute_truth_table(const Program &program) noexcept;

Program find_equivalent_program(const TruthTable table,
                                const InstructionSet instructionSet,
                                const unsigned variables) noexcept;

std::ostream &print_instruction(std::ostream &out, Instruction ins, const Program &p);

std::ostream& operator<<(std::ostream &out, const Program &p);

#endif // PROGRAM_HPP
