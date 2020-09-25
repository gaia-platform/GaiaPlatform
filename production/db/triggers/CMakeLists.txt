#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(triggers)

set(GAIA_TRIGGERS_INCLUDES
  "${GAIA_DB_INC}/triggers")

message(VERBOSE "GAIA_TRIGGERS_INCLUDES=${GAIA_TRIGGERS_INCLUDES}")

# Our main target.
add_library(gaia_triggers SHARED
  src/events.cpp)
target_include_directories(gaia_triggers PUBLIC ${GAIA_TRIGGERS_INCLUDES})
set_target_properties(gaia_triggers PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_link_libraries(gaia_triggers PRIVATE "gaia_common")
