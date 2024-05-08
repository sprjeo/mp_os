cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0135 NEW)
project(mp_os)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(allocator)
add_subdirectory(arithmetic)
add_subdirectory(associative_container)
add_subdirectory(common)
add_subdirectory(logger)