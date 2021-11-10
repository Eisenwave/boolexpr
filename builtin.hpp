#ifndef BUILTIN_HPP
#define BUILTIN_HPP

#include "build.hpp"

namespace builtin {

// BOOLEXPR_HAS_BUILTIN(builtin):
//     Checks whether a builtin exists.
//     This only works on recent versions of clang, as this macro does not exist in gcc.
//     For gcc, the result is always 1 (true), optimistically assuming that the builtin exists.
#ifdef __has_builtin
#define BOOLEXPR_HAS_BUILTIN_HAS_BUILTIN
#define BOOLEXPR_HAS_BUILTIN(builtin) __has_builtin(builtin)
#else
#define BOOLEXPR_HAS_BUILTIN(builtin) 1
#endif

// int popcount(unsigned ...):
//     Counts the number of one-bits in an unsigned integer type.
#if defined(BOOLEXPR_GNU_OR_CLANG) && BOOLEXPR_HAS_BUILTIN(__builtin_popcount) && \
    BOOLEXPR_HAS_BUILTIN(__builtin_popcountl) && BOOLEXPR_HAS_BUILTIN(__builtin_popcountll)
#define BOOLEXPR_HAS_BUILTIN_POPCOUNT
inline int popcount(unsigned char x) noexcept
{
    return __builtin_popcount(x);
}

inline int popcount(unsigned short x) noexcept
{
    return __builtin_popcount(x);
}

inline int popcount(unsigned int x) noexcept
{
    return __builtin_popcount(x);
}

inline int popcount(unsigned long x) noexcept
{
    return __builtin_popcountl(x);
}

inline int popcount(unsigned long long x) noexcept
{
    return __builtin_popcountll(x);
}
#elif defined(BOOLEXPR_MSVC)
#define BOOLEXPR_HAS_BUILTIN_POPCOUNT
__forceinline int popcount(uint8_t x) noexcept
{
    return static_cast<int>(__popcnt16(x));
}

__forceinline int popcount(uint16_t x) noexcept
{
    return static_cast<int>(__popcnt16(x));
}

__forceinline int popcount(uint32_t x) noexcept
{
    return static_cast<int>(__popcnt(x));
}

__forceinline int popcount(uint64_t x) noexcept
{
    return static_cast<int>(__popcnt64(x));
}
#endif

// int clz(unsigned ...):
//     Counts the number of leading zeros in an unsigned integer type.
//     The result of countLeadingZeros(0) is undefined for all types.
#if defined(BOOLEXPR_GNU_OR_CLANG) && BOOLEXPR_HAS_BUILTIN(__builtin_clz) && BOOLEXPR_HAS_BUILTIN(__builtin_clzl) && \
    BOOLEXPR_HAS_BUILTIN(__builtin_clzll)
#define BOOLEXPR_HAS_BUILTIN_CLZ
inline int clz(unsigned char x) noexcept
{
    return __builtin_clz(x) - int{(sizeof(int) - sizeof(unsigned char)) * 8};
}

inline int clz(unsigned short x) noexcept
{
    return __builtin_clz(x) - int{(sizeof(int) - sizeof(unsigned short)) * 8};
}

inline int clz(unsigned int x) noexcept
{
    return __builtin_clz(x);
}

inline int clz(unsigned long x) noexcept
{
    return __builtin_clzl(x);
}

inline int clz(unsigned long long x) noexcept
{
    return __builtin_clzll(x);
}

#elif defined(BOOLEXPR_MSVC) && defined(BOOLEXPR_X86_OR_X64)
#define BOOLEXPR_HAS_BUILTIN_CLZ
__forceinline int clz(unsigned char x) noexcept
{
    return __lzcnt16(x) - 8;
}

__forceinline int clz(unsigned short x) noexcept
{
    return __lzcnt16(x);
}

__forceinline int clz(unsigned int x) noexcept
{
    return __lzcnt(x);
}

__forceinline int clz(unsigned long x) noexcept
{
    static_assert(sizeof(unsigned long) == sizeof(unsigned) || sizeof(unsigned long) == sizeof(uint64_t));

    if constexpr (sizeof(unsigned long) == sizeof(unsigned)) {
        return __lzcnt(static_cast<unsigned>(x));
    }
    else {
        return __lzcnt64(static_cast<uint64_t>(x));
    }
}

__forceinline int clz(uint64_t x) noexcept
{
    return __lzcnt64(x);
}
#endif

}  // namespace builtin

#endif  // BUILTIN_HPP
