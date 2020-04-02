#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(system)

set(GAIA_SYSTEM_INCLUDES 
    "${GAIA_INC}/public/common"
    "${GAIA_INC}/public/rules"
    "${GAIA_INC}/internal/common"
    "${FLATBUFFERS_INC}"
    "${GAIA_REPO}/production/db/storage_engine/mock"
    "${GAIA_REPO}/production/rules/event_manager/inc")
message(STATUS "GAIA_SYSTEM_INCLUDES=${GAIA_SYSTEM_INCLUDES}")

# Our main target.
add_library(gaia STATIC 
    src/gaia_system.cpp
    src/gaia_object_defs.cpp
    src/storage_engine_defs.cpp
)
target_include_directories(gaia PRIVATE ${GAIA_SYSTEM_INCLUDES})
set_target_properties(gaia PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
