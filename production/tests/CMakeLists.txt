#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)

enable_testing()

# Set the project name.
project(tests)

# Add individual component folders.
add_subdirectory(common)

# Tests.
add_executable(test_retail_assert test_retail_assert.cpp)
target_include_directories(test_retail_assert PRIVATE
  "${GAIA_INC}/internal/common"
  ${GOOGLE_TEST_INC})
target_link_libraries(test_retail_assert PRIVATE gtest_main)
gtest_discover_tests(test_retail_assert)
