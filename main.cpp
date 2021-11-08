#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "compiler.hpp"
#include "lexer.hpp"
#include "program.hpp"

namespace {

struct LaunchOptions {
    TruthTable table;
    std::size_t table_variables = 0;
    std::string expression_str;

    bool is_help = false;

    bool is_greedy = false;
    bool is_output_expr = false;
    bool is_output_program = false;

    bool is_tokenize = false;
    bool is_polish = false;
    bool is_compile = false;
    bool is_build_table = false;
};

constexpr auto HELP_SHORT = 'h';
constexpr auto HELP_LONG = "--help";
constexpr auto EXPR_SHORT = 'e';
constexpr auto EXPR_LONG = "--expr";
constexpr auto TABLE_SHORT = 't';
constexpr auto TABLE_LONG = "--table";
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

constexpr char parse_option(LaunchOptions &result, const std::string_view arg) noexcept
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

LaunchOptions parse_program_args(int argc, char **argv)
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
            << std::setw(static_cast<int>(field_width - std::strlen(lng) - std::strlen(arg)))
            << descr << '\n' << std::setw(0);
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

[[nodiscard]] int run_output_table(std::uint64_t table, const std::size_t variables)
{
    for (std::size_t v = 0; v < 1 << variables; ++v) {
        if (v != 0 && v % 4 == 0) {
            std::cout << '.';
        }
        std::cout << (table & 1);
        table >>= 1;
    }
    std::cout << '\n';

    return EXIT_SUCCESS;
}

[[nodiscard]] int print_results(const std::vector<Instruction> &results,
                                const std::size_t variables,
                                const LaunchOptions &options,
                                Program *const original_program = nullptr)
{
    Program program{variables};
    if (original_program != nullptr) {
        program.symbols = original_program->symbols;
    }
    bool first = true;
    for (Instruction ins : results) {
        if (ins == EOF_INSTRUCTION) {
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
            continue;
        }
        program.push(ins);
    }

    return EXIT_SUCCESS;
}

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
    Program program = compile(tokens);

    if (options.is_compile) {
        std::cout << program;
        return EXIT_SUCCESS;
    }

    const TruthTable table = program.compute_truth_table();
    if (options.is_build_table) {
        return run_output_table(table.t, program.variables);
    }
    const std::vector<Instruction> results =
        find_equivalent_programs(table, InstructionSet::C, program.variables, options.is_greedy);

    return print_results(results, program.variables, options, &program);
}

[[nodiscard]] int run_with_truth_table(const LaunchOptions &options)
{
    const std::size_t variables = log2floor(options.table_variables);
    const std::vector<Instruction> results =
        find_equivalent_programs(options.table, InstructionSet::C, variables, options.is_greedy);

    return print_results(results, variables, options);
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

int main(int argc, char **argv)
{
    if (argc <= 1) {
        return run_help(std::cout);
    }
    LaunchOptions options = parse_program_args(argc, argv);
    return run(options);
}
