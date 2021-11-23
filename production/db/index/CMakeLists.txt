#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(index)

set(GAIA_INDEX_INCLUDES
  "${GAIA_INC}"
  "${GAIA_DB_INC}/index"
  "${GAIA_DB_INC}/payload_types"
  "${FLATBUFFERS_INC}")

message(VERBOSE "GAIA_INDEX_INCLUDES=${GAIA_INDEX_INCLUDES}")

# Our main target.
add_library(gaia_index STATIC
  src/base_index.cpp
  src/index.cpp
  src/hash_index.cpp
  src/range_index.cpp)
configure_gaia_target(gaia_index)
target_include_directories(gaia_index PUBLIC ${GAIA_INDEX_INCLUDES})
target_link_libraries(gaia_index PRIVATE gaia_common gaia_payload_types)

set (INDEX_TEST_INCLUDES
  ${GAIA_INDEX_INCLUDES}
  ${GAIA_GENERATED_SCHEMAS}
)

# Tests.

add_gtest(test_index tests/test_index.cpp "${INDEX_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;gaia_common;gaia_payload_types;gaia_index;dac_prerequisites")
