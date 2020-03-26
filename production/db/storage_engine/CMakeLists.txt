#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(storage_engine)

set(GAIA_STORAGE_ENGINE_INCLUDES "${GAIA_INC}/internal/common;${GAIA_DB_INC}/storage_engine")
message(STATUS "GAIA_STORAGE_ENGINE_INCLUDES=${GAIA_STORAGE_ENGINE_INCLUDES}")

add_subdirectory(mock)
