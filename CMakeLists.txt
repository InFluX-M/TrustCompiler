cmake_minimum_required(VERSION 3.25)
project(TrustCompiler)

set(CMAKE_CXX_STANDARD 20)

include_directories(.)

add_executable(TrustCompiler
    lexical_analyzer.cpp
    lexical_analyzer.h
    utils.h)
