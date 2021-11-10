#include <algorithm>
#include <iostream>

#include "constants.hpp"
#include "util.hpp"

#include "truth_table.hpp"

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
    constexpr auto is_valid_table_char = [](unsigned char c) {
        return c == '1' || c == '0' || c == '*';
    };
    if (std::find_if_not(str.begin(), str.end(), is_valid_table_char) != str.end()) {
        std::cout << "Truth table must consist of only '0', '1' and '" << DONT_CARE << "'\n";
        return false;
    }
    return true;
}
