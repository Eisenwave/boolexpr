#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "compiler.hpp"
#include "constants.hpp"
#include "lexer.hpp"
#include "program.hpp"

namespace {

struct LaunchOptions {
    TruthTable table;
    std::size_t table_variables = 0;
    std::string expression_str;
    SymbolOrder symbol_order = SymbolOrder::LEX_ASCENDING;

    bool is_help = false;

    bool is_greedy = false;
    bool is_output_expr = false;
    bool is_output_program = false;

    bool is_tokenize = false;
    bool is_polish = false;
    bool is_compile = false;
    bool is_build_table = false;
};

[[nodiscard]] constexpr char parse_option(LaunchOptions &result, const std::string_view arg) noexcept
{
    if (arg.length() < 2 || arg[0] != '-') {
        return 0;
    }
    if (arg[1] == HELP_SHORT || arg == HELP_LONG) {
        result.is_help = true;
        return ' ';
    }

    if (arg[1] == EXPR_SHORT || arg == EXPR_LONG) {
        return 'e';
    }
    if (arg[1] == TABLE_SHORT || arg == TABLE_LONG) {
        return 't';
    }
    if (arg[1] == SYMBOL_ORDER_SHORT || arg == SYMBOL_ORDER_LONG) {
        return 's';
    }

    if (arg[1] == GREEDY_SHORT || arg == GREEDY_LONG) {
        result.is_greedy = true;
        return ' ';
    }
    if (arg[1] == OUTPUT_EXPR_SHORT || arg == OUTPUT_EXPR_LONG) {
        result.is_output_expr = true;
        return ' ';
    }
    if (arg[1] == OUTPUT_PROGRAM_SHORT || arg == OUTPUT_PROGRAM_LONG) {
        result.is_output_program = true;
        return ' ';
    }

    if (arg[1] == TOKENIZE_SHORT || arg == TOKENIZE_LONG) {
        result.is_tokenize = true;
        return ' ';
    }
    if (arg[1] == POLISH_SHORT || arg == POLISH_LONG) {
        result.is_polish = true;
        return ' ';
    }
    if (arg[1] == COMPILE_SHORT || arg == COMPILE_LONG) {
        result.is_compile = true;
        return ' ';
    }
    if (arg[1] == BUILD_TABLE_SHORT || arg == BUILD_TABLE_LONG) {
        result.is_build_table = true;
        return ' ';
    }
    return 0;
}

constexpr std::optional<SymbolOrder> order_parse(const std::string_view str) noexcept
{
    switch (tiny_string(str)) {
    case tiny_string("l"):
    case tiny_string("la"): return SymbolOrder::LEX_ASCENDING;
    case tiny_string("ld"): return SymbolOrder::LEX_DESCENDING;
    case tiny_string("a"):
    case tiny_string("aa"): return SymbolOrder::APPEARANCE_ASCENDING;
    case tiny_string("ad"): return SymbolOrder::APPEARANCE_DESCENDING;
    default: return std::nullopt;
    }
}

[[nodiscard]] LaunchOptions parse_program_args(int argc, char **argv)
{
    LaunchOptions result;

    char state = ' ';
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        switch (state) {
        case 'e': {
            result.expression_str = std::move(arg);
            state = 0;
            break;
        }

        case 't': {
            arg.erase(std::remove(arg.begin(), arg.end(), '.'), arg.end());
            if (not truth_table_is_valid(arg)) {
                std::exit(1);
            }
            result.table = truth_table_parse(arg);
            result.table_variables = arg.length();
            state = 0;
            break;
        }

        case 's': {
            std::optional<SymbolOrder> order = order_parse(arg);
            if (not order.has_value()) {
                std::cout << "Invalid symbol order \"" << arg << "\", must be l, la, ld, a, aa, or ad\n";
                std::exit(1);
            }
            result.symbol_order = *order;
            break;
        }

        default:
            state = parse_option(result, arg);
            if (state == 0) {
                std::cout << "Unrecognized option: " << arg << '\n';
                std::exit(1);
            }
        }
    }

    return result;
}

