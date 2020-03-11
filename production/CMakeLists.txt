#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)

enable_testing()

# Set the project name.
project(production)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

include(gaia)
include(GoogleTest)

# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set some global variables.
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb -pthread")
set(GAIA_INC "${PROJECT_SOURCE_DIR}/inc")
message(STATUS "GAIA_INC=${GAIA_INC}")

get_repo_root(${PROJECT_SOURCE_DIR} "GaiaPlatform" GAIA_REPO)
message(STATUS "GAIA_REPO=${GAIA_REPO}")

set(GOOGLE_TEST "${GAIA_REPO}/third_party/production/googletest")
set(GOOGLE_TEST_INC "${GOOGLE_TEST}/googletest/include;${GOOGLE_TEST}/googlemock/include")

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

# Add googletest
add_subdirectory(${GOOGLE_TEST} googletest)

# Tests.
add_executable(test_retail_assert tests/test_retail_assert.cpp)
target_include_directories(test_retail_assert PRIVATE "${GAIA_INC}/internal/common")
set_test(test_retail_assert 0 ${TEST_SUCCESS})
