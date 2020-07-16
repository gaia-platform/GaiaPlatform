#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(triggers)

set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/rules/event_manager/generated")
set(GAIA_TRIGGERS_INCLUDES
  "${GAIA_DB_INC}/triggers"
  "${GAIA_REPO}/production/rules/event_manager/inc"
  "${GAIA_REPO}/production/inc/public/rules"
  "${GAIA_REPO}/production/inc/public/direct_access"
  "${GAIA_REPO}/production/db/storage_engine/mock"
  "${GAIA_INC}/public/common"
  "${GAIA_INC}/internal/common"
  "${GENERATED_OUTPUTS}")

message(STATUS "GAIA_TRIGGERS_INCLUDES=${GAIA_TRIGGERS_INCLUDES}")

# Our main target.
add_library(gaia_triggers SHARED
    src/events.cpp
    src/trigger.cpp)
add_dependencies(gaia_triggers generate_event_logs)   
target_include_directories(gaia_triggers PUBLIC ${GAIA_TRIGGERS_INCLUDES})
set_target_properties(gaia_triggers PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_link_libraries(gaia_triggers PRIVATE "gaia_common")
