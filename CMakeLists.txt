cmake_minimum_required(VERSION 3.20)
project(Salt)

set(SALT_COMPILER "${CMAKE_SOURCE_DIR}/salt_shaker")
set(SALT_PATH, "${CMAKE_SOURCE_DIR}/std")

set(SALT_SOURCES
        src/main.salt
)

add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/salt_shaker
        COMMAND ${CMAKE_COMMAND} -E env SALT_PATH=$ENV{SALT_PATH}
        ${SALT_COMPILER} ${CMAKE_SOURCE_DIR}/src/main.salt
        -o ${CMAKE_BINARY_DIR}/salt_shaker
        DEPENDS ${SALT_SOURCES}
        COMMENT "Compiling Salt Shaker..."
)

add_custom_target(SaltShaker ALL
        DEPENDS ${CMAKE_BINARY_DIR}/salt_shaker
)