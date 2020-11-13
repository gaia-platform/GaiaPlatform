#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.17)

enable_testing()

# Set the project name.
project(demos)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../production/cmake;${CMAKE_MODULE_PATH}")
include(gaia)
include(GoogleTest)

# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set some global variables.
get_filename_component(GAIA_REPO ../ ABSOLUTE)

set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb -pthread")
set(GAIA_DEMO_BUILD "${CMAKE_BINARY_DIR}")
set(GAIA_PROD "${GAIA_REPO}/production")
set(GAIA_INC "${GAIA_PROD}/inc")
set(FLATBUFFERS "${GAIA_REPO}/third_party/production/flatbuffers")
set(FLATBUFFERS_INC "${FLATBUFFERS}/include")

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
if (DEFINED GAIA_PROD_BUILD AND EXISTS ${GAIA_PROD_BUILD})
  # Convert relative path to absolute path so custom commands will work if given a relative path
  get_filename_component(GAIA_PROD_BUILD "${GAIA_PROD_BUILD}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
  add_subdirectory(incubator)
  add_subdirectory(tools)
else()
  message(STATUS "To build the incubator demo or the rule_subscriber tool, build production first and set GAIA_PROD_BUILD to the build dir.")
endif()
