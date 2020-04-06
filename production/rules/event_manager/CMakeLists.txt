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
add_gtest(test_event_guard "tests/test_event_guard.cpp" "${GAIA_EVENT_MANAGER_INCLUDES}" "")
add_gtest(test_event_manager "tests/test_event_manager.cpp" "${GAIA_EVENT_MANAGER_INCLUDES}" "event_manager;rt")
