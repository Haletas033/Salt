cmake_minimum_required(VERSION 4.3)
project(Salt)

set(CMAKE_CXX_STANDARD 20)

add_executable(Salt
        src/main.cpp
        src/lexer.cpp
        include/lexer.h
        include/tokens.h
        src/parser.cpp
        include/parser.h
)

target_include_directories(Salt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