[[nodiscard]] int run_help(std::ostream &out)
{
    static constexpr int field_width = 64;
    static constexpr const char *indent = "    ";

    out << "Usage: OPTIONS...\n";

    const auto print = [&out](const char sht, const char *lng, const char *descr, const char *arg = "") {
        out << indent << '-' << sht << ',' << lng << arg
            << std::setw(static_cast<int>(field_width - std::strlen(lng) - std::strlen(arg))) << descr << '\n'
            << std::setw(0);
    };

    // clang-format off
    out << "\nHelp options:\n";
    print(HELP_SHORT, HELP_LONG, "show this help menu");

    out << "\nInput options:\n";
    print(EXPR_SHORT, EXPR_LONG, "input expression", " EXPRESSION");
    print(TABLE_SHORT, TABLE_LONG, "input truth table", " TABLE");

    out << "\nOutput flags:\n";
    print(GREEDY_SHORT, GREEDY_LONG, "greedily search for all optimal programs");
    print(OUTPUT_EXPR_SHORT, OUTPUT_EXPR_LONG, "print results as expression");
    print(OUTPUT_PROGRAM_SHORT, OUTPUT_PROGRAM_LONG, "print results as program");

    out << "\nAlternative output flags (for input expressions):\n";
    print(TOKENIZE_SHORT, TOKENIZE_LONG, "tokenize expression and print");
    print(POLISH_SHORT, POLISH_LONG, "print expression in reverse Polish notation");
    print(COMPILE_SHORT, POLISH_LONG, "print print boolean program of expression");
    print(BUILD_TABLE_SHORT, BUILD_TABLE_LONG, "build truth table of expression");

    out << '\n';
    out << "Truth table (regex: [10x.]+): " << DONT_CARE << " is \"don't care\", . is digit ignored\n";
    // clang-format on
    return EXIT_SUCCESS;
}

[[nodiscard]] int run_tokenize(const LaunchOptions &options)
{
    if (options.expression_str.empty()) {
        std::cout << "Tokenize option set but no expression to tokenize was given\n";
        return EXIT_FAILURE;
    }
    std::vector<Token> tokens = tokenize(options.expression_str);
    for (auto token : tokens) {
        std::cout << token << '\n';
    }
    return EXIT_SUCCESS;
}

[[nodiscard]] int run_polish(const LaunchOptions &options)
{
    if (options.expression_str.empty()) {
        std::cout << "Reverse polish output option set but no expression was given\n";
        return EXIT_FAILURE;
    }
    std::vector<Token> tokens = tokenize(options.expression_str);
    std::vector<Token> polish;
    if (not to_reverse_polish_notation(polish, tokens)) {
        return EXIT_FAILURE;
    }
    for (const auto &token : polish) {
        std::cout << token.value << ' ';
    }
    std::cout << '\n';
    return EXIT_SUCCESS;
}

[[nodiscard]] int run_output_table(const Program &program, const std::uint64_t table)
{
    static constexpr std::string_view FALLBACK_SYMBOLS[]{"A", "B", "C", "D", "E", "F"};

    for (std::size_t v = 0; v < std::size_t{1} << program.variables; ++v) {
        if (v != 0 && v % 4 == 0) {
            std::cout << '.';
        }
        std::cout << (table >> v & 1);
    }
    std::cout << "\n\n";

    std::size_t symbol_widths[VARIABLE_COUNT];
    for (std::size_t i = 0; i < VARIABLE_COUNT; ++i) {
        symbol_widths[i] = program.symbols[i].size();
    }

    for (std::size_t v = 0; v < program.variables; ++v) {
        bool has_symbol = symbol_widths[v] != 0;
        std::string_view symbol = has_symbol ? program.symbols[v] : FALLBACK_SYMBOLS[v];
        std::cout << ' ' << std::setw(static_cast<int>(symbol.size())) << symbol << " |";
    }
    std::cout << " =\n";

    for (std::size_t v = 0; v < std::size_t{1} << program.variables; ++v) {
        if (v % 4 == 0) {
            for (std::size_t v = 0; v < program.variables; ++v) {
                std::size_t len = symbol_widths[v] ? symbol_widths[v] : 1;
                std::cout << std::string(len + 2, '-') << "+";
            }
            std::cout << "---\n";
        }

        for (std::size_t i = 0; i < program.variables; ++i) {
            std::size_t len = symbol_widths[i] ? symbol_widths[i] : 1;
            std::cout << ' ' << std::setw(static_cast<int>(len)) << (v >> i & 1) << " |";
        }
        std::cout << ' ' << (table >> v & 1) << '\n';
    }

    return EXIT_SUCCESS;
}

