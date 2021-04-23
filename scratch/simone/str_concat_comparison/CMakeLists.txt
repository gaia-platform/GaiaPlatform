cmake_minimum_required(VERSION 3.16)

project(str_concat_comparison)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(spdlog 1.5.0 REQUIRED CONFIG)

add_executable(str_concat_comparison
    src/main.cpp
    src/timer.cpp
)
target_link_libraries(str_concat_comparison spdlog::spdlog)
target_include_directories(str_concat_comparison
    PRIVATE
    ${PROJECT_SOURCE_DIR}/inc
)
