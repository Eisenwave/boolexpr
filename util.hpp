#ifndef UTIL_HPP
#define UTIL_HPP

#include <type_traits>

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

template <auto X>
inline constexpr auto constant = X;

#endif // UTIL_HPP