struct PrintingProgramConsumer : public ProgramConsumer {
    Program program;
    LaunchOptions options;
    bool first = true;

    [[nodiscard]] PrintingProgramConsumer(const std::size_t variables,
                                          LaunchOptions options,
                                          Program *const original_program = nullptr) noexcept
        : program{variables}, options{std::move(options)}
    {
        if (original_program != nullptr) {
            program.symbols = original_program->symbols;
        }
    }

    void operator()(const Instruction *ins, const std::size_t count) final
    {
        program.clear();
        for (std::size_t i = 0; i < count; ++i) {
            program.push(ins[i]);
        }

        if (not first && options.is_output_program) {
            std::cout << '\n';
        }
        first = false;

        if (options.is_output_expr || (not options.is_output_expr && not options.is_output_program)) {
            print_program_as_expression(std::cout, program);
        }
        if (options.is_output_program) {
            std::cout << program;
        }

        program.clear();
    }
};

[[nodiscard]] int run_with_expression(const LaunchOptions &options)
{
    if (options.is_tokenize) {
        return run_tokenize(options);
    }
    if (options.is_polish) {
        return run_polish(options);
    }
    if (options.expression_str.empty()) {
        std::cout << "Compile option set but no expression was given\n";
        return EXIT_FAILURE;
    }

    const std::vector<Token> tokens = tokenize(options.expression_str);
    Program program = compile(tokens, options.symbol_order);

    if (options.is_compile) {
        std::cout << program;
        return EXIT_SUCCESS;
    }

    const TruthTable table = program.compute_truth_table();
    if (options.is_build_table) {
        return run_output_table(program, table.t);
    }

    PrintingProgramConsumer consumer{program.variables, options, &program};

    find_equivalent_programs(consumer, table, InstructionSet::C, program.variables, options.is_greedy);
    return EXIT_SUCCESS;
}

[[nodiscard]] int run_with_truth_table(const LaunchOptions &options)
{
    const std::size_t variables = log2floor(options.table_variables);
    PrintingProgramConsumer consumer{variables, options};

    find_equivalent_programs(consumer, options.table, InstructionSet::C, variables, options.is_greedy);
    return EXIT_SUCCESS;
}

[[nodiscard]] int run(const LaunchOptions &options)
{
    if (options.is_help) {
        return run_help(std::cout);
    }
    const bool has_expression = not options.expression_str.empty();
    const bool has_table = options.table_variables != 0;

    if (has_expression && has_table) {
        std::cout << "Conflicting inputs: both truth table and expression provided\n";
        return EXIT_FAILURE;
    }
    if (has_expression) {
        return run_with_expression(options);
    }
    if (has_table) {
        return run_with_truth_table(options);
    }

    std::cout << "No input provided\n";
    return EXIT_FAILURE;
}

}  // namespace

#include "bruteforce.hpp"

#define ASSERT(...) (static_cast<bool>(__VA_ARGS__) ? void() : throw 0)

int main(int argc, char **argv)
{
    CanonicalProgram p(6);
    ASSERT(p.try_push(Op::NOT_A, 4));
    ASSERT(p.try_push(Op::AND, 3, 6));
    ASSERT(p.try_push(Op::XOR, 2, 7));
    ASSERT(p.try_push(Op::NOT_A, 8));
    ASSERT(p.try_push(Op::AND, 1, 9));
    ASSERT(p.try_push(Op::OR, 0, 10));

    if (argc <= 1) {
        return run_help(std::cout);
    }
    LaunchOptions options = parse_program_args(argc, argv);
    return run(options);
}
