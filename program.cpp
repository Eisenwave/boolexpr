#include <algorithm>
#include <iostream>

#include "program.hpp"

namespace  {

struct CanonicalInstruction {
    /// the truth table of the operation
    std::uint8_t op;
    /// the index of the first operand, where the first six values are reserved for the program inputs
    std::uint8_t a;
    /// the index of the second operand, where the first six values are reserved for the program inputs
    std::uint8_t b;
};

struct CanonicalProgram {
    static constexpr std::size_t instruction_count = 58;

    std::array<Instruction, instruction_count> instructions;
    unsigned length;

    constexpr void push(Instruction ins) noexcept {
        instructions[length++] = ins;
    }

    constexpr void push(Op op, unsigned a, unsigned b) noexcept {
        push({static_cast<std::uint8_t>(op), static_cast<std::uint8_t>(a), static_cast<std::uint8_t>(b)});
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

    Program to_program(unsigned variables) {
        static_assert (CanonicalProgram::instruction_count <= Program::instruction_count);
        Program result{{}, length, variables, {}};
        std::copy(instructions.begin(), instructions.end(), result.instructions.begin());
        return result;;
    }
};

enum class TruthTableMode {
    TEST,
    FIND
};


struct bitvec256 {
    std::uint64_t bits[4];

    bitvec256(std::uint64_t bits) : bits{bits, 0, 0, 0} {}
};

constexpr bool get_bit(const bitvec256 &vec, const std::uint64_t i) noexcept
{
    return vec.bits[i / 64] >> i % 64 & 1;
}

constexpr void set_bit_if(bitvec256 &vec, const std::uint64_t i, const bool condition) noexcept
{
    vec.bits[i / 64] |= std::uint64_t{condition} << i % 64;
}

constexpr bool get_bit(const std::uint64_t vec, const std::uint64_t i) noexcept
{
    return vec >> i & 1;
}

constexpr void set_bit_if(std::uint64_t &vec, const std::uint64_t i, const bool condition) noexcept
{
    vec |= std::uint64_t{condition} << i;
}

template <typename P>
using program_state_type = std::conditional_t<(P::instruction_count <= 58), std::uint64_t, bitvec256>;

template <typename P>
constexpr bool program_emulate_once(const P &program, program_state_type<P> state) noexcept
{
    bool res = false;
    for (unsigned i = 0; i < program.length; ++i) {
        const Instruction ins = program.instructions[i];
        const bool a = get_bit(state, ins.a);
        const bool b = get_bit(state, ins.b);
        res = ins.op >> (a << 1 | b) & 1;
        set_bit_if(state, i + 6, res);
    }
    return res;
}

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

template <InstructionSet InstructionSet, typename V>
constexpr bool do_find_equivalent_program(CanonicalProgram &program,
                                          const TruthTable table,
                                          const V variables,
                                          const unsigned len) noexcept
{
    const auto fix_operand = [variables](const unsigned o) {
        return o + (o >= variables) * (6 - variables);
    };

    if (program.length == len) {
        return program_emulate<TruthTableMode::TEST>(program, variables, table);
    }

    for (std::uint64_t opcode = to_underlying(InstructionSet); opcode != 0; opcode >>= 4) {
        const Op op = static_cast<Op>(opcode & 0xf);
        const bool unary = op_is_unary(op);
        const bool commutative = op_is_commutative(op);

        for (unsigned a = 0; a < program.length + variables; ++a) {
            const unsigned a_op = fix_operand(a);

            if (unary) {
                program.push(op, a_op, 0);
                if (do_find_equivalent_program<InstructionSet>(program, table, variables, len)) {
                    return true;
                }
                program.pop();
                continue;
            }

            const unsigned b_start = commutative * (a + 1);
            for (unsigned b = b_start; b < program.length + variables; ++b) {
                const unsigned b_op = fix_operand(b);
                program.push(op, a_op, b_op);
                if (do_find_equivalent_program<InstructionSet>(program, table, variables, len)) {
                    return true;
                }
                program.pop();
            }

        }
    }
    return false;
}

template <InstructionSet InstructionSet>
constexpr bool do_find_equivalent_program_switch(CanonicalProgram &program,
                                                 const TruthTable table,
                                                 const unsigned variables,
                                                 const unsigned len) noexcept
{
    switch (variables) {
    case 1: return do_find_equivalent_program<InstructionSet>(program, table, constant<1u>, len);
    case 2: return do_find_equivalent_program<InstructionSet>(program, table, constant<2u>, len);
    case 3: return do_find_equivalent_program<InstructionSet>(program, table, constant<3u>, len);
    case 4: return do_find_equivalent_program<InstructionSet>(program, table, constant<4u>, len);
    case 5: return do_find_equivalent_program<InstructionSet>(program, table, constant<5u>, len);
    case 6: return do_find_equivalent_program<InstructionSet>(program, table, constant<6u>, len);
    }
    __builtin_unreachable();
}

template <InstructionSet InstructionSet>
Program find_equivalent_program(const TruthTable table, const unsigned variables) noexcept
{
    CanonicalProgram p{{}, 0};

    for (unsigned len = 1; ; ++len) {
        p.length = 0;

        if (do_find_equivalent_program_switch<InstructionSet>(p, table, variables, len)) {
            return p.to_program(variables);
        }
    }

    return p.to_program(variables);
}

} // namespace

TruthTable truth_table_parse(const std::string_view str) noexcept
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
    return program_emulate<TruthTableMode::TEST>(program, program.variables, table);
}

TruthTable program_compute_truth_table(const Program &program) noexcept {
    std::uint64_t table = program_emulate<TruthTableMode::FIND>(program, program.variables);
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

std::ostream &print_instruction(std::ostream &out, const Instruction ins, const Program &p)
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
