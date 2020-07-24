#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(types)

set(GAIA_TYPES_INCLUDES
  "${GAIA_INC}/public/common"
  "${GAIA_INC}/public/catalog"
  "${GAIA_INC}/public/direct_access"
  "${GAIA_INC}/public/db/"
  "${GAIA_INC}/internal/common"
  "${GAIA_INC}/internal/db"
  "${GAIA_DB_INC}/types"
  "${FLATBUFFERS_INC}"
  "${GAIA_CATALOG_GENERATED}")

message(STATUS "GAIA_TYPES_INCLUDES=${GAIA_TYPES_INCLUDES}")

# Our main target.
add_library(gaia_types STATIC
    src/data_holder.cpp
    src/type_cache.cpp
    src/field_access.cpp
    src/field_list.cpp)
add_dependencies(gaia_types gaia_catalog)
target_include_directories(gaia_types PUBLIC ${GAIA_TYPES_INCLUDES})
set_target_properties(gaia_types PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_link_libraries(gaia_types PRIVATE gaia_common gaia_catalog)
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_types PROPERTY POSITION_INDEPENDENT_CODE ON)

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
add_gtest(test_field_access tests/test_field_access.cpp "${GAIA_TYPES_INCLUDES}" "gaia_common;gaia_types;flatbuffers")
add_gtest(test_data_holder tests/test_data_holder.cpp "${GAIA_TYPES_INCLUDES}" "gaia_common;gaia_types;flatbuffers")
add_gtest(test_list tests/test_field_list.cpp "${GAIA_TYPES_INCLUDES}" "gaia_common;gaia_types;flatbuffers")
