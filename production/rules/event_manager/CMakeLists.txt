#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

# Set the project name.
project(event_manager)

set(GAIA_EVENT_MANAGER_INCLUDES 
    ${GAIA_REPO}/production/db/storage_engine/mock
    ${GAIA_INC}/public/common
    ${GAIA_INC}/internal/common
    ${GAIA_INC}/public/rules
    ${GAIA_RULES_INC}
    ${FLATBUFFERS_INC}
    ${PROJECT_SOURCE_DIR}/inc
)

message(STATUS "GAIA_EVENT_MANAGER_INCLUDES=${GAIA_EVENT_MANAGER_INCLUDES}")

# main target
add_library(event_manager SHARED
    src/event_manager.cpp
)
set_target_properties(event_manager PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")

target_include_directories(event_manager PRIVATE ${GAIA_EVENT_MANAGER_INCLUDES})  

# unit tests

#test_event_guard
add_executable(test_event_guard tests/test_event_guard.cpp)
set_target_properties(test_event_guard PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(test_event_guard PRIVATE 
    ${GAIA_EVENT_MANAGER_INCLUDES}
    ${GOOGLE_TEST_INC}
)
target_link_libraries(test_event_guard PRIVATE 
    gtest_main
)
gtest_discover_tests(test_event_guard)

#test_event_manager
add_executable(test_event_manager tests/test_event_manager.cpp)
set_target_properties(test_event_manager PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(test_event_manager PRIVATE 
    ${GAIA_EVENT_MANAGER_INCLUDES}
    ${GOOGLE_TEST_INC}
)
target_link_libraries(test_event_manager PRIVATE 
    event_manager
    gtest_main
)
gtest_discover_tests(test_event_manager)
