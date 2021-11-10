#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

constexpr unsigned VARIABLE_COUNT = 6;
constexpr char DONT_CARE = 'x';

constexpr auto HELP_SHORT = 'h';
constexpr auto HELP_LONG = "--help";
constexpr auto EXPR_SHORT = 'e';
constexpr auto EXPR_LONG = "--expr";
constexpr auto TABLE_SHORT = 't';
constexpr auto TABLE_LONG = "--table";
constexpr auto SYMBOL_ORDER_SHORT = 's';
constexpr auto SYMBOL_ORDER_LONG = "--symbol-order";
constexpr auto GREEDY_SHORT = 'g';
constexpr auto GREEDY_LONG = "--greedy";
constexpr auto OUTPUT_EXPR_SHORT = 'x';
constexpr auto OUTPUT_EXPR_LONG = "--print-expr";
constexpr auto OUTPUT_PROGRAM_SHORT = 'p';
constexpr auto OUTPUT_PROGRAM_LONG = "--print-program";
constexpr auto TOKENIZE_SHORT = 'Z';
constexpr auto TOKENIZE_LONG = "--tokenize";
constexpr auto POLISH_SHORT = 'P';
constexpr auto POLISH_LONG = "--polish";
constexpr auto COMPILE_SHORT = 'C';
constexpr auto COMPILE_LONG = "--compile";
constexpr auto BUILD_TABLE_SHORT = 'B';
constexpr auto BUILD_TABLE_LONG = "--build-table";

#endif  // CONSTANTS_HPP
