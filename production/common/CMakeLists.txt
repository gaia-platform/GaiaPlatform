#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(common)

set(GAIA_COMMON_INCLUDES
  "${GAIA_INC}/public/common")

message(STATUS "GAIA_COMMON_INCLUDES=${GAIA_COMMON_INCLUDES}")

# Our main target.
add_library(gaia_common SHARED
    src/gaia_synchronization.cpp)
target_include_directories(gaia_common PUBLIC ${GAIA_COMMON_INCLUDES})
set_target_properties(gaia_common PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")

# Tests.
add_gtest(test_retail_assert tests/test_retail_assert.cpp "${GAIA_INC}/public/common" "")
add_gtest(test_shared_mutex tests/test_shared_mutex.cpp "${GAIA_INC}/public/common" "gaia_common")
