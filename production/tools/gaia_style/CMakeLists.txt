#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(gaia_style)

set(GAIA_GENERATE_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${GAIA_INC}/public/common
        ${GAIA_INC}/public/direct_access
        ${GAIA_INC}/public/catalog
        ${GAIA_INC}/public/db
        )


set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,clang-analyzer-*,-clang-analyzer-cplusplus*)


add_executable(gaia_style src/gaia_format.cpp src/clang_tidy.cpp)
target_include_directories(gaia_style PRIVATE ${GAIA_GENERATE_INCLUDES})
