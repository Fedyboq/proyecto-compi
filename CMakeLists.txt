cmake_minimum_required(VERSION 3.31)
project(compilador)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(compilador
        ast.cpp
        ast.h
        environment.h
        main.cpp
        parser.cpp
        parser.h
        scanner.cpp
        scanner.h
        token.cpp
        token.h
        visitor.cpp
        visitor.h)
