cmake_minimum_required(VERSION 3.5)

project(boolexpr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra -Werror -march=native")

add_executable(boolexpr
    main.cpp
    build.hpp
    builtin.hpp
    bruteforce.cpp
    bruteforce.hpp
    compiler.cpp
    compiler.hpp
    constants.hpp
    lexer.cpp
    lexer.hpp
    operation.hpp
    program.cpp
    program.hpp
    truth_table.cpp
    truth_table.hpp
    util.hpp)
