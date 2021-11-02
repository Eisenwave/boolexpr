cmake_minimum_required(VERSION 3.5)

project(boolexpr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(boolexpr
    main.cpp
    compiler.cpp
    compiler.hpp
    lexer.cpp
    lexer.hpp
    operation.hpp
    program.cpp
    program.hpp
    util.hpp)
