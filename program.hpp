#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include <array>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>

#include "operation.hpp"
#include "truth_table.hpp"
#include "util.hpp"

enum class InstructionSet : std::uint64_t {
    NAND = to_underlying(Op::NOT_A) | to_underlying(Op::NAND) << 4,
    NOR = to_underlying(Op::NOT_A) | to_underlying(Op::NOR) << 4,
    BASIC = to_underlying(Op::NOT_A) | to_underlying(Op::AND) << 4 | to_underlying(Op::OR) << 8,
    C = BASIC | to_underlying(Op::XOR) << 12,
    X64 = C | to_underlying(Op::A_ANDN_B) << 16,
};

struct Instruction {
    /// the truth table of the operation
    std::uint8_t op;
    /// the index of the first operand, where the first six values are reserved for the program inputs
    std::uint8_t a;
    /// the index of the second operand, where the first six values are reserved for the program inputs
    std::uint8_t b;

    constexpr bool operator==(const Instruction &other) const noexcept
    {
        return this->op == other.op && this->a == other.a && this->b == other.b;
    }

    constexpr bool operator!=(const Instruction &other) const noexcept
    {
        return not(*this == other);
    }
};

inline constexpr Instruction EOF_INSTRUCTION = {0xff, 0xff, 0xff};
inline constexpr Instruction FALSE_INSTRUCTION{static_cast<std::uint8_t>(Op::FALSE), 0, 0};
inline constexpr Instruction TRUE_INSTRUCTION{static_cast<std::uint8_t>(Op::TRUE), 0, 0};

template <typename T, std::size_t N>
struct ProgramBase {
    using instruction_type = T;
    using size_type = std::size_t;
    static constexpr size_type instruction_count = N;

protected:
    std::array<instruction_type, instruction_count> instructions{};
    size_type length = 0;

public:
    constexpr size_type size() const noexcept
    {
        return length;
    }

    constexpr bool empty() const noexcept
    {
        return length == 0;
    }

    constexpr instruction_type operator[](const size_type i) const noexcept
    {
        return instructions[i];
    }

    constexpr instruction_type &top() noexcept
    {
        return instructions[length - 1];
    }

    constexpr const instruction_type &top() const noexcept
    {
        return instructions[length - 1];
    }

    constexpr void clear() noexcept
    {
        length = 0;
    }

    constexpr void push(const instruction_type ins) noexcept
    {
        instructions[length++] = ins;
    }

    constexpr void pop() noexcept
    {
        --length;
    }
};

struct Program : public ProgramBase<Instruction, 250> {
    using state_type = bitvec256;
    using base_type = ProgramBase<Instruction, 250>;

    size_type variables;
    std::array<std::string, 6> symbols;

    explicit Program(const size_type variables = 0) noexcept : variables{variables} {}

    using base_type::push;

    constexpr void push(const Op op, const unsigned a, const unsigned b) noexcept
    {
        push({static_cast<std::uint8_t>(op), static_cast<std::uint8_t>(a), static_cast<std::uint8_t>(b)});
    }

    [[nodiscard]] std::string symbol(size_type i, bool input_prefix = true) const noexcept;

    [[nodiscard]] bool is_equivalent(TruthTable table) const noexcept;

    [[nodiscard]] TruthTable compute_truth_table() const noexcept;
};

struct ProgramConsumer {
    virtual ~ProgramConsumer();
    virtual void operator()(const Instruction *ins, std::size_t count) = 0;
};

void find_equivalent_programs(ProgramConsumer &consumer,
                              const TruthTable table,
                              InstructionSet instructionSet,
                              std::size_t variables,
                              bool exhaustive);

std::ostream &print_instruction(std::ostream &out, Instruction ins, const Program &p);

std::ostream &print_program_as_expression(std::ostream &out, const Program &program);

std::ostream &operator<<(std::ostream &out, const Program &p);

#endif  // PROGRAM_HPP
