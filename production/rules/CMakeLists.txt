#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(rules)

set(GAIA_RULES_INC "${PROJECT_SOURCE_DIR}/inc")
message(STATUS "GAIA_RULES_INC=${GAIA_RULES_INC}")

# Add individual component folders.
add_subdirectory(event_manager)
