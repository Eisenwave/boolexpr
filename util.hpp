#ifndef UTIL_HPP
#define UTIL_HPP

#include <type_traits>
#include <cstdint>

template <typename Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

constexpr unsigned log2floor(unsigned long long x) noexcept {
    return (x != 0) * (63u - static_cast<unsigned>(__builtin_clzll(x)));
}

constexpr bool is_pow_2(unsigned long long x) noexcept {
    return (x & (x - 1)) == 0 && x != 0;
}

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

template <auto X>
inline constexpr std::integral_constant<decltype(X), X> constant{};

#endif // UTIL_HPP
