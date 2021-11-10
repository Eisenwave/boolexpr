#include "bruteforce.hpp"

namespace {

std::uint8_t distance_from_inputs(CanonicalProgram &program, const unsigned operand) noexcept
{
    return operand < 6 ? 0 : program[operand - 6].distance;
}

bool contains(const CanonicalProgram &program, const CanonicalInstruction ins) noexcept
{
    for (std::size_t i = 0; i < program.size(); ++i) {
        if (program[i] == ins) {
            return true;
        }
    }
    return false;
}

bool are_complement_of_same_input(const CanonicalProgram &program, unsigned a, unsigned b) noexcept
{
    swap_if(a, b, b < a);
    if (b < 6) {
        return false;
    }
    const CanonicalInstruction ins = program[b - 6];
    return static_cast<Op>(ins.op) == Op::NOT_A && program[b - 6].a == a;
}

template <bool Unary>
bool can_push(const CanonicalProgram &program, const CanonicalInstruction ins) noexcept
{
    if (program.empty()) {
        return true;
    }

    // 1 prevent non-canonical ordering of instructions
    // 1.1 enforce ascending order of distance from inputs of instructions
    if (ins.distance < program.top().distance) {
        return false;
    }
    // 1.2 enforce ascending order of instructions (as integral) for equally distant instructions
    if (ins.distance == program.top().distance && ins.to_integral() < program.top().to_integral()) {
        return false;
    }

    const Op op = static_cast<Op>(ins.op);
    // 2 prevent double negation (!!x)
    if (op == Op::NOT_A && ins.a >= 6 && program[ins.a - 6].op == ins.op) {
        return false;
    }
    if constexpr (not Unary) {
        // 3 prevent producing trivial results (x & !x => false, x | !x => true, x ^ !x => true, ...)
        if (are_complement_of_same_input(program, ins.a, ins.b)) {
            return false;
        }
    }

    // 4 prevent duplicate evaluations
    if (contains(program, ins)) {
        return false;
    }

    return true;
}

template <bool Unary>
bool try_push_impl(CanonicalProgram &program, const Op op, const unsigned a, const unsigned b) noexcept
{
    const auto base_dist = Unary ? distance_from_inputs(program, a)
                                 : std::max(distance_from_inputs(program, a), distance_from_inputs(program, b));
    const auto dist = static_cast<std::uint8_t>(base_dist + 1);

    const auto op8 = static_cast<std::uint8_t>(op);
    const auto a8 = static_cast<std::uint8_t>(a);
    const auto b8 = static_cast<std::uint8_t>(b);
    const CanonicalInstruction ins{op8, a8, b8, dist};

    if (not can_push<Unary>(program, ins)) {
        return false;
    }
    program.push(ins);
    return true;
}

}  // namespace

bool CanonicalProgram::try_push(const Op op, const unsigned a) noexcept
{
    return try_push_impl<true>(*this, op, a, 0);
}

bool CanonicalProgram::try_push(const Op op, const unsigned a, const unsigned b) noexcept
{
    return try_push_impl<false>(*this, op, a, b);
}
