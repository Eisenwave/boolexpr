#include <algorithm>
#include <iostream>

#include "bruteforce.hpp"

#include "program.hpp"

namespace {

template <typename P>
[[nodiscard]] constexpr bool program_emulate_once(const P &program, typename P::state_type state) noexcept
{
    bool res = false;
    for (unsigned i = 0; i < program.size(); ++i) {
        const auto &ins = program[i];
        const bool a = get_bit(state, ins.a);
        const bool b = get_bit(state, ins.b);
        res = ins.op >> (a << 1 | b) & 1;
        set_bit_if(state, i + 6, res);
    }
    return res;
}

enum class TruthTableMode { TEST, FIND };

template <TruthTableMode Mode, typename P, typename V>
[[nodiscard]] constexpr std::uint64_t program_emulate(const P &program,
                                                      const V variables,
                                                      const TruthTable table [[maybe_unused]] = {}) noexcept
{
    // const auto limit = std::size_t{1} << variables;

    std::uint64_t result = 0;
    for (std::uint64_t v = 0; v < std::size_t{1} << variables; ++v) {
        const bool res = program_emulate_once(program, v);
        if constexpr (Mode == TruthTableMode::TEST) {
            const bool expected = (res ? table.f : table.t) >> v & 1;
            if (res != expected) {
                return false;
            }
        }
        else {
            result |= std::uint64_t{res} << v;
        }
    }
    return Mode == TruthTableMode::TEST ? 1u : result;
}

enum class FinderDecision : unsigned char {
    ABORT,
    KEEP_SEARCHING,
};

template <InstructionSet InstructionSet>
class ProgramFinder {
private:
    using program_type = CanonicalProgram;

    ProgramConsumer &consumer;
    program_type program;
    TruthTable table;
    std::size_t variables;
    bool found = false;
    bool greedy = false;

public:
    explicit ProgramFinder(ProgramConsumer &consumer,
                           const TruthTable table,
                           const std::size_t variables,
                           const std::size_t target_length,
                           const bool greedy) noexcept
        : consumer{consumer}, program{target_length}, table{table}, variables{variables}, greedy{greedy}
    {
    }

    void find_equivalent_program() noexcept
    {
        if (find_equivalent_trivial_program() || find_equivalent_mov_program()) {
            return;
        }

        for (std::size_t target_length = 1;; ++target_length) {
            program.reset(target_length);

            if (do_find_equivalent_program_switch()) {
                return;
            }
        }
    }

private:
    bool find_equivalent_trivial_program() noexcept
    {
        if (table.f == 0) {
            consumer(&FALSE_INSTRUCTION, 1);
            return found = true;
        }

        const auto mask = (variables == 6 ? 0 : (std::uint64_t{1} << (std::uint64_t{1} << variables))) - 1;
        if (table.t == mask) {
            consumer(&TRUE_INSTRUCTION, 1);
            return found = true;
        }
        return found;
    }

    bool find_equivalent_mov_program() noexcept
    {
        for (std::uint8_t i = 0; i < variables; ++i) {
            const std::uint8_t op8 = static_cast<std::uint8_t>(Op::A);
            program.push({op8, i, 0, 1});
            if (program_emulate<TruthTableMode::TEST>(program, variables, table)) {
                on_matching_emulation();
            }
            program.clear();
            if (found) {
                return true;
            }
        }
        return false;
    }

    template <typename V>
    FinderDecision do_find_equivalent_program(const V variables) noexcept;

    bool do_find_equivalent_program_switch() noexcept
    {
        switch (variables) {
        case 1: do_find_equivalent_program(constant<1u>); return found;
        case 2: do_find_equivalent_program(constant<2u>); return found;
        case 3: do_find_equivalent_program(constant<3u>); return found;
        case 4: do_find_equivalent_program(constant<4u>); return found;
        case 5: do_find_equivalent_program(constant<5u>); return found;
        case 6: do_find_equivalent_program(constant<6u>); return found;
        }
        __builtin_unreachable();
    }

