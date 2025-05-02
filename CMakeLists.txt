cmake_minimum_required(VERSION 3.25)
project(TrustCompiler)

set(CMAKE_CXX_STANDARD 20)

include_directories(.)

add_executable(TrustCompiler
    LexicalAnalyzer/lexical_analyzer.cpp
    LexicalAnalyzer/lexical_analyzer.h
    utils.h main.cpp)
