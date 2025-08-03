cmake_minimum_required(VERSION 3.16)
project(qobd LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(
    -Wall -Wextra -Wshadow -Wconversion -Wsign-conversion
    -Wcast-align -Wformat=2 -Wundef -Wstrict-prototypes
    -pedantic -fstack-protector-strong
)

include_directories(include)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    src/*.c
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)