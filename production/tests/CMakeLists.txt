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
add_gtest(test_retail_assert test_retail_assert.cpp "${GAIA_INC}/public/common" "")
