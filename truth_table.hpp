#ifndef TRUTH_TABLE_HPP
#define TRUTH_TABLE_HPP

#include <cstdint>
#include <string_view>

#include "constants.hpp"
#include "util.hpp"

struct TruthTable {
    [[nodiscard]] static bool is_well_formed(std::string_view str) noexcept;
    [[nodiscard]] static TruthTable parse(std::string_view str) noexcept;

    /// table where all don't cares are false
    std::uint64_t f;
    /// table where all don't cares are true
    std::uint64_t t;

    [[nodiscard]] constexpr std::uint64_t dont_care() const noexcept
    {
        return f ^ t;
    }

    [[nodiscard]] constexpr std::uint64_t mandatory() const noexcept
    {
        return f & t;
    }

    [[nodiscard]] constexpr std::uint64_t relevancy(const std::uint64_t variables) const noexcept
    {
        std::uint64_t result = 0;
        for (std::uint64_t i = 0; i < variables; ++i) {
            auto [lo, hi] = split_bits_alternating(mandatory(), i);
            result |= unsigned{lo != hi} << i;
        }
        return result;
    }
};

#endif  // TRUTH_TABLE_HPP
