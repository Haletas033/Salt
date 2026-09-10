cmake_minimum_required(VERSION 4.3)
project(Salt)

set(CMAKE_CXX_STANDARD 20)

find_package(LLVM REQUIRED CONFIG)

add_executable(Salt
        src/main.cpp
        src/lexer.cpp
        include/lexer.h
        include/tokens.h
        src/parser.cpp
        include/parser.h
        include/codegen.h
        src/codegen.cpp
)

target_include_directories(Salt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(Salt PRIVATE LLVM-22)
target_compile_definitions(Salt PRIVATE ${LLVM_DEFINITIONS})