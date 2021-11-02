#include <iostream>
#include <algorithm>

#include "lexer.hpp"
#include "compiler.hpp"
#include "program.hpp"

namespace {

#if 0
enum Variable : unsigned {
    VAR_A,
    VAR_B,
    VAR_C,
    VAR_D,
    VAR_E,
    VAR_F
};

void print_playground() {
    Program p{
        {
            {static_cast<unsigned>(Op::AND), VAR_A, VAR_B, 0},
            {static_cast<unsigned>(Op::B_CONS_A), VAR_A, VAR_B, 0},
            {static_cast<unsigned>(Op::A_CONS_B), VAR_A, VAR_B, 0},
            {static_cast<unsigned>(Op::A_ANDN_B), VAR_A, VAR_B, 0},
            {static_cast<unsigned>(Op::B_ANDN_A), VAR_A, VAR_B, 0},
            {static_cast<unsigned>(Op::FALSE), 0, 0, 0},
            {static_cast<unsigned>(Op::NXOR), 6, VAR_C, 0},
            {static_cast<unsigned>(Op::A), 9, 0, 0},
            {static_cast<unsigned>(Op::NOT_B), 0, 10, 0},
        },
        9, 2
    };
    std::cout << p;
}
#endif

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

constexpr char parse_option(LaunchOptions &result, std::string_view arg) noexcept {
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
        << ",--help            : show this help menu";
    out << indent << '-' << EXPR_SHORT
        << ",--expr EXPRESSION : input expression (using C expression syntax)\n";
    out << indent << '-' << TBLE_SHORT
        << ",--table TABLE     : input truth table (regex: [10*.]+)\n";
    out << indent << '-' << TKNZ_SHORT
        << ",--tokenize        : don't optimize, but tokenize expression and print\n";
    out << indent << '-' << PLSH_SHORT
        << ",--polish          : don't optimize, but print expression in reverse Polish notation\n";
    out << indent << '-' << CMPL_SHORT
        << ",--compile         : don't optimize, but print print boolean program of expression\n";
    out << indent << '-' << BTBL_SHORT
        << ",--build-table     : don't optimize, but build truth table of expression\n";

    out << '\n';
    out << indent << "Truth table: " << DONT_CARE << " is \"don't care\", . is digit ignored\n";
    return 0;
}

[[nodiscard]]
int run_tokenize(const LaunchOptions &options) {
    if (options.expression_str.empty()) {
        std::cout << "Tokenize option set but no expression to tokenize was given\n";
        return 1;
    }
    std::vector<Token> tokens = tokenize(options.expression_str);
    for (auto token : tokens) {
        std::cout << token << '\n';
    }
    return 0;
}

[[nodiscard]]
int run_polish(const LaunchOptions &options) {
    if (options.expression_str.empty()) {
        std::cout << "Reverse polish output option set but no expression was given\n";
        return 1;
    }
    std::vector<Token> tokens = tokenize(options.expression_str);
    std::vector<Token> polish;
    if (not to_reverse_polish_notation(polish, tokens)) {
        return 1;
    }
    for (const auto &token : polish) {
        std::cout << token.value << ' ';
    }
    std::cout << '\n';
    return 0;
}

[[nodiscard]]
int run_build_table(const Program &program) {
    std::uint64_t table = program_compute_truth_table(program).t;
    for (unsigned v = 0; v < 1 << program.variables; ++v) {
        if (v != 0 && v % 4 == 0) {
            std::cout << '.';
        }
        std::cout << (table & 1);
        table >>= 1;
    }
    std::cout << '\n';
    return 0;
}

[[nodiscard]]
int run(const LaunchOptions &options) {
    if (options.is_help) {
        return run_help(std::cout);
    }

    if (options.table_variables != 0) {
        unsigned variables = log2floor(options.table_variables);
        Program program = find_equivalent_program(options.table, InstructionSet::C, variables);
        std::cout << program << '\n';
        return 0;
    }

    if (options.is_tokenize) {
        return run_tokenize(options);
    }
    if (options.is_polish) {
        return run_polish(options);
    }
    if (options.expression_str.empty()) {
        std::cout << "Compile option set but no expression was given\n";
        return 1;
    }

    std::vector<Token> tokens = tokenize(options.expression_str);
    Program program = compile(tokens);

    if (options.is_compile) {
        std::cout << program;
        return 0;
    }
    if (options.is_build_table) {
        return run_build_table(program);
    }

    std::cout << "No input provided\n";
    return 1;
}

} // namespace

int main(int argc, char **argv)
{
    //print_playground();

    if (argc <= 1) {
        run_help(std::cout);
        return 1;
    }

    LaunchOptions options = parse_program_args(argc, argv);
    return run(options);
}
