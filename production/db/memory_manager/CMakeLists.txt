#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)

# Set the project name.
project(memory_manager)

set(GAIA_MEMORY_MANAGER_INCLUDES "${GAIA_INC}/internal/common;${GAIA_DB_INC}/memory_manager")
message(STATUS "GAIA_MEMORY_MANAGER_INCLUDES=${GAIA_MEMORY_MANAGER_INCLUDES}")

# Our main target.
add_library(memory_manager SHARED
    src/access_control.cpp
    src/base_memory_manager.cpp
    src/memory_manager.cpp
    src/stack_allocator.cpp)
target_include_directories(memory_manager PUBLIC ${GAIA_MEMORY_MANAGER_INCLUDES})
set_target_properties(memory_manager PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_link_libraries(memory_manager PRIVATE common)

# Our unit tests.
add_executable(test_system tests/test_system.cpp)
target_include_directories(test_system PRIVATE
    ${GAIA_MEMORY_MANAGER_INCLUDES}
    ${GOOGLE_TEST_INC})
target_link_libraries(test_system PRIVATE gtest_main)
gtest_discover_tests(test_system)

add_executable(test_access_control tests/test_access_control.cpp)
target_include_directories(test_access_control PRIVATE
    ${GAIA_MEMORY_MANAGER_INCLUDES}
    ${GOOGLE_TEST_INC})
target_link_libraries(test_access_control PRIVATE
    common
    memory_manager
    gtest_main)
gtest_discover_tests(test_access_control)

add_executable(test_memory_manager tests/test_memory_manager.cpp)
target_include_directories(test_memory_manager PRIVATE
    ${GOOGLE_TEST_INC})
target_link_libraries(test_memory_manager PRIVATE
    common
    memory_manager
    gtest_main)
gtest_discover_tests(test_memory_manager)

add_executable(test_stack_allocator tests/test_stack_allocator.cpp)
target_include_directories(test_stack_allocator PRIVATE
    ${GOOGLE_TEST_INC})
target_link_libraries(test_stack_allocator PRIVATE
    common
    memory_manager
    gtest_main)
gtest_discover_tests(test_stack_allocator)
