#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(direct_access)

set(GAIA_DIRECT_ACCESS_INCLUDES 
    "${GAIA_INC}/public/common"
    "${GAIA_INC}/internal/common"
    "${FLATBUFFERS_INC}"
    "${GAIA_REPO}/production/db/storage_engine/mock")
message(STATUS "GAIA_DIRECT_ACCESS_INCLUDES=${GAIA_DIRECT_ACCESS_INCLUDES}")

# Our main target.
add_library(gaia_direct STATIC src/gaia_object_defs.cpp)
target_include_directories(gaia_direct PRIVATE ${GAIA_DIRECT_ACCESS_INCLUDES})
set_target_properties(gaia_direct PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
