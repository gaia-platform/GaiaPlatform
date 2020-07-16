#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(system)

set(GAIA_SYSTEM_INCLUDES 
    "${GAIA_INC}/public/common"
    "${GAIA_INC}/public/direct_access"
    "${GAIA_INC}/public/system"
    "${GAIA_INC}/public/rules"
    "${GAIA_INC}/internal/common"
    "${FLATBUFFERS_INC}"
    "${GAIA_REPO}/production/db/storage_engine/mock"
    "${GAIA_REPO}/production/db/inc/triggers"
    "${GAIA_REPO}/production/rules/event_manager/inc"
    "${CMAKE_BINARY_DIR}/rules/event_manager/generated"
)
message(STATUS "GAIA_SYSTEM_INCLUDES=${GAIA_SYSTEM_INCLUDES}")

# Our main target.
add_library(gaia STATIC src/gaia_system.cpp)
target_include_directories(gaia PRIVATE ${GAIA_SYSTEM_INCLUDES})
target_link_libraries(gaia gaia_semock gaia_direct gaia_rules)
set_target_properties(gaia PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
