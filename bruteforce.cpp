#include <optional>

#include "bruteforce.hpp"

namespace {

[[nodiscard]] std::uint8_t distance_from_inputs(const CanonicalProgram &program, const unsigned operand) noexcept
{
    return operand < 6 ? 0 : program[operand - 6].distance;
}

[[nodiscard]] bool is_breaking_canonical_dag_order(const CanonicalProgram &program,
                                                   const CanonicalInstruction ins) noexcept
{
    // 1.1 enforce ascending order of distance from inputs of instructions
    if (ins.distance < program.top().distance) {
        return true;
    }

    // 1.2 enforce ascending order of instructions (as integral) for equally distant instructions
    return ins.distance == program.top().distance && ins.to_integral() < program.top().to_integral();
}

[[nodiscard]] bool is_double_negation(const CanonicalProgram &program, const CanonicalInstruction ins) noexcept
{
    return static_cast<Op>(ins.op) == Op::NOT_A && ins.a >= 6 && static_cast<Op>(program[ins.a - 6].op) == Op::NOT_A;
}

[[nodiscard]] bool contains(const CanonicalProgram &program, const CanonicalInstruction ins) noexcept
{
    for (std::size_t i = 0; i < program.size(); ++i) {
        if (program[i] == ins) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool are_complement_of_same_input(const CanonicalProgram &program, unsigned a, unsigned b) noexcept
{
    swap_if(a, b, b < a);
    if (b < 6) {
        return false;
    }
    const CanonicalInstruction ins = program[b - 6];
    return static_cast<Op>(ins.op) == Op::NOT_A && program[b - 6].a == a;
}

template <bool Unary>
[[nodiscard]] bool is_program_unrevivable(const CanonicalProgram &program, const unsigned a, const unsigned b) noexcept
{
    using state_type = CanonicalProgram::state_type;

    const auto use = state_type{1} << a | state_type{not Unary} << b;

    const std::size_t total_used = popcount(program.used_instructions() | use);
    const std::size_t relevant = popcount(program.target_relevancy());
    const std::size_t remaining = program.target_length() - program.size();

    return relevant + program.size() + 1 > remaining + total_used;
}

template <bool Unary>
[[nodiscard]] bool can_push(const CanonicalProgram &program, const CanonicalInstruction ins) noexcept
{
    if (program.empty()) {
        return true;
    }

    // 1 prevent non-canonical ordering of instructions
    if (is_breaking_canonical_dag_order(program, ins)) {
        return false;
    }

    // 2 prevent double negation
    if (is_double_negation(program, ins)) {
        return false;
    }

    // 3 prevent producing trivial results (x & !x => false, x | !x => true, x ^ !x => true, ...)
    if (not Unary && are_complement_of_same_input(program, ins.a, ins.b)) {
        return false;
    }

    // 4 prevent duplicate evaluations
    if (contains(program, ins)) {
        return false;
    }

    // 5 prevent creation of zombie programs
    //   i.e. programs with so many dead (unused) instructions, that even after the addition of the given instruction,
    //   not all subexpressions of the program can be used
    if (is_program_unrevivable<Unary>(program, ins.a, ins.b)) {
        return false;
    }

    return true;
}

template <bool Unary>
[[nodiscard]] std::optional<CanonicalInstruction> can_push(const CanonicalProgram &program,
                                                           const Op op,
                                                           const unsigned a,
                                                           const unsigned b) noexcept
{
    const auto base_dist = Unary ? distance_from_inputs(program, a)
                                 : std::max(distance_from_inputs(program, a), distance_from_inputs(program, b));
    const auto dist = static_cast<std::uint8_t>(base_dist + 1);

    const auto op8 = static_cast<std::uint8_t>(op);
    const auto a8 = static_cast<std::uint8_t>(a);
    const auto b8 = static_cast<std::uint8_t>(b);
    const CanonicalInstruction ins{op8, a8, b8, dist};

    return can_push<Unary>(program, ins) ? std::optional{ins} : std::nullopt;
}

}  // namespace

bool CanonicalProgram::try_push(const Op op, const unsigned a) noexcept
{
    if (auto ins = can_push<true>(*this, op, a, 0)) {
        push(*ins);
        return true;
    }
    return false;
}

bool CanonicalProgram::try_push(const Op op, const unsigned a, const unsigned b) noexcept
{
    if (auto ins = can_push<false>(*this, op, a, b)) {
        push(*ins);
        return true;
    }
    return false;
}
