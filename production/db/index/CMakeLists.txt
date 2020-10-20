#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(index)

set(GAIA_INDEX_INCLUDES
  "${GAIA_INC}"
  "${GAIA_DB_INC}/index"
  "${GAIA_DB_INC}/payload_types"
  "${FLATBUFFERS_INC}")

message(VERBOSE "GAIA_INDEX_INCLUDES=${GAIA_INDEX_INCLUDES}")

# Our main target.
add_library(gaia_index STATIC
  src/index.cpp
  src/hash_index.cpp
  src/range_index.cpp)
target_include_directories(gaia_index PUBLIC ${GAIA_INDEX_INCLUDES})
set_target_properties(gaia_index PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_index PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
target_link_libraries(gaia_index PRIVATE gaia_common gaia_payload_types)

# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_index PROPERTY POSITION_INDEPENDENT_CODE ON)

# Tests.
add_gtest(test_index_iter tests/test_index_iter.cpp "${GAIA_INDEX_INCLUDES}" "gaia_common;gaia_payload_types;gaia_index")
