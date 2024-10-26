cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(miniclangpp)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
set(LLVM_ENABLE_WARNINGS OFF)

message(STATUS "found llvm ${LLVM_PACKAGE_VERSION}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
llvm_map_components_to_libnames(llvm_libs support core irreader)

file(GLOB_RECURSE SOURCE_FILES 
	${CMAKE_SOURCE_DIR}/src/lexer.cpp
	${CMAKE_SOURCE_DIR}/src/parse_expressions.cpp
	${CMAKE_SOURCE_DIR}/src/parse_declarations.cpp
	${CMAKE_SOURCE_DIR}/src/type.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_compile_definitions(TEST_VERBOSE)
add_compile_options(-Wall -Wextra -pedantic -Werror -Fsanitize=address)

add_library(miniclangpp_lib ${SOURCE_FILES})
target_link_libraries(miniclangpp_lib ${llvm_libs})
link_libraries(miniclangpp_lib)

add_executable(miniclangpp ${CMAKE_SOURCE_DIR}/src/main.cpp)

add_executable(lexer_test ${CMAKE_SOURCE_DIR}/tests/lexer.cpp)
add_executable(parser_test ${CMAKE_SOURCE_DIR}/tests/parser.cpp)
