cmake_minimum_required(VERSION 3.17)
project(haffman_algorithm)

set(CMAKE_CXX_STANDARD 14)

add_executable(encode encode.cpp haffman_algorithm.cpp)
target_include_directories(encode PRIVATE ${PROJECT_SOURCE_DIR})

add_executable(decode decode.cpp haffman_algorithm.cpp)
target_include_directories(encode PRIVATE ${PROJECT_SOURCE_DIR})