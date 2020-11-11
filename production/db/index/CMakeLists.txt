#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(index)

set(GAIA_INDEX_INCLUDES
  "${GAIA_INC}/public/common"
  "${GAIA_INC}/internal/common"
  "${GAIA_INC}/public/db"  
  "${GAIA_INC}/internal/db"
  "${GAIA_DB_INC}/index"
  "${GAIA_DB_INC}/payload_types"
  "${FLATBUFFERS_INC}")

message(VERBOSE "GAIA_INDEX_INCLUDES=${GAIA_INDEX_INCLUDES}")

# Our main target.
add_library(gaia_index SHARED
  src/index.cpp)
target_include_directories(gaia_index PUBLIC ${GAIA_INDEX_INCLUDES})
set_target_properties(gaia_index PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_link_libraries(gaia_index PRIVATE "gaia_common")
