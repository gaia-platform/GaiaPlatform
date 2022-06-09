###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

set(GAIA "/opt/gaia")

include(${GAIA}/cmake/gaia.cmake)
set(GAIA_INC "${GAIA}/include")

# --- Generate DAC from DDL---
process_schema(
  DDL_FILE ${PING_PONG_DDL}
  DATABASE_NAME ping_pong
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

target_compile_options(ping_pong PRIVATE -Wall -Wextra -ggdb)
target_link_options(ping_pong PRIVATE -ggdb -pthread)

# Enable AddressSanitizer in debug mode.
if(ENABLE_ASAN AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  target_compile_options(ping_pong PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(ping_pong PRIVATE -fsanitize=address)
endif()

add_dependencies(ping_pong translate_ping_pong_ruleset)
target_include_directories(ping_pong PRIVATE ${GAIA_INC})
target_include_directories(ping_pong PRIVATE
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
)
target_link_libraries(ping_pong PRIVATE gaia rt pthread)
