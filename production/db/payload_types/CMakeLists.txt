#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(payload_types)

set(GAIA_PAYLOAD_TYPES_INCLUDES
  "${GAIA_INC}"
  "${GAIA_INTERNAL_INC}/common"
  "${GAIA_INTERNAL_INC}/db"
  "${GAIA_DB_INC}/payload_types"
  "${GAIA_REPO}/production/db/core/inc"
  "${GAIA_REPO}/production/db/inc/memory_manager"
  "${GAIA_REPO}/production/catalog/inc"
  "${FLATBUFFERS_INC}")

message(VERBOSE "GAIA_PAYLOAD_TYPES_INCLUDES=${GAIA_PAYLOAD_TYPES_INCLUDES}")

# Our main target.
add_library(gaia_payload_types STATIC
  src/data_holder.cpp
  src/type_cache.cpp
  src/field_access.cpp)
target_include_directories(gaia_payload_types PUBLIC ${GAIA_PAYLOAD_TYPES_INCLUDES})
set_target_properties(gaia_payload_types PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_payload_types PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
target_link_libraries(gaia_payload_types PRIVATE gaia_common flatbuffers)
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_payload_types PROPERTY POSITION_INDEPENDENT_CODE ON)

# Generate the flatbuffers files required by test_field_access.
add_custom_command(
  OUTPUT ${PAYLOAD_TYPES_TESTS_GENERATED_OUTPUT}/test_record.bfbs
  COMMAND flatc
    --schema
    -b ${PROJECT_SOURCE_DIR}/tests/test_record.fbs
  DEPENDS ${PROJECT_SOURCE_DIR}/tests/test_record.fbs
  DEPENDS ${CMAKE_BINARY_DIR}/flatbuffers/flatc
  COMMENT "Completed flatbuffers compilation of test_record.fbs!"
  VERBATIM)
add_custom_target(test_record_bfbs ALL DEPENDS ${PAYLOAD_TYPES_TESTS_GENERATED_OUTPUT}/test_record.bfbs)

add_custom_command(
  OUTPUT ${PAYLOAD_TYPES_TESTS_GENERATED_OUTPUT}/test_record_data.bin
  COMMAND flatc
    --force-defaults
    -b ${PROJECT_SOURCE_DIR}/tests/test_record.fbs
    ${PROJECT_SOURCE_DIR}/tests/test_record_data.json
  DEPENDS ${PROJECT_SOURCE_DIR}/tests/test_record_data.json
  DEPENDS ${CMAKE_BINARY_DIR}/flatbuffers/flatc
  COMMENT "Completed flatbuffers compilation of test_record_data.json!"
  VERBATIM)
add_custom_target(test_record_data_bin ALL DEPENDS ${PAYLOAD_TYPES_TESTS_GENERATED_OUTPUT}/test_record_data.bin)

# Tests.
add_gtest(test_field_access tests/test_field_access.cpp "${GAIA_PAYLOAD_TYPES_INCLUDES}" "gaia_common;gaia_payload_types;flatbuffers")
add_gtest(test_data_holder tests/test_data_holder.cpp "${GAIA_PAYLOAD_TYPES_INCLUDES}" "gaia_common;gaia_payload_types;flatbuffers")
# The flatbuffers test also requires our two input files.
configure_file(tests/test_record.fbs . COPYONLY)
configure_file(tests/test_record_data.json . COPYONLY)
add_gtest(test_flatbuffers tests/test_flatbuffers.cpp "${GAIA_PAYLOAD_TYPES_INCLUDES}" "gaia_common;flatbuffers")
