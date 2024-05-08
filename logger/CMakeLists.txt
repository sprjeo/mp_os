cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0135 NEW)
project(mp_os_lggr
        VERSION 1.0
        DESCRIPTION "logger libraries"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(client_logger)
add_subdirectory(logger)
add_subdirectory(server_logger)