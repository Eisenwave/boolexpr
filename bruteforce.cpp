#include <optional>

#include "bruteforce.hpp"

namespace {

[[nodiscard]] std::uint8_t distance_from_inputs(const CanonicalProgram &program, const unsigned operand) noexcept
{
    return operand < VARIABLE_COUNT ? 0 : program[operand - VARIABLE_COUNT].distance;
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
    return static_cast<Op>(ins.op) == Op::NOT_A && ins.a >= VARIABLE_COUNT &&
           static_cast<Op>(program[ins.a - VARIABLE_COUNT].op) == Op::NOT_A;
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
    // FIXME given operand generation, can b ever be lower than a ? (probably not)
    swap_if(a, b, b < a);
    if (b < VARIABLE_COUNT) {
        return false;
    }
    const CanonicalInstruction ins = program[b - 6];
    return static_cast<Op>(ins.op) == Op::NOT_A && program[b - 6].a == a;
}

[[nodiscard]] bool is_using_operand(const CanonicalProgram &program,
                                    const std::uint8_t operand,
                                    const CanonicalInstruction ins) noexcept
{
    const auto check_use = [&program, operand](std::uint8_t other) {
        return other == operand ||
               (other >= VARIABLE_COUNT && is_using_operand(program, operand, program[other - VARIABLE_COUNT]));
    };

    return check_use(ins.a) || (not op_is_unary(static_cast<Op>(ins.op)) && check_use(ins.b));
}

[[nodiscard]] bool is_suboptimal_and_or(const CanonicalProgram &program, const CanonicalInstruction ins) noexcept
{
    const auto op = static_cast<Op>(ins.op);
    return (op == Op::AND || op == Op::OR) && ins.b >= VARIABLE_COUNT &&
           is_using_operand(program, ins.a, program[ins.b - VARIABLE_COUNT]);
}

[[nodiscard]] bool is_non_canonical_commutative(const CanonicalProgram &program,
                                                const CanonicalInstruction ins) noexcept
{
    const auto op = static_cast<Op>(ins.op);
    if (not op_is_commutative(op) || ins.b < VARIABLE_COUNT) {
        return false;
    }
    const CanonicalInstruction other = program[ins.b - VARIABLE_COUNT];
    if (ins.op != other.op) {
        return false;
    }

    const std::uint8_t a = ins.a;
    const std::uint8_t b = other.a;
    const std::uint8_t c = other.b;

    // already in canonical order, nothing left to check
    if (a < b) {
        return false;
    }

    // if the instructions aren't equidistant,
    // then requiring canonical ordering could also make valid programs impossible (possibly ?)
    const std::uint8_t dist_a = distance_from_inputs(program, a);
    const std::uint8_t dist_b = distance_from_inputs(program, b);
    const std::uint8_t dist_c = distance_from_inputs(program, c);

    return dist_a == dist_b && dist_a == dist_c;
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

    // 4 non-canonical ordering of commutative operations, e.g. C and (A and B)
    //   only A and (B and C) is allowed
    if (not Unary && is_non_canonical_commutative(program, ins)) {
        return false;
    }

    // 5 suboptimal use of and/or, e.g. A and SubExpr where A appears in SubExpr
    if (not Unary && is_suboptimal_and_or(program, ins)) {
        return false;
    }

    // 6 prevent creation of zombie programs
    //   i.e. programs with so many dead (unused) instructions, that even after the addition of the given instruction,
    //   not all subexpressions of the program can be used
    if (is_program_unrevivable<Unary>(program, ins.a, ins.b)) {
        return false;
    }

    // 7 prevent duplicate evaluations
    if (contains(program, ins)) {
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
