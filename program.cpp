#include <algorithm>
#include <iostream>

#include "program.hpp"

namespace  {

enum class TruthTableMode {
    TEST,
    FIND
};

// FIXME: make template where state can be larger bit-vector to handle long input programs
constexpr bool program_emulate_once(const Program &program, std::uint64_t state) noexcept
{
    bool res = false;
    for (unsigned i = 0; i < program.length; ++i) {
        const Instruction ins = program.instructions[i];
        const bool a = state >> ins.a & 1;
        const bool b = state >> ins.b & 1;
        res = ins.op >> (a << 1 | b) & 1;
        state |= std::uint64_t{res} << (i + 6);
    }
    return res;
}

template <TruthTableMode Mode, unsigned Variables>
constexpr std::uint64_t program_emulate(const Program &program,
                                        const TruthTable table [[maybe_unused]] = {}) noexcept
{
    std::uint64_t result = 0;
    for (unsigned v = 0; v < 1 << Variables; ++v) {
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

template <TruthTableMode Mode>
constexpr std::uint64_t program_emulate(const Program &program,
                                        const TruthTable table [[maybe_unused]] = {}) noexcept
{
    switch (program.variables) {
    case 1: return program_emulate<Mode, 1>(program, table);
    case 2: return program_emulate<Mode, 2>(program, table);
    case 3: return program_emulate<Mode, 3>(program, table);
    case 4: return program_emulate<Mode, 4>(program, table);
    case 5: return program_emulate<Mode, 5>(program, table);
    case 6: return program_emulate<Mode, 6>(program, table);
    }
    __builtin_unreachable();
}

template <InstructionSet InstructionSet, unsigned Variables>
constexpr bool do_find_equivalent_program(Program &program, const TruthTable table, const unsigned len) noexcept
{
    constexpr auto fix_operand = [](unsigned o) {
        return o + (o >= Variables) * (6 - Variables);
    };

    if (program.length == len) {
        return program_emulate<TruthTableMode::TEST, Variables>(program, table);
    }

    for (std::uint64_t opcode = to_underlying(InstructionSet); opcode != 0; opcode >>= 4) {
        const Op op = static_cast<Op>(opcode & 0xf);
        const bool unary = op_is_unary(op);
        const bool commutative = op_is_commutative(op);

        for (unsigned a = 0; a < program.length + Variables; ++a) {
            const unsigned a_op = fix_operand(a);

            if (unary) {
                program.push({static_cast<std::uint8_t>(op), static_cast<std::uint8_t>(a_op), 0, 0});
                if (do_find_equivalent_program<InstructionSet, Variables>(program, table, len)) {
                    return true;
                }
                program.pop();
                continue;
            }

            const unsigned b_start = commutative * (a + 1);
            for (unsigned b = b_start; b < program.length + Variables; ++b) {
                const unsigned b_op = fix_operand(b);
                program.push({static_cast<std::uint8_t>(op),
                              static_cast<std::uint8_t>(a_op),
                              static_cast<std::uint8_t>(b_op), 0});
                if (do_find_equivalent_program<InstructionSet, Variables>(program, table, len)) {
                    return true;
                }
                program.pop();
            }

        }
    }
    return false;
}

template <InstructionSet InstructionSet, unsigned Variables>
Program find_equivalent_program(const TruthTable table) noexcept
{
    Program p{{}, 0, Variables, {}};

    for (unsigned len = 1; ; ++len) {
        p.length = 0;

        if (do_find_equivalent_program<InstructionSet, Variables>(p, table, len)) {
            return p;
        }
    }

    return p;
}

template <InstructionSet InstructionSet>
Program find_equivalent_program(const TruthTable table, unsigned variables) noexcept
{
    switch (variables) {
    case 1: return find_equivalent_program<InstructionSet, 1>(table);
    case 2: return find_equivalent_program<InstructionSet, 2>(table);
    case 3: return find_equivalent_program<InstructionSet, 3>(table);
    case 4: return find_equivalent_program<InstructionSet, 4>(table);
    case 5: return find_equivalent_program<InstructionSet, 5>(table);
    case 6: return find_equivalent_program<InstructionSet, 6>(table);
    }
    __builtin_unreachable();
}



} // namespace

TruthTable truth_table_parse(std::string_view str) noexcept
{
    std::uint64_t f = 0, t = 0;
    for (unsigned i = 0; i < str.length(); ++i) {
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

bool truth_table_is_valid(std::string_view str) noexcept
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

Program find_equivalent_program(const TruthTable table,
                                          const InstructionSet instructionSet,
                                          const unsigned variables) noexcept {
    if (instructionSet != InstructionSet::C) {
        std::cout << "Only C instruction set is supported right now\n";
        std::exit(1);
    }
    return find_equivalent_program<InstructionSet::C>(table, variables);
}

bool program_is_equivalent(const Program &program, const TruthTable table) noexcept {
    return program_emulate<TruthTableMode::TEST>(program, table);
}

TruthTable program_compute_truth_table(const Program &program) noexcept {
    std::uint64_t table = program_emulate<TruthTableMode::FIND>(program);
    return {table, table};
}

std::string Program::symbol(std::size_t i) const noexcept
{
    if (i < 6) {
        return std::string{'@'} + (symbols[i].empty() ? std::string{static_cast<char>('A' + i)} : symbols[i]);
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

std::ostream &print_instruction(std::ostream &out, Instruction ins, const Program &p)
{
    constexpr const char *DISPLAY_NOT = op_display_label(Op::NOT_A);

    const Op op = static_cast<Op>(ins.op);
    const char *label = op_display_label(op);
    unsigned a = ins.a;
    unsigned b = ins.b;
    if (op_is_reversed_for_display(op)) {
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
        return out << p.symbol(a);
    }
    else {
        if (op_is_complement(op)) {
            out << DISPLAY_NOT << "(";
        }
        else if (op_is_operand_compl(op)) {
            out << DISPLAY_NOT;
            if constexpr (std::char_traits<char>::length(DISPLAY_NOT) > 1) {
                out << ' ';
            }
        }
        out << p.symbol(a) << ' ' << label << ' ' << p.symbol(b);
        if (op_is_complement(op)) {
            out << ')';
        }
        return out;
    }
}

std::ostream& operator<<(std::ostream &out, const Program &p)
{
    for (unsigned i = 0; i < p.length; ++i) {
        out << p.symbol(i + 6) << " = ";
        print_instruction(out, p.instructions[i], p);
        out << '\n';
    }
    return out;
}
