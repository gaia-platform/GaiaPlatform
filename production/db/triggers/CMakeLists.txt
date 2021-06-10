#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(triggers)

set(GAIA_TRIGGERS_INCLUDES
  "${GAIA_DB_INC}/triggers")

message(VERBOSE "GAIA_TRIGGERS_INCLUDES=${GAIA_TRIGGERS_INCLUDES}")

# Our main target.
add_library(gaia_triggers STATIC
  src/events.cpp)
target_include_directories(gaia_triggers PUBLIC ${GAIA_TRIGGERS_INCLUDES})
set_target_properties(gaia_triggers PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_triggers PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
target_link_libraries(gaia_triggers PRIVATE "gaia_common")
