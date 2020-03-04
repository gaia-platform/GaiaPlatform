#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)

enable_testing()

# Set the project name.
project(production)

# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set some global variables.
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb -pthread")
set(GAIA_INC "${PROJECT_SOURCE_DIR}/inc")
message(STATUS "GAIA_INC=${GAIA_INC}")

# Helper function for setting up our tests.
function(set_test target arg result)
  add_test(NAME ${target}_${arg} COMMAND ${target} ${arg})
  set_tests_properties(${target}_${arg} PROPERTIES PASS_REGULAR_EXPRESSION ${result})
endfunction(set_test)

# Set tests.
set(TEST_SUCCESS "All tests passed!")

find_program(PYTHON "python")

# Common code - currently, a headers-only library.
add_library(common INTERFACE)
target_sources(common INTERFACE 
  ${PROJECT_SOURCE_DIR}/inc/internal/common/constants.hpp 
  ${PROJECT_SOURCE_DIR}/inc/internal/common/gaia_exception.hpp 
  ${PROJECT_SOURCE_DIR}/inc/internal/common/retail_assert.hpp
)

# Add individual component folders.
add_subdirectory(db)
add_subdirectory(rules)

# Tests.
add_executable(test_retail_assert tests/test_retail_assert.cpp)
target_include_directories(test_retail_assert PRIVATE "${GAIA_INC}/internal/common")
set_test(test_retail_assert 0 ${TEST_SUCCESS})

