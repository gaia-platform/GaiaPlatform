#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

enable_testing()

# Set the project name.
project(demos)

# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set some global variables.
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb -pthread")

# Helper function for setting up our tests.
function(set_test target arg result)
  add_test(NAME ${target}_${arg} COMMAND ${target} ${arg})
  set_tests_properties(${target}_${arg} PROPERTIES PASS_REGULAR_EXPRESSION ${result})
endfunction(set_test)

# Set expected test success output string.
set(TEST_SUCCESS "All tests passed!")

find_program(PYTHON "python")

# Add custom targets for airport demo.
if (PYTHON)
  add_custom_target(airport sudo ${PYTHON} ../airport/setup/setup.py install)
  add_custom_target(airport-clean sudo rm -fr build sudo rm -fr dist sudo rm -fr airport.egg-info)
  add_custom_target(gaia-init ${PYTHON} ../airport/setup/gaia_init.py)
endif()

# Add project folders.
add_subdirectory(cow_se)
add_subdirectory(rules)
