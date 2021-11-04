#include <iostream>
#include <algorithm>

#include "lexer.hpp"
#include "compiler.hpp"
#include "program.hpp"

namespace {

struct LaunchOptions {
    TruthTable table;
    std::size_t table_variables = 0;
    std::string expression_str;

    bool is_help = false;
    bool is_tokenize = false;
    bool is_polish = false;
    bool is_compile = false;
    bool is_build_table = false;
};

constexpr char HELP_SHORT = 'h';
constexpr char EXPR_SHORT = 'e';
constexpr char TBLE_SHORT = 't';
constexpr char TKNZ_SHORT = 'Z';
constexpr char PLSH_SHORT = 'P';
constexpr char CMPL_SHORT = 'C';
constexpr char BTBL_SHORT = 'B';

constexpr char parse_option(LaunchOptions &result, const std::string_view arg) noexcept {
    if (arg.length() < 2 || arg[0] != '-') {
        return 0;
    }
    if (arg[1] == EXPR_SHORT || arg == "--expr") {
        return 'e';
    }
    else if (arg[1] == TBLE_SHORT || arg == "--table") {
        return 't';
    }
    else if (arg[1] == HELP_SHORT || arg == "--help") {
        result.is_help = true;
        return ' ';
    }
    else if (arg[1] == TKNZ_SHORT || arg == "--tokenize") {
        result.is_tokenize = true;
        return ' ';
    }
    else if (arg[1] == PLSH_SHORT || arg == "--polish") {
        result.is_polish = true;
        return ' ';
    }
    else if (arg[1] == CMPL_SHORT || arg == "--compile") {
        result.is_compile = true;
        return ' ';
    }
    else if (arg[1] == BTBL_SHORT || arg =="--build-table") {
        result.is_build_table = true;
        return ' ';
    }
    return 0;
}


LaunchOptions parse_program_args(int argc, char **argv) {
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

int run_help(std::ostream &out) {
    constexpr const char *indent = "    ";
    out << "Usage: OPTIONS...\n\n";

    out << indent << '-' << HELP_SHORT
        << ",--help              show this help menu\n";
    out << indent << '-' << EXPR_SHORT
        << ",--expr EXPRESSION   input expression (using C expression syntax)\n";
    out << indent << '-' << TBLE_SHORT
        << ",--table TABLE       input truth table (regex: [10*.]+)\n";
    out << indent << '-' << TKNZ_SHORT
        << ",--tokenize          don't optimize, but tokenize expression and print\n";
    out << indent << '-' << PLSH_SHORT
        << ",--polish            don't optimize, but print expression in reverse Polish notation\n";
    out << indent << '-' << CMPL_SHORT
        << ",--compile           don't optimize, but print print boolean program of expression\n";
    out << indent << '-' << BTBL_SHORT
        << ",--build-table       don't optimize, but build truth table of expression\n";

    out << '\n';
    out << indent << "Truth table: " << DONT_CARE << " is \"don't care\", . is digit ignored\n";
    return EXIT_SUCCESS;
}

[[nodiscard]]
int run_tokenize(const LaunchOptions &options) {
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

[[nodiscard]]
int run_polish(const LaunchOptions &options) {
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

[[nodiscard]]
int run_output_table(std::uint64_t table, const unsigned variables) {
    for (unsigned v = 0; v < 1 << variables; ++v) {
        if (v != 0 && v % 4 == 0) {
            std::cout << '.';
        }
        std::cout << (table & 1);
        table >>= 1;
    }
    std::cout << '\n';

    return EXIT_SUCCESS;
}

int run_with_expression(const LaunchOptions &options)
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
    const Program program = compile(tokens);

    if (options.is_compile) {
        std::cout << program;
        return EXIT_SUCCESS;
    }

    const TruthTable table = program_compute_truth_table(program);
    if (options.is_build_table) {
        return run_output_table(table.t, program.variables);
    }
    Program optimal_program = find_equivalent_program(table, InstructionSet::C, program.variables);
    optimal_program.symbols = std::move(program.symbols);
    std::cout << optimal_program;

    return EXIT_SUCCESS;
}

int run_with_truth_table(const LaunchOptions &options)
{
    unsigned variables = log2floor(options.table_variables);
    Program program = find_equivalent_program(options.table, InstructionSet::C, variables);
    std::cout << program;
    return EXIT_SUCCESS;
}

[[nodiscard]]
int run(const LaunchOptions &options) {
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

} // namespace

int main(int argc, char **argv)
{
    //print_playground();

    if (argc <= 1) {
        run_help(std::cout);
        return EXIT_FAILURE;
    }

    LaunchOptions options = parse_program_args(argc, argv);
    return run(options);
}
