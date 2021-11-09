#ifndef OPERATION_HPP
#define OPERATION_HPP

#define BOOLEXPR_ENUM_LIST_OP      \
    BOOLEXPR_ENUM_ACTION(FALSE)    \
    BOOLEXPR_ENUM_ACTION(NOR)      \
    BOOLEXPR_ENUM_ACTION(B_ANDN_A) \
    BOOLEXPR_ENUM_ACTION(NOT_A)    \
                                   \
    BOOLEXPR_ENUM_ACTION(A_ANDN_B) \
    BOOLEXPR_ENUM_ACTION(NOT_B)    \
    BOOLEXPR_ENUM_ACTION(XOR)      \
    BOOLEXPR_ENUM_ACTION(NAND)     \
                                   \
    BOOLEXPR_ENUM_ACTION(AND)      \
    BOOLEXPR_ENUM_ACTION(NXOR)     \
    BOOLEXPR_ENUM_ACTION(B)        \
    BOOLEXPR_ENUM_ACTION(A_CONS_B) \
                                   \
    BOOLEXPR_ENUM_ACTION(A)        \
    BOOLEXPR_ENUM_ACTION(B_CONS_A) \
    BOOLEXPR_ENUM_ACTION(OR)       \
    BOOLEXPR_ENUM_ACTION(TRUE)

#define BOOLEXPR_ENUM_ACTION(e) e,
enum class Op : unsigned { BOOLEXPR_ENUM_LIST_OP };
#undef BOOLEXPR_ENUM_ACTION

#if 0
#define BOOLEXPR_ENUM_ACTION(e) \
    case Op::e:                 \
        return #e;
constexpr const char *op_label(Op op) {
    switch (op) {
    BOOLEXPR_ENUM_LIST_OP
    }
}
#undef BOOLEXPR_ENUM_ACTION
#endif

constexpr const char *op_display_label(Op op) noexcept
{
    switch (op) {
    case Op::FALSE: return "false";
    case Op::NOR: return "or";
    case Op::B_ANDN_A: return "and";
    case Op::NOT_A: return "~";
    case Op::A_ANDN_B: return "and";
    case Op::NOT_B: return "~";
    case Op::XOR: return "xor";
    case Op::NAND: return "and";
    case Op::AND: return "and";
    case Op::NXOR: return "xor";
    case Op::B: return "";
    case Op::A_CONS_B: return "or";
    case Op::A: return "";
    case Op::B_CONS_A: return "or";
    case Op::OR: return "or";
    case Op::TRUE: return "true";
    }
    __builtin_unreachable();
}

[[nodiscard]] constexpr bool op_display_is_reversed(Op op) noexcept
{
    constexpr unsigned bits = 0b0010'0100'0010'0100;
    return bits >> static_cast<unsigned>(op) & 1;
}

[[nodiscard]] constexpr bool op_display_is_operand_compl(Op op) noexcept
{
    constexpr unsigned bits = 0b0010'1000'0001'0100u;
    return bits >> static_cast<unsigned>(op) & 1;
}

[[nodiscard]] constexpr bool op_is_trivial(Op op) noexcept
{
    constexpr unsigned bits = 0b1000'0000'0000'0001u;
    return bits >> static_cast<unsigned>(op) & 1;
}

[[nodiscard]] constexpr bool op_is_commutative(Op op) noexcept
{
    constexpr unsigned bits = 0b1100'0011'1100'0010;
    return bits >> static_cast<unsigned>(op) & 1;
}

[[nodiscard]] constexpr bool op_is_complement(Op op) noexcept
{
    constexpr unsigned bits = 0b0000'0010'1010'1010u;
    return bits >> static_cast<unsigned>(op) & 1;
}

[[nodiscard]] constexpr bool op_is_unary(Op op) noexcept
{
    constexpr unsigned bits = 0b0001'0100'0010'1000u;
    return bits >> static_cast<unsigned>(op) & 1;
}

#endif  // OPERATION_HPP
