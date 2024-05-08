cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0135 NEW)
project(mp_os_assctv_cntnr)

add_library(
        mp_os_assctv_cntnr
        include/associative_container.h)
target_include_directories(
        mp_os_assctv_cntnr
        PUBLIC
        ./include)
set_target_properties(
        mp_os_assctv_cntnr PROPERTIES
        VERSION 1.0
        DESCRIPTION "associative container interface library"
        LANGUAGES CXX
        LINKER_LANGUAGE CXX
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)