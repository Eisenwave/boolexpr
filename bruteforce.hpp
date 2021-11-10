#ifndef BRUTEFORCE_HPP
#define BRUTEFORCE_HPP

#include <array>
#include <cstdint>

#include "program.hpp"

struct CanonicalInstruction {
    /// the truth table of the operation
    std::uint8_t op;
    /// the index of the first operand, where the first six values are reserved for the program inputs
    std::uint8_t a;
    /// the index of the second operand, where the first six values are reserved for the program inputs
    std::uint8_t b;
    /// the maximum distance in the DAG from the inputs
    std::uint8_t distance;

    constexpr explicit operator Instruction() const noexcept
    {
        return {op, a, b};
    }

    constexpr std::uint32_t to_integral() const noexcept
    {
        return std::uint32_t{op} | std::uint32_t{a} << 8 | std::uint32_t{b} << 16 | std::uint32_t{distance} << 24;
    }

    constexpr bool operator<(const CanonicalInstruction &other) const noexcept
    {
        return to_integral() < other.to_integral();
    }

    constexpr bool operator==(const CanonicalInstruction &other) const noexcept
    {
        return to_integral() == other.to_integral();
    }
};

struct CanonicalProgram : public ProgramBase<CanonicalInstruction, 58> {
    using state_type = std::uint64_t;

public:
    size_type target_length;

    explicit CanonicalProgram(const size_type target_length) noexcept : target_length{target_length} {}

    bool try_push(const Op op, const unsigned a) noexcept;

    bool try_push(const Op op, const unsigned a, const unsigned b) noexcept;

    void reset(const size_type target_length) noexcept
    {
        clear();
        this->target_length = target_length;
    }
};

#endif  // BRUTEFORCE_HPP
