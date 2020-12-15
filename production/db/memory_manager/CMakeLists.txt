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
add_library(gaia_memory_manager SHARED
  src/access_control.cpp
  src/base_memory_manager.cpp
  src/memory_manager.cpp
  src/stack_allocator.cpp)
target_include_directories(gaia_memory_manager PUBLIC ${GAIA_MEMORY_MANAGER_INCLUDES})
set_target_properties(gaia_memory_manager PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_link_libraries(gaia_memory_manager PRIVATE gaia_common)

# Our unit tests.
add_gtest(test_system tests/test_system.cpp "${GAIA_MEMORY_MANAGER_INCLUDES}" "gaia_common;gaia_memory_manager")
add_gtest(test_access_control tests/test_access_control.cpp "${GAIA_MEMORY_MANAGER_INCLUDES}" "gaia_common;gaia_memory_manager")
add_gtest(test_memory_manager tests/test_memory_manager.cpp "" "gaia_common;gaia_memory_manager")
add_gtest(test_stack_allocator tests/test_stack_allocator.cpp "" "gaia_common;gaia_memory_manager")
