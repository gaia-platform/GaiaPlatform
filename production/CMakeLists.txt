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

# Helper function for setting up our tests.
function(set_test target arg result)
  add_test(NAME Test_${target}_${arg} COMMAND ${target} ${arg})
  set_tests_properties(Test_${target}_${arg} PROPERTIES PASS_REGULAR_EXPRESSION ${result})
endfunction(set_test)

# Set tests.
set(TEST_SUCCESS "All tests passed!")

find_program(PYTHON "python")

# Add individual component folders.

