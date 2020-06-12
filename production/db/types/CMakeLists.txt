#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(types)

set(GAIA_TYPES_INCLUDES
  "${GAIA_INC}/public/common"
  "${GAIA_DB_INC}/types"
  "${FLATBUFFERS_INC}")

message(STATUS "GAIA_TYPES_INCLUDES=${GAIA_TYPES_INCLUDES}")

# Our main target.
add_library(gaia_types SHARED
    src/field_access.cpp
    src/type_cache.cpp)
target_include_directories(gaia_types PUBLIC ${GAIA_TYPES_INCLUDES})
set_target_properties(gaia_types PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_link_libraries(gaia_types PRIVATE gaia_common)

# Tests.
add_gtest(test_field_access tests/test_field_access.cpp "${GAIA_TYPES_INCLUDES}" "gaia_common;gaia_types")
