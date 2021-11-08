#include <algorithm>
#include <iostream>

#include "program.hpp"

namespace  {

template <typename P>
constexpr bool program_emulate_once(const P &program, typename P::state_type state) noexcept
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

enum class TruthTableMode {
    TEST,
    FIND
};

template <TruthTableMode Mode, typename P, typename V>
constexpr std::uint64_t program_emulate(const P &program,
                                        const V variables,
                                        const TruthTable table [[maybe_unused]] = {}) noexcept
{
    std::uint64_t result = 0;
    for (std::uint64_t v = 0; v < 1 << variables; ++v) {
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

struct CanonicalInstruction {
    /// the truth table of the operation
    std::uint8_t op;
    /// the index of the first operand, where the first six values are reserved for the program inputs
    std::uint8_t a;
    /// the index of the second operand, where the first six values are reserved for the program inputs
    std::uint8_t b;

    constexpr explicit operator Instruction() const noexcept {
        return {op, a, b};
    }
};

struct CanonicalProgram {
    using instruction_type = CanonicalInstruction;
    using state_type = std::uint64_t;
    using size_type = std::size_t;
    static constexpr size_type instruction_count = 58;

private:
    std::array<CanonicalInstruction, instruction_count> instructions;
    size_type length = 0;
public:
    size_type target_length;

    explicit CanonicalProgram(const size_type target_length) : target_length{target_length} {}

    constexpr size_type size() const noexcept {
        return length;
    }

    constexpr instruction_type operator[](const size_type i) const noexcept {
        return instructions[i];
    }

    constexpr void push(CanonicalInstruction ins) noexcept {
        instructions[length++] = ins;
    }

    constexpr void push(Op op, unsigned a, unsigned b) noexcept {
        push({static_cast<std::uint8_t>(op), static_cast<std::uint8_t>(a), static_cast<std::uint8_t>(b)});
    }

    constexpr CanonicalInstruction &top() noexcept {
        return instructions[length - 1];
    }

    constexpr const CanonicalInstruction &top() const noexcept {
        return instructions[length - 1];
    }

    constexpr void reset(const size_type target_length) noexcept {
        clear();
        this->target_length = target_length;
    }

    constexpr void clear() noexcept {
        length = 0;
    }

    constexpr void pop() noexcept {
        --length;
    }
};

enum class FinderDecision : unsigned char {
    ABORT,
    KEEP_SEARCHING,
};

template <InstructionSet InstructionSet>
class ProgramFinder {
private:

    CanonicalProgram program;
    TruthTable table;
    std::size_t variables;
    bool found = false;
    bool greedy = false;
    std::vector<Instruction> result;

public:
    explicit ProgramFinder(const TruthTable table,
                           const std::size_t variables,
                           const std::size_t target_length,
                           const bool greedy) noexcept
        : program{target_length}
        , table{table}
        , variables{variables}
        , greedy{greedy} {}


    std::vector<Instruction> find_equivalent_program() noexcept
    {
        for (std::size_t target_length = 1; ; ++target_length) {
            program.reset(target_length);

            if (do_find_equivalent_program_switch()) {
                return std::move(result);
            }
        }
        // technically unreachable, keep this line for refactoring safety
        return std::move(result);
    }

private:
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

    void on_matching_emulation() noexcept {
        found = true;
        result.reserve(result.size() + program.size() + 1);
        for (std::size_t i = 0; i < program.size(); ++i) {
            result.push_back(static_cast<Instruction>(program[i]));
        }
        result.push_back(EOF_INSTRUCTION);
    }
};

template <InstructionSet InstructionSet>
template <typename V>
FinderDecision ProgramFinder<InstructionSet>::do_find_equivalent_program(const V variables) noexcept
{
    static_assert (std::is_convertible_v<V, unsigned>);

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
                program.push(op, a_op, 0);
                if (do_find_equivalent_program(variables) == FinderDecision::ABORT) {
                    return FinderDecision::ABORT;
                }
                program.pop();
                continue;
            }

            const unsigned b_start = commutative * (a + 1);
            for (unsigned b = b_start; b < program.size() + variables; ++b) {
                const unsigned b_op = fix_operand(b);
                program.push(op, a_op, b_op);
                if (do_find_equivalent_program(variables) == FinderDecision::ABORT) {
                    return FinderDecision::ABORT;
                }
                program.pop();
            }

        }
    }
    return FinderDecision::KEEP_SEARCHING;
}

std::ostream &do_print_program_as_expression(std::ostream &out, const Program &program, const std::size_t i)
{
    const auto print_operand = [&](const std::size_t j) -> std::ostream& {
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
    bool is_simple_unary = op_is_unary(op) && a < 6;

    if (op_display_is_reversed(op)) {
        std::swap(a, b);
    }
    if (op_is_complement(op)) {
        out << op_display_label(Op::NOT_A);
        if (not is_simple_unary) {
            out << '(';
        }
    }

    if (op_display_is_operand_compl(op) && not op_is_unary(op)) {
        out << op_display_label(Op::NOT_A);
        if (a >= 6) {
            out << '(';
        }
        out << op_display_label(Op::NOT_A);
        print_operand(a);
        if (a >= 6) {
            out << '(';
        }
    }
    else {
        print_operand(a);
    }

    if (not op_is_unary(op)) {
        out << ' ' << op_display_label(op) << ' ';
        print_operand(b);
    }
    if (op_is_complement(op) && not is_simple_unary) {
        out << ')';
    }
    return out;
}

} // namespace

std::vector<Instruction> find_equivalent_programs(const TruthTable table,
                                                 const InstructionSet instructionSet,
                                                 const std::size_t variables,
                                                 const bool greedy) noexcept {
    if (instructionSet != InstructionSet::C) {
        std::cout << "Only C instruction set is supported right now\n";
        std::exit(1);
    }

    ProgramFinder<InstructionSet::C> finder{table, variables, 0, greedy};
    return finder.find_equivalent_program();
}

TruthTable truth_table_parse(const std::string_view str) noexcept
{
    std::uint64_t f = 0, t = 0;
    for (std::uint64_t i = 0; i < str.length(); ++i) {
        if (str[i] == '1') {
            f |= std::uint64_t{1} << i;
            t |= std::uint64_t{1} << i;
        }
        else if (str[i] == DONT_CARE) {
            t |= std::uint64_t{1} << i;
        }
    }
    return {f, t};
}

bool truth_table_is_valid(const std::string_view str) noexcept
{
    if (str.length() > 64) {
        std::cout << "Truth table is too long (at most 64 entries supported)\n";
        return false;
    }
    if (not is_pow_2(str.length())) {
        std::cout << "Length of truth table has to be a power of two, is " << str.length() << '\n';
        return false;
    }
    constexpr auto is_valid_table_char = [](unsigned char c){
        return c == '1' || c == '0' || c == '*';
    };
    if (std::find_if_not(str.begin(), str.end(), is_valid_table_char) != str.end()) {
        std::cout << "Truth table must consist of only '0', '1' and '" << DONT_CARE << "'\n";
        return false;
    }
    return true;
}

bool Program::is_equivalent(const TruthTable table) const noexcept {
    return program_emulate<TruthTableMode::TEST>(*this, this->variables, table);
}

TruthTable Program::compute_truth_table() const noexcept {
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
            out << DISPLAY_NOT << "(";
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

std::ostream& operator<<(std::ostream &out, const Program &program)
{
    for (std::size_t i = 0; i < program.size(); ++i) {
        out << program.symbol(i + 6) << " = ";
        print_instruction(out, program[i], program);
        out << '\n';
    }
    return out;
}
