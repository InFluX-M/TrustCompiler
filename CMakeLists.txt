cmake_minimum_required(VERSION 3.25)
project(TrustCompiler)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_executable(TrustCompiler
        main.cpp
        LexicalAnalyzer/lexical_analyzer.cpp
        utils.h
        SyntaxAnalyzer/syntax_analyzer.cpp)

include_directories(
        .
        LexicalAnalyzer
)

