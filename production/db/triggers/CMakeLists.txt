###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(triggers)

set(GAIA_TRIGGERS_INCLUDES
  "${GAIA_DB_INC}/triggers")

message(VERBOSE "GAIA_TRIGGERS_INCLUDES=${GAIA_TRIGGERS_INCLUDES}")

# Our main target.
add_library(gaia_triggers STATIC
  src/events.cpp)
configure_gaia_target(gaia_triggers)
target_include_directories(gaia_triggers PUBLIC ${GAIA_TRIGGERS_INCLUDES})
target_link_libraries(gaia_triggers PRIVATE "gaia_common")
