#ifndef BRUTEFORCE_HPP
#define BRUTEFORCE_HPP

#include <array>
#include <cstdint>

#include "program.hpp"

struct alignas(std::uint32_t) CanonicalInstruction {
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

struct CanonicalProgram : protected ProgramBase<CanonicalInstruction, 58> {
    using state_type = std::uint64_t;
    using base_type = ProgramBase<CanonicalInstruction, 58>;
    using base_type::instruction_count;
    using base_type::instruction_type;

protected:
    state_type used = 0;
    size_type target_length_;
    state_type target_relevancy_;

public:
    explicit CanonicalProgram(const size_type target_length, const state_type target_relevancy) noexcept
        : target_length_{target_length}, target_relevancy_{target_relevancy}
    {
    }

    using base_type::empty;
    using base_type::size;
    using base_type::operator[];
    using base_type::top;

    state_type used_instructions() const noexcept
    {
        return used;
    }

    state_type target_relevancy() const noexcept
    {
        return target_relevancy_;
    }

    state_type target_length() const noexcept
    {
        return target_length_;
    }

    bool try_push(const Op op, const unsigned a) noexcept;

    bool try_push(const Op op, const unsigned a, const unsigned b) noexcept;

    void reset(const size_type target_length) noexcept
    {
        clear();
        this->target_length_ = target_length;
        this->used = 0;
    }

    void clear() noexcept
    {
        used = 0;
        base_type::clear();
    }

    void push(const instruction_type ins) noexcept
    {
        used |= state_type{1} << ins.a | state_type{1} << ins.b;
        base_type::push(ins);
    }

    void pop() noexcept
    {
        const size_type new_length = std::exchange(length, 0) - 1;
        used = 0;
        for (size_type i = 0; i < new_length; ++i) {
            push(instructions[i]);
        }
    }
};

#endif  // BRUTEFORCE_HPP
