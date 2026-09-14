cmake_minimum_required(VERSION 4.3)
project(Salt)

set(CMAKE_CXX_STANDARD 20)

find_package(LLVM REQUIRED CONFIG)

add_executable(salt_shaker
        src/main.cpp
        src/lexer.cpp
        include/lexer.h
        include/tokens.h
        src/parser.cpp
        include/parser.h
        include/codegen.h
        src/codegen.cpp
        include/preprocessor.h
        src/preprocessor.cpp
        include/compiler.h
        include/compilerOptions.h
        src/compiler.cpp
)

target_include_directories(salt_shaker PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(salt_shaker PRIVATE LLVM-22)
target_compile_definitions(salt_shaker PRIVATE ${LLVM_DEFINITIONS})