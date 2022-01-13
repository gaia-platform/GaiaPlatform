#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(payload_types)
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(GAIA_PAYLOAD_TYPES_INCLUDES
  "${GAIA_INC}"
  "${GAIA_DB_INC}/payload_types"
  "${GAIA_REPO}/production/db/core/inc"
  "${GAIA_REPO}/production/db/inc/memory_manager"
  "${GAIA_REPO}/production/catalog/inc"
  "${GEN_DIR}")

message(VERBOSE "GAIA_PAYLOAD_TYPES_INCLUDES=${GAIA_PAYLOAD_TYPES_INCLUDES}")

# Our main target.
add_library(gaia_payload_types STATIC
  src/data_holder.cpp
  src/type_cache.cpp
  src/field_access.cpp
  src/data_buffer.cpp)
configure_gaia_target(gaia_payload_types)
target_include_directories(gaia_payload_types PUBLIC ${GAIA_PAYLOAD_TYPES_INCLUDES})
target_link_libraries(gaia_payload_types PRIVATE gaia_common flatbuffers)

# Generate the flatbuffers files required by test_field_access.
add_custom_command(
  COMMENT "Compiling test_record.fbs with flatbuffers..."
  OUTPUT ${PAYLOAD_TYPES_TESTS_GENERATED_OUTPUT}/test_record.bfbs
  COMMAND ${FLATBUFFERS_BIN}/flatc
    --schema
    -b ${PROJECT_SOURCE_DIR}/tests/test_record.fbs
  DEPENDS ${PROJECT_SOURCE_DIR}/tests/test_record.fbs
  DEPENDS flatc
  VERBATIM)
add_custom_target(test_record_bfbs ALL DEPENDS ${PAYLOAD_TYPES_TESTS_GENERATED_OUTPUT}/test_record.bfbs)

add_custom_command(
  COMMENT "Compiling test_record_data.json with flatbuffers..."
  OUTPUT ${PAYLOAD_TYPES_TESTS_GENERATED_OUTPUT}/test_record_data.bin
  COMMAND ${FLATBUFFERS_BIN}/flatc
    --force-defaults
    -b ${PROJECT_SOURCE_DIR}/tests/test_record.fbs
    ${PROJECT_SOURCE_DIR}/tests/test_record_data.json
  DEPENDS ${PROJECT_SOURCE_DIR}/tests/test_record_data.json
  DEPENDS flatc
  VERBATIM)
add_custom_target(test_record_data_bin ALL DEPENDS ${PAYLOAD_TYPES_TESTS_GENERATED_OUTPUT}/test_record_data.bin)

# Generate flatbuffers files required by test_serializaton.
add_custom_command(
  COMMENT "Compiling test_serialization.fbs with flatbuffers..."
  OUTPUT ${GEN_DIR}/test_serialization_generated.h
  COMMAND ${FLATBUFFERS_BIN}/flatc
    --cpp -o ${GEN_DIR} ${PROJECT_SOURCE_DIR}/tests/test_serialization.fbs
  DEPENDS ${PROJECT_SOURCE_DIR}/tests/test_serialization.fbs
  DEPENDS flatc
  VERBATIM)

add_custom_target(test_serialization_schema ALL DEPENDS ${GEN_DIR}/test_serialization_generated.h)

# Tests.
add_gtest(test_field_access tests/test_field_access.cpp "${GAIA_PAYLOAD_TYPES_INCLUDES}" "gaia_common;gaia_payload_types;flatbuffers")
add_dependencies(test_field_access test_record_data_bin)
add_gtest(test_data_holder tests/test_data_holder.cpp "${GAIA_PAYLOAD_TYPES_INCLUDES}" "gaia_common;gaia_payload_types;flatbuffers")
add_gtest(test_serialization tests/test_serialization.cpp "${GAIA_PAYLOAD_TYPES_INCLUDES}" "gaia_common;gaia_payload_types;flatbuffers")
add_dependencies(test_serialization test_serialization_schema)

# The flatbuffers test also requires our two input files.
configure_file(tests/test_record.fbs . COPYONLY)
configure_file(tests/test_record_data.json . COPYONLY)

add_gtest(test_flatbuffers tests/test_flatbuffers.cpp "${GAIA_PAYLOAD_TYPES_INCLUDES}" "gaia_common;flatbuffers")
add_dependencies(test_flatbuffers test_record_data_bin test_record_bfbs)
