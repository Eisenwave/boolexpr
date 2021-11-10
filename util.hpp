#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <string_view>
#include <type_traits>

template <typename Enum>
[[nodiscard]] constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

[[nodiscard]] constexpr unsigned log2floor(unsigned long long x) noexcept
{
    return (x != 0) * (63u - static_cast<unsigned>(__builtin_clzll(x)));
}

[[nodiscard]] constexpr bool is_pow_2(unsigned long long x) noexcept
{
    return (x & (x - 1)) == 0 && x != 0;
}

struct bitvec256 {
    std::uint64_t bits[4];

    [[nodiscard]] bitvec256(std::uint64_t bits) noexcept : bits{bits, 0, 0, 0} {}
};

[[nodiscard]] constexpr bool get_bit(const bitvec256 &vec, const std::uint64_t i) noexcept
{
    return vec.bits[i / 64] >> i % 64 & 1;
}

constexpr void set_bit_if(bitvec256 &vec, const std::uint64_t i, const bool condition) noexcept
{
    vec.bits[i / 64] |= std::uint64_t{condition} << i % 64;
}

[[nodiscard]] constexpr bool get_bit(const std::uint64_t vec, const std::uint64_t i) noexcept
{
    return vec >> i & 1;
}

constexpr void set_bit_if(std::uint64_t &vec, const std::uint64_t i, const bool condition) noexcept
{
    vec |= std::uint64_t{condition} << i;
}

template <typename T>
constexpr auto swap_if(T &a, T &b, const bool c) noexcept -> std::enable_if_t<std::is_fundamental_v<T>, void>
{
    // Only clang is able to optimize the following code well, meaning that it emits cmov instead of branches.
    // c is assumed to be unpredictable, so it is important to not emit those.
    // The alternative implementation is a a conditional XOR swap, which performs slightly better than the reference.
    // See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2187r5.pdf
#ifdef BOOLEXPR_CLANG
    const T tmp = a;
    a = c ? b : tmp;
    b = c ? tmp : b;
#elif 0
    const auto mask = -T{c};
    a ^= b & mask;
    b ^= a & mask;
    a ^= b & mask;
#else
    T tmp[]{a, b};
    a = tmp[c];
    b = tmp[not c];
#endif
}

[[nodiscard]] constexpr std::uint64_t tiny_string(const std::string_view str, const bool ignore_case = true) noexcept
{
    std::uint64_t result = 0;
    if (str.length() > 8) {
        return result;
    }
    for (const char c : str) {
        result <<= 8;
        result |= static_cast<std::uint8_t>(c | (ignore_case * 32));
    }
    return result;
}

static_assert(tiny_string("nOR") == ('n' << 16 | 'o' << 8 | 'r'));

template <auto X>
inline constexpr std::integral_constant<decltype(X), X> constant{};

#endif  // UTIL_HPP