    void on_matching_emulation() noexcept
    {
        thread_local std::array<Instruction, program_type::instruction_count> output_buffer;

        found = true;
        for (std::size_t i = 0; i < program.size(); ++i) {
            output_buffer[i] = static_cast<Instruction>(program[i]);
        }
        consumer(output_buffer.data(), program.size());
    }
};

template <InstructionSet InstructionSet>
template <typename V>
FinderDecision ProgramFinder<InstructionSet>::do_find_equivalent_program(const V variables) noexcept
{
    static_assert(std::is_convertible_v<V, unsigned>);

    if (program.size() == program.target_length) {
        if (program_emulate<TruthTableMode::TEST>(program, variables, table)) {
            on_matching_emulation();
            return greedy ? FinderDecision::KEEP_SEARCHING : FinderDecision::ABORT;
        }
        return FinderDecision::KEEP_SEARCHING;
    }

    const auto fix_operand = [variables](const unsigned o) {
        return o + (o >= variables) * (6 - variables);
    };

    for (std::uint64_t opcode = to_underlying(InstructionSet); opcode != 0; opcode >>= 4) {
        const Op op = static_cast<Op>(opcode & 0xf);
        const bool unary = op_is_unary(op);
        const bool commutative = op_is_commutative(op);

        for (unsigned a = 0; a < program.size() + variables; ++a) {
            const unsigned a_op = fix_operand(a);

            if (unary) {
                if (program.try_push(op, a_op)) {
                    if (do_find_equivalent_program(variables) == FinderDecision::ABORT) {
                        return FinderDecision::ABORT;
                    }
                    program.pop();
                }
                continue;
            }

            const unsigned b_start = commutative * (a + 1);
            for (unsigned b = b_start; b < program.size() + variables; ++b) {
                const unsigned b_op = fix_operand(b);
                if (program.try_push(op, a_op, b_op)) {
                    if (do_find_equivalent_program(variables) == FinderDecision::ABORT) {
                        return FinderDecision::ABORT;
                    }
                    program.pop();
                }
            }
        }
    }
    return FinderDecision::KEEP_SEARCHING;
}

std::ostream &do_print_program_as_expression(std::ostream &out, const Program &program, const std::size_t i)
{
    const auto print_operand = [&](const std::size_t j) -> std::ostream & {
        if (j < 6) {
            out << program.symbol(j, false);
        }
        else {
            do_print_program_as_expression(out, program, j - 6);
        };
        return out;
    };

    Instruction ins = program[i];
    Op op = static_cast<Op>(ins.op);
    if (op_is_trivial(op)) {
        out << op_display_label(op);
        return out;
    }
    unsigned a = ins.a;
    unsigned b = ins.b;

    if (op_display_is_reversed(op)) {
        std::swap(a, b);
    }
    if (op_is_complement(op)) {
        out << op_display_label(Op::NOT_A);
    }
    if (not op_is_unary(op)) {
        out << '(';
    }

    if (op_display_is_operand_compl(op) && not op_is_unary(op)) {
        out << op_display_label(Op::NOT_A);
        if (a >= 6) {
            out << '(';
        }
        out << op_display_label(Op::NOT_A);
        print_operand(a);
        if (a >= 6) {
            out << ')';
        }
    }
    else {
        print_operand(a);
    }

    if (not op_is_unary(op)) {
        out << ' ' << op_display_label(op) << ' ';
        print_operand(b) << ')';
    }
    return out;
}

}  // namespace

ProgramConsumer::~ProgramConsumer() = default;

void find_equivalent_programs(ProgramConsumer &consumer,
                              const TruthTable table,
                              const InstructionSet instructionSet,
                              const std::size_t variables,
                              const bool greedy)
{
    if (instructionSet != InstructionSet::C) {
        std::cout << "Only C instruction set is supported right now\n";
        std::exit(1);
    }

    ProgramFinder<InstructionSet::C> finder{consumer, table, variables, 0, greedy};
    finder.find_equivalent_program();
}

bool Program::is_equivalent(const TruthTable table) const noexcept
{
    return program_emulate<TruthTableMode::TEST>(*this, this->variables, table);
}

TruthTable Program::compute_truth_table() const noexcept
{
    const std::uint64_t table = program_emulate<TruthTableMode::FIND>(*this, static_cast<unsigned>(this->variables));
    return {table, table};
}

std::string Program::symbol(std::size_t i, bool input_prefix) const noexcept
{
    if (i < 6) {
        std::string result = input_prefix ? "@" : "";
        result += symbols[i].empty() ? std::string{static_cast<char>('A' + i)} : symbols[i];
        return result;
    }
    std::string result = "%";
    if ((i -= 6) < 10) {
        result += static_cast<char>('0' + i);
        return result;
    }
    if ((i -= 10) <= 26) {
        result += static_cast<char>('a' + i);
        return result;
    }
    if ((i -= 26) <= 26) {
        result += static_cast<char>('A' + i);
        return result;
    }
    result += "t";
    result += std::to_string(i);
    return result;
}

std::ostream &print_program_as_expression(std::ostream &out, const Program &program)
{
    return do_print_program_as_expression(out, program, program.size() - 1) << '\n';
}

std::ostream &print_instruction(std::ostream &out, const Instruction ins, const Program &program)
{
    constexpr const char *DISPLAY_NOT = op_display_label(Op::NOT_A);

    const Op op = static_cast<Op>(ins.op);
    const char *label = op_display_label(op);
    unsigned a = ins.a;
    unsigned b = ins.b;
    if (op_display_is_reversed(op)) {
        std::swap(a, b);
    }

    if (op_is_trivial(op)) {
        return out << label;
    }
    else if (op_is_unary(op)) {
        out << label;
        if (std::char_traits<char>::length(label) > 1) {
            out << ' ';
        }
        return out << program.symbol(a);
    }
    else {
        if (op_is_complement(op)) {
            out << DISPLAY_NOT << '(';
        }
        else if (op_display_is_operand_compl(op)) {
            out << DISPLAY_NOT;
            if constexpr (std::char_traits<char>::length(DISPLAY_NOT) > 1) {
                out << ' ';
            }
        }
        out << program.symbol(a) << ' ' << label << ' ' << program.symbol(b);
        if (op_is_complement(op)) {
            out << ')';
        }
        return out;
    }
}

std::ostream &operator<<(std::ostream &out, const Program &program)
{
    for (std::size_t i = 0; i < program.size(); ++i) {
        out << program.symbol(i + 6) << " = ";
        print_instruction(out, program[i], program);
        out << '\n';
    }
    return out;
}
