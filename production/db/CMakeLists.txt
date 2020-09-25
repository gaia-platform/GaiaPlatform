#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(db)

set(GAIA_DB_INC "${PROJECT_SOURCE_DIR}/inc")

# message(STATUS "GAIA_DB_INC=${GAIA_DB_INC}")

# Add individual component folders.
add_subdirectory(memory_manager)
add_subdirectory(storage_engine)
add_subdirectory(payload_types)
add_subdirectory(index)
add_subdirectory(triggers)
