#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)
enable_testing()

# Set the project name.
project(event_manager)

set(GAIA_EVENT_MANAGER_INCLUDES  "${GAIA_INC}/api;${GAIA_INC}/common;${GAIA_RULES_INC};${PROJECT_SOURCE_DIR}/inc;")
message(STATUS "GAIA_EVENT_MANAGER_INCLUDES=${GAIA_EVENT_MANAGER_INCLUDES}")

# main target
add_library(event_manager SHARED
    src/event_manager.cpp
)

target_include_directories(event_manager PRIVATE ${GAIA_EVENT_MANAGER_INCLUDES})  

# unit tests
add_executable(test_event_manager tests/test_event_manager.cpp)
target_include_directories(test_event_manager PRIVATE ${GAIA_EVENT_MANAGER_INCLUDES})
target_link_libraries(test_event_manager PRIVATE event_manager)
set_test(test_event_manager 0 ${TEST_SUCCESS})

