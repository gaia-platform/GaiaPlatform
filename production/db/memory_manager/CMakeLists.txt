#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(memory_manager)

set(GAIA_MEMORY_MANAGER_INCLUDES
  "${GAIA_INC}"
  "${GAIA_DB_INC}/memory_manager")

message(VERBOSE "GAIA_MEMORY_MANAGER_INCLUDES=${GAIA_MEMORY_MANAGER_INCLUDES}")

# Our main target.
add_library(gaia_memory_manager STATIC
  src/bitmap.cpp
  src/memory_manager.cpp
  src/chunk_manager.cpp)
configure_gaia_target(gaia_memory_manager)
target_include_directories(gaia_memory_manager PUBLIC ${GAIA_MEMORY_MANAGER_INCLUDES})
target_link_libraries(gaia_memory_manager PRIVATE gaia_common)

# Our unit tests.
add_gtest(test_system tests/test_system.cpp "${GAIA_MEMORY_MANAGER_INCLUDES}" "gaia_common;gaia_memory_manager")
add_gtest(test_bitmap tests/test_bitmap.cpp "${GAIA_MEMORY_MANAGER_INCLUDES}" "gaia_common;gaia_memory_manager")
add_gtest(test_memory_manager tests/test_memory_manager.cpp "" "gaia_common;gaia_memory_manager")
add_gtest(test_chunk_manager tests/test_chunk_manager.cpp "" "gaia_common;gaia_memory_manager")
