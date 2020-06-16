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

# Generate the flatbuffers files required by test_field_access.
add_custom_command(
    OUTPUT ${TYPES_TESTS_GENERATED_OUTPUT}/test_record.bfbs
    COMMAND flatc
      --schema
      -b ${PROJECT_SOURCE_DIR}/tests/test_record.fbs
    DEPENDS ${CMAKE_BINARY_DIR}/flatbuffers/flatc
    COMMENT "Completed flatbuffers compilation of test_record.fbs!")
add_custom_target(test_record_bfbs ALL DEPENDS ${TYPES_TESTS_GENERATED_OUTPUT}/test_record.bfbs)

add_custom_command(
    OUTPUT ${TYPES_TESTS_GENERATED_OUTPUT}/test_record_data.bin
    COMMAND flatc
      -b ${PROJECT_SOURCE_DIR}/tests/test_record.fbs
      ${PROJECT_SOURCE_DIR}/tests/test_record_data.json
    DEPENDS ${CMAKE_BINARY_DIR}/flatbuffers/flatc
    COMMENT "Completed flatbuffers compilation of test_record_data.json!")
add_custom_target(test_record_data_bin ALL DEPENDS ${TYPES_TESTS_GENERATED_OUTPUT}/test_record_data.bin)

# Tests.
add_gtest(test_field_access tests/test_field_access.cpp "${GAIA_TYPES_INCLUDES}" "gaia_common;gaia_types")
