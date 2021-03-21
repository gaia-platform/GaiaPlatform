#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb -pthread")

# Enable AddressSanitizer in debug mode.
if(ENABLE_ASAN AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  set(GAIA_COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
  set(GAIA_LINK_FLAGS "${GAIA_LINK_FLAGS} -fsanitize=address")
endif()

set(GAIA "/opt/gaia")

include(${GAIA}/cmake/gaia.cmake)
set(GAIA_INC "${GAIA}/include")

# --- Generate EDC from DDL---
process_schema(
  DDL_FILE ${PING_PONG_DDL}
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR})

# -- Translate ruleset into CPP --
translate_ruleset(
  RULESET_FILE ${PING_PONG_RULESET}
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR}
  DEPENDS generate_ping_pong_headers)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION /usr/local/lib/libgaia.so)

add_executable(ping_pong
  ping_pong.cpp
  ${PROJECT_BINARY_DIR}/ping_pong_ruleset.cpp
)

add_dependencies(ping_pong translate_ping_pong_ruleset)
target_include_directories(ping_pong PRIVATE ${GAIA_INC})
target_include_directories(ping_pong PRIVATE
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
)
target_link_libraries(ping_pong PRIVATE gaia rt pthread)
