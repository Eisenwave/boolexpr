#ifndef TRUTH_TABLE_HPP
#define TRUTH_TABLE_HPP

#include <cstdint>
#include <string_view>

struct TruthTable {
    /// table where all don't cares are false
    std::uint64_t f;
    /// table where all don't cares are true
    std::uint64_t t;
};

[[nodiscard]] TruthTable truth_table_parse(std::string_view str) noexcept;

[[nodiscard]] bool truth_table_is_valid(std::string_view str) noexcept;

#endif  // TRUTH_TABLE_HPP
