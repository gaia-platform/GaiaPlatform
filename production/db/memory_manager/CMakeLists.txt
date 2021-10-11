#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(memory_manager)

set(GAIA_MEMORY_MANAGER_INCLUDES
  "${GAIA_INC}"
  "${GAIA_DB_INC}/memory_manager"
  "${GAIA_DB_INC}/index"
  "${GAIA_REPO}/production/db/core/inc")

message(VERBOSE "GAIA_MEMORY_MANAGER_INCLUDES=${GAIA_MEMORY_MANAGER_INCLUDES}")

# Our main target.
add_library(gaia_memory_manager STATIC
  src/bitmap.cpp
  src/memory_manager.cpp
  src/chunk_manager.cpp)
configure_gaia_target(gaia_memory_manager)
target_include_directories(gaia_memory_manager PUBLIC ${GAIA_MEMORY_MANAGER_INCLUDES})
# libstdc++ apparently needs libatomic for struct last_allocation_metadata_t, even though it's 8 bytes.
target_link_libraries(gaia_memory_manager PRIVATE gaia_common atomic)

# Our unit tests.
add_gtest(test_system tests/test_system.cpp "${GAIA_MEMORY_MANAGER_INCLUDES}" "gaia_common;gaia_memory_manager")
add_gtest(test_bitmap tests/test_bitmap.cpp "${GAIA_MEMORY_MANAGER_INCLUDES}" "gaia_common;gaia_memory_manager")
