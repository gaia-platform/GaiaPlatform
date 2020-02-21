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
set(GAIA_INCLUDES "${PROJECT_SOURCE_DIR}/inc/")

# Helper function for setting up our tests.
function(set_test target arg result)
  add_test(NAME Test_${target}_${arg} COMMAND ${target} ${arg})
  set_tests_properties(Test_${target}_${arg} PROPERTIES PASS_REGULAR_EXPRESSION ${result})
endfunction(set_test)

# Set tests.
set(TEST_SUCCESS "All tests passed!")

find_program(PYTHON "python")

# Add individual component folders.

# Common code - currently, a headers-only library.
add_library(common INTERFACE)
target_sources(common INTERFACE inc/common/constants.hpp inc/common/gaia_exception.hpp inc/common/retail_assert.hpp)

# Tests.
add_executable(test_retail_assert tests/test_retail_assert.cpp)
target_include_directories(test_retail_assert PRIVATE "${GAIA_INCLUDES}/common")
set_test(test_retail_assert 0 ${TEST_SUCCESS})

